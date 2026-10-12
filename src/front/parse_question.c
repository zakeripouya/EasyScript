// Questions and requests: each means exactly what the same words mean in a
// program. A question ends with "?" (a sentence end, like "."):
//
//   what is total?  what's total?  what are the big orders?  total?    say total
//   is total above 100?  is rex big?                                      say total is above 100
//   how many orders are big?                                              say how many orders are big
//   which customer has the highest total?                                 say the customer with the highest total
//   which orders are big?                                                 say the orders that are big
//
// Requests: "show me X", "give me X", "tell me X", "list X" are "say X", and
// polite words before a sentence ("I want to", "I'd like to", "let's", "can
// you", "could you", "would you", "please") are left out.

#include <string.h>
#include "front/parse_internal.h"

// The sentence starting here ends with "?" (at its own level, not in parentheses).
bool parser_question_ahead(const Parser *p) {
    int depth = 0;
    for (size_t i = 0;; i++) {
        const Token *token = parser_peek(p, i);
        switch (token->kind) {
        case TOK_LPAREN: depth++; break;
        case TOK_RPAREN: depth--; break;
        case TOK_PERIOD:
            if (depth <= 0) return token->text[0] == '?';
            break;
        case TOK_NEWLINE:
        case TOK_EOF:
        case TOK_INDENT:
        case TOK_DEDENT:
        case TOK_COLON: return false;
        default: break;
        }
    }
}

static bool ends_at(const Parser *p, size_t ahead) {
    TokenKind kind = parser_peek(p, ahead)->kind;
    return kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT || kind == TOK_INDENT;
}

void parser_skip_polite(Parser *p) {
    static const char *const openers[] = {"i want to", "i'd like to", "i would like to", "can you", "could you",
                                          "would you", "please"};
    for (bool again = true; again;) {
        again = false;
        for (size_t i = 0; i < sizeof openers / sizeof openers[0]; i++) {
            size_t n = parser_match_words(p, openers[i]);
            if (n && !ends_at(p, n)) {
                parser_consume(p, n);
                again = true;
            }
        }
        if (parser_at_word(p, 0, "let") && parser_at(p, 1, TOK_POSSESSIVE) && !ends_at(p, 2)) {
            parser_consume(p, 2);  // let's
            again = true;
        }
    }
}

// for example, X is Y.
Stmt *parser_parse_example(Parser *p) {
    const Token *first = parser_peek(p, 0);
    parser_consume(p, 3);  // for example ,
    Stmt *stmt = parser_new_stmt(p, STMT_EXAMPLE, first);
    stmt->as.value = parse_expression(p);
    return p->sentence_failed ? NULL : stmt;
}

static Expr *which(Parser *p) {
    SourcePos start = parser_token_pos(parser_advance(p));
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, n)) && !parser_at_word(p, n, "has") && !parser_at_word(p, n, "with")) n++;
    if (n > 0 && (parser_at_word(p, n, "has") || parser_at_word(p, n, "with"))) {  // which customer has the highest total
        Name item;
        parser_take_name(p, n, &item);
        return parser_finish_best(p, parser_new_best(p, &item, start));
    }
    Expr *list = parse_primary(p);  // which orders are big
    if (p->sentence_failed) return list;
    Expr *only = ast_new_expr(p->arena, EXPR_ONLY, start);
    only->as.only.list = list;
    only->as.only.condition = parser_parse_item_test(p, "which orders are big?");
    only->pos = ast_pos_join(start, only->as.only.condition->pos);
    return only;
}

// A question ("...?") whose first word doesn't start a sentence: what it asks, said.
Stmt *parser_parse_question(Parser *p) {
    const Token *first = parser_peek(p, 0);
    Stmt *say = parser_new_stmt(p, STMT_SAY, first);
    if (parser_at_word(p, 0, "who") || parser_at_word(p, 0, "where") || parser_at_word(p, 0, "why") ||
        parser_at_word(p, 0, "when")) {
        if (parser_error(p, first->span, "I can't answer \"%s\" questions yet.", first->text)) {
            diag_note(p->diag, "Ask with \"what\", \"which\", \"how many\" or \"is\", like \"which customer has the "
                               "highest total?\".");
            diag_info(p->diag, DIAG_NOT_ALLOWED, arena_sprintf(p->arena, "a \"%s\" question", first->text), NULL);
            diag_choice(p->diag, "what is ...?");
            diag_choice(p->diag, "which ...?");
        }
        return NULL;
    }
    bool what_in = (parser_at(p, 1, TOK_POSSESSIVE) || parser_at_word(p, 1, "is")) && parser_at_word(p, 2, "in");
    if (parser_at_word(p, 0, "what") && !what_in) {
        parser_advance(p);
        if (parser_at_word(p, 0, "is") || parser_at_word(p, 0, "are") || parser_at(p, 0, TOK_POSSESSIVE)) {
            parser_advance(p);
        }
        say->as.value = parser_parse_pieces(p);
    } else if (parser_at_word(p, 0, "is") || parser_at_word(p, 0, "are")) {
        Span is = parser_advance(p)->span;
        Expr *left = parse_join(p);
        say->as.value = p->sentence_failed ? left : parser_parse_is_rest(p, left, is);
    } else if (parser_at_word(p, 0, "which")) {
        say->as.value = which(p);
    } else {
        say->as.value = parser_parse_pieces(p);  // "total?"
    }
    return p->sentence_failed ? NULL : say;
}
