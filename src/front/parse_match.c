// Telling the variants of a kind apart:
//
//   check which kind s is:              if s is a circle:
//       when it's a circle:                 say s's radius.
//           say its radius.
//       when it's a square or a point:
//           say "corners".
//       when it's nothing:              (for a value that can be nothing)
//           say "none".
//       otherwise:
//           say "something else".
//
// "it's" is "it" and a possessive to the lexer; "when it is a circle" works
// too. A variant's name needs its "a" (or "an") in "is a circle", like "is a
// letter", so "x is circle" still compares with a variable called circle.

#include <string.h>
#include "front/parse_internal.h"

static const char *const match_example = "check which kind s is:";
static const char *const when_example = "when it's a circle:";

// The word was written with "a" or "an" before it.
static bool after_article(const Parser *p, const Token *token) {
    Span f = token->filler;
    if (f.length == 0 || f.length > 2) return false;
    const char *s = p->source + f.offset;
    return (s[0] | 0x20) == 'a' && (f.length == 1 || (s[1] | 0x20) == 'n');
}

Expr *parser_parse_kind_test(Parser *p, Expr *left) {
    size_t verb = parser_match_words(p, "is not") ? 2 : parser_at_word(p, 0, "is") || parser_at_word(p, 0, "isn't") ? 1 : 0;
    if (verb == 0 || !after_article(p, parser_peek(p, verb))) return NULL;
    size_t words;
    int variant = parser_match_variant(p, verb, &words);
    if (variant < 0) return NULL;
    bool negated = verb == 2 || parser_at_word(p, 0, "isn't");
    Span op_span = parser_consume(p, verb + words);
    Expr *expr = ast_new_expr(p->arena, EXPR_KIND_TEST, left->pos);
    expr->as.kind_test.operand = left;
    expr->as.kind_test.variant = (size_t)variant;
    expr->as.kind_test.negated = negated;
    expr->as.kind_test.op_span = op_span;
    expr->pos = ast_pos_join(left->pos, (SourcePos){op_span, 0, 0, 0});
    return expr;
}

// --- check which kind S is -----------------------------------------------------------

static void report_unknown_variant(Parser *p, const Token *token) {
    if (!parser_error(p, token->span, "I don't know a variant called \"%s\".", token->text)) return;
    size_t n = p->records->variants.len;
    const char **names = arena_alloc(p->arena, (n + 1) * sizeof(char *));
    for (size_t i = 0; i < n; i++) names[i] = p->records->variants.items[i].name;
    const char *closest = parser_closest_words(p, token->text, names, n);
    if (closest) {
        diag_note(p->diag, "Did you mean %s?", closest);
    } else {
        diag_note(p->diag, "Write one of the variants listed where its kind is defined, like \"%s\", or "
                           "\"when it's nothing:\".", when_example);
    }
}

// [a] VARIANT or [a] VARIANT ... (or nothing), after "when it's".
static bool parse_when_kinds(Parser *p, MatchBranch *branch) {
    do {
        if (parser_at_word(p, 0, "nothing")) {
            parser_advance(p);
            branch->nothing = true;
            continue;
        }
        const Token *token = parser_peek(p, 0);
        size_t words;
        int variant = parser_match_variant(p, 0, &words);
        if (variant < 0 && (parser_at_sentence_end(p) || token->kind == TOK_COLON)) {
            if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
                diag_note(p->diag, "Say which variant, like \"%s\".", when_example);
            }
            return false;
        }
        if (variant < 0) {
            if (token->kind == TOK_WORD) {
                report_unknown_variant(p, token);
            } else if (parser_error(p, token->span, "I expected the name of a variant here, but found %s.",
                                    parser_describe(p, token))) {
                diag_note(p->diag, "Write it like \"%s\".", when_example);
            }
            return false;
        }
        parser_consume(p, words);
        vec_push(p->arena, &branch->variants, (size_t)variant);
    } while (parser_at_word(p, 0, "or") && parser_advance(p));
    return true;
}

// when it's [a] VARIANT [or ...]: (or "when it is ...")
static bool parse_when(Parser *p, MatchBranch *branch) {
    parser_advance(p);  // when
    size_t it = parser_at_word(p, 0, "it") && (parser_at(p, 1, TOK_POSSESSIVE) || parser_at_word(p, 1, "is")) ? 2 : 0;
    if (it == 0) {
        const Token *token = parser_peek(p, 0);
        bool reported = parser_at_sentence_end(p) || token->kind == TOK_COLON
                            ? parser_error(p, p->prev_span, "Something is missing after \"when\".")
                            : parser_error(p, token->span, "I expected \"it's\" here, but found %s.",
                                           parser_describe(p, token));
        if (reported) diag_note(p->diag, "Write it like \"%s\".", when_example);
        return false;
    }
    parser_consume(p, it);
    if (!parse_when_kinds(p, branch)) return false;
    if (parser_at_word(p, 0, "whose") || parser_at_word(p, 0, "where") || parser_at_word(p, 0, "that")) {
        Expr *it = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(parser_peek(p, 0)));
        Expr *only = parser_parse_filters(p, it);  // the filter's words, on the value checked
        if (p->sentence_failed || only->kind != EXPR_ONLY) return false;
        branch->guard = only->as.only.condition;
    }
    return true;
}

// A line inside "check which kind" that isn't a "when" or an "otherwise".
static void reject_line(Parser *p) {
    const Token *token = parser_peek(p, 0);
    if (parser_error(p, token->span, "Each part of \"check which kind\" starts with \"when it's ...\" or "
                                     "\"otherwise\", but this starts with %s.", parser_describe(p, token))) {
        diag_note(p->diag, "Write it like \"%s\", with its sentences indented below.", when_example);
    }
    parser_skip_line_and_block(p);
}

// The "when" and "otherwise" parts, up to and including the DEDENT that ends them.
static bool parse_branches(Parser *p, Stmt *stmt) {
    bool ok = true;
    size_t otherwise_line = 0;
    while (!parser_at(p, 0, TOK_EOF)) {
        TokenKind kind = parser_peek(p, 0)->kind;
        if (kind == TOK_DEDENT) {
            parser_advance(p);
            break;
        }
        if (kind == TOK_NEWLINE || kind == TOK_PERIOD) {
            parser_advance(p);
            continue;
        }
        const Token *first = parser_peek(p, 0);
        bool when = parser_at_word(p, 0, "when");
        if (!when && !parser_at_otherwise(p)) {
            reject_line(p);
            ok = false;
            continue;
        }
        if (otherwise_line) {
            if (parser_error(p, first->span, "The \"otherwise\" on line %zu has to come last.", otherwise_line)) {
                diag_note(p->diag, "Move this part above it.");
            }
            parser_skip_line_and_block(p);
            ok = false;
            continue;
        }
        MatchBranch branch = {0};
        branch.pos = parser_token_pos(first);
        if (when && !parse_when(p, &branch)) {
            parser_skip_line_and_block(p);
            ok = false;
            continue;
        }
        if (!when) {
            parser_advance(p);
            branch.otherwise = true;
            otherwise_line = first->line;
        }
        if (!parser_parse_block(p, &branch.body, &branch.pos, NULL, when ? "this \"when\"" : "this \"otherwise\"")) {
            ok = false;
            continue;
        }
        vec_push(p->arena, &stmt->as.match.branches, branch);
    }
    if (ok && stmt->as.match.branches.len > 0 && stmt->as.match.branches.items[0].otherwise) {
        if (parser_error(p, stmt->as.match.branches.items[0].pos.span, "\"check which kind\" needs a \"when\" before "
                                                                       "its \"otherwise\".")) {
            diag_note(p->diag, "Write a part like \"%s\" first.", when_example);
        }
        ok = false;
    }
    return ok;
}

// check which kind S is: NEWLINE INDENT (when ...: | otherwise:)... DEDENT
Stmt *parse_match(Parser *p, const Token *verb) {
    static const char *const is[] = {"is", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_MATCH, verb);
    bool ok = parser_expect_phrase(p, "which kind", "which kind", match_example);
    if (ok) {
        stmt->as.match.subject = parse_join(p);
        ok = !p->sentence_failed && parser_expect_word(p, is, match_example);
    }
    if (ok && !parser_at(p, 0, TOK_COLON)) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected a colon after \"is\", but found %s.", parser_describe(p, token))) {
            diag_note(p->diag, "Write it like \"%s\", with \"when it's ...\" parts indented below.", match_example);
        }
        ok = false;
    }
    if (ok) {
        stmt->pos = ast_pos_join(stmt->pos, parser_token_pos(parser_advance(p)));
        if (!parser_at(p, 0, TOK_NEWLINE) || !parser_at(p, 1, TOK_INDENT)) {
            if (parser_error(p, stmt->pos.span, "Nothing is indented under this \"check which kind\".")) {
                diag_note(p->diag, "Put a part like \"%s\" on the next line, indented, with its sentences indented "
                                   "below it.", when_example);
            }
            ok = false;
        }
    }
    if (!ok) {
        parser_skip_line_and_block(p);
        p->ended_with_block = true;
        return NULL;
    }
    parser_consume(p, 2);
    ok = parse_branches(p, stmt);
    p->ended_with_block = true;  // set last: statements inside the blocks reset it
    return ok ? stmt : NULL;
}
