// Depends-on tables, at the start of a line:
//
//   the shipping cost depends on the total:      makes "shipping cost" (like "let"), from the
//       0 when it's over 100.                    first line whose condition is yes for the
//       5 when it's over 50.                     total ("it" in the conditions)
//       10 otherwise.
//
//   the price of a drink depends on its size:    a value of every drink, like "a drink's price
//       3 when it's "small".                     is ...": "the price of latte" works it out
//       5 otherwise.
//
// Every table ends with an "otherwise" line, so every case has a value; one
// without it is an error that says which case is missing when the conditions
// make that clear ("when the total is 50 or less").

#include <stdlib.h>
#include <string.h>
#include "front/parse_internal.h"

static bool at_line_start(const Parser *p, size_t at) {
    if (at == 0) return true;
    TokenKind before = p->tokens[at - 1].kind;
    return before == TOK_NEWLINE || before == TOK_INDENT || before == TOK_DEDENT;
}

// How many words name what's defined ("shipping cost", "price"), with
// *kind_words the kind's for "the price of a drink"; 0 if no table starts here.
static size_t table_words(const Parser *p, size_t *kind_words) {
    *kind_words = 0;
    if (!at_line_start(p, p->pos) || parser_is_statement_word(parser_peek(p, 0)->text)) return 0;
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, n)) && !parser_at_word(p, n, "depends") && !parser_at_word(p, n, "of")) n++;
    if (n == 0) return 0;
    if (parser_match_words_at(p, n, "depends on")) return n;
    size_t words;
    Type kind = parser_at_word(p, n, "of") ? parser_match_kind_name(p, n + 1, false, &words) : TYPE_UNKNOWN;
    if (!(type_is_record(kind) || type_is_sum(kind)) || !parser_match_words_at(p, n + 1 + words, "depends on")) return 0;
    *kind_words = words;
    return n;
}

bool parser_at_table(const Parser *p) {
    size_t kind;
    return table_words(p, &kind) > 0;
}

void parser_find_tables(Parser *p) {
    size_t saved = p->pos, kind;
    for (p->pos = 0; p->pos + 1 < p->count; p->pos++) {
        size_t n = table_words(p, &kind);
        if (n == 0) continue;
        Name name;
        size_t at = p->pos;
        parser_take_name(p, n, &name);
        p->pos = at;
        if (kind) {
            parser_add_field_name(p, name.text, n);  // "the price of latte", "its price"
        } else if (n > 1) {
            KnownName known = {name.text, n, false, 0, false};  // "the shipping cost", like a "let"
            vec_push(p->arena, &p->names, known);
        }
    }
    p->pos = saved;
}

// --- The lines ----------------------------------------------------------------------

static bool line_end(Parser *p, const char *example) {
    if (parser_at(p, 0, TOK_PERIOD)) parser_advance(p);
    if (parser_at(p, 0, TOK_NEWLINE)) {
        parser_advance(p);
        return true;
    }
    if (parser_at(p, 0, TOK_DEDENT) || parser_at(p, 0, TOK_EOF)) return true;
    const Token *token = parser_peek(p, 0);
    if (parser_error(p, token->span, "I expected the end of the line here, but found %s.", parser_describe(p, token))) {
        diag_note(p->diag, "Each line of the table is one value and when it applies, like \"%s\".", example);
        diag_info(p->diag, DIAG_NOT_ALLOWED, "more after the line", NULL);
    }
    return false;
}

// VALUE when it's REST / VALUE when it is REST / VALUE otherwise
static bool parse_line(Parser *p, Expr *table, bool *otherwise) {
    static const char *const example = "5 when it's over 50.";
    if (*otherwise) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "The \"otherwise\" line has to be the last one.")) {
            diag_note(p->diag, "It says what the value is when no line above it applies, so move it to the end.");
            diag_info(p->diag, DIAG_NOT_ALLOWED, "a line after \"otherwise\"", NULL);
        }
        return false;
    }
    Expr *value = parse_join(p);
    if (p->sentence_failed) return false;
    vec_push(p->arena, &table->as.table.values, value);
    if (parser_at_word(p, 0, "otherwise")) {
        parser_advance(p);
        *otherwise = true;
        vec_push(p->arena, &table->as.table.conditions, (Expr *)NULL);
        return line_end(p, example);
    }
    bool possessive = parser_match_words(p, "when it") && parser_at(p, 2, TOK_POSSESSIVE);
    bool other = parser_match_words(p, "when it") && !possessive;  // "when it is over 50", "when it starts with"
    if (!possessive && !other) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected \"when it's\" or \"otherwise\" here, but found %s.",
                         parser_describe(p, token))) {
            diag_note(p->diag, "Write each line like \"%s\" or \"10 otherwise.\".", example);
            diag_info(p->diag, DIAG_MISSING, "when the value applies", NULL);
            diag_choice(p->diag, "when it's ...");
            diag_choice(p->diag, "otherwise");
        }
        return false;
    }
    Span when = parser_consume(p, possessive ? 3 : 1);  // "when it's", or "when" (then "it" starts the comparison)
    Expr *it = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(&p->tokens[p->pos - (possessive ? 2 : 0)]));
    Expr *condition;
    if (possessive) {
        condition = parser_parse_is_rest(p, it, when);
    } else {
        parser_advance(p);  // it
        condition = parser_parse_comparison_after(p, it);
        if (condition == it && !p->sentence_failed) {
            const Token *token = parser_peek(p, 0);
            if (parser_error(p, token->span, "I expected a comparison after \"when it\", but found %s.",
                             parser_describe(p, token))) {
                diag_note(p->diag, "Write it like \"%s\" or \"5 when it starts with \"a\".\".", example);
                diag_info(p->diag, DIAG_MISSING, "a comparison", NULL);
            }
            return false;
        }
    }
    if (p->sentence_failed) return false;
    vec_push(p->arena, &table->as.table.conditions, condition);
    return line_end(p, example);
}

// Skips the rest of the table's block after a mistake, so it causes no more.
static void skip_block(Parser *p) {
    for (size_t depth = 1; depth > 0 && !parser_at(p, 0, TOK_EOF); parser_advance(p)) {
        if (parser_at(p, 0, TOK_INDENT)) depth++;
        if (parser_at(p, 0, TOK_DEDENT) && --depth == 0) break;
    }
    if (parser_at(p, 0, TOK_DEDENT)) parser_advance(p);
}

// "50 or less" for lines that are all "over N" (or "at least N"), and so on; NULL when it isn't that simple.
static const char *missing_case(Parser *p, const Expr *table) {
    const Expr *lowest = NULL, *highest = NULL;
    bool up = false, down = false;
    for (size_t i = 0; i < table->as.table.conditions.len; i++) {
        const Expr *c = table->as.table.conditions.items[i];
        if (c->kind != EXPR_BINARY || c->as.binary.right->kind != EXPR_NUMBER) return NULL;
        BinaryOp op = c->as.binary.op;
        double n = strtod(c->as.binary.right->as.number.text, NULL);
        if (op == BINARY_GREATER || op == BINARY_GREATER_EQUAL) {
            up = true;
            if (!lowest || n < strtod(lowest->as.binary.right->as.number.text, NULL)) lowest = c;
        } else if (op == BINARY_LESS || op == BINARY_LESS_EQUAL) {
            down = true;
            if (!highest || n > strtod(highest->as.binary.right->as.number.text, NULL)) highest = c;
        } else {
            return NULL;
        }
    }
    if (up == down) return NULL;
    const Expr *edge = up ? lowest : highest;
    const char *n = edge->as.binary.right->as.number.text;
    switch (edge->as.binary.op) {
    case BINARY_GREATER: return arena_sprintf(p->arena, "%s or less", n);
    case BINARY_GREATER_EQUAL: return arena_sprintf(p->arena, "below %s", n);
    case BINARY_LESS: return arena_sprintf(p->arena, "%s or more", n);
    default: return arena_sprintf(p->arena, "over %s", n);
    }
}

// ":" and the indented lines.
static bool parse_lines(Parser *p, Expr *table, const char *name, const char *subject, SourcePos header) {
    if (!parser_at(p, 0, TOK_COLON) || !parser_at(p, 1, TOK_NEWLINE) || !parser_at(p, 2, TOK_INDENT)) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->kind == TOK_COLON ? token->span : p->prev_span,
                         "A table needs its lines below it, indented, after a \":\".")) {
            diag_note(p->diag, "Write it like \"%s depends on %s:\" with lines like \"5 when it's over 50.\" "
                               "below it, and \"10 otherwise.\" last.", name, subject);
            diag_info(p->diag, DIAG_MISSING, "the table's lines", NULL);
        }
        parser_skip_line_and_block(p);
        return false;
    }
    parser_consume(p, 3);
    bool otherwise = false;
    while (!parser_at(p, 0, TOK_DEDENT) && !parser_at(p, 0, TOK_EOF)) {
        if (!parse_line(p, table, &otherwise)) {
            skip_block(p);
            return false;
        }
    }
    if (parser_at(p, 0, TOK_DEDENT)) parser_advance(p);
    if (otherwise) return true;
    const char *missing = missing_case(p, table);
    if (parser_error(p, header.span, missing ? "\"%s\" doesn't say what it is when %s is %s."
                                             : "\"%s\" doesn't say what it is in every case.", name, subject, missing)) {
        diag_note(p->diag, "Add a last line like \"10 otherwise.\" for %s.", missing ? "that case" : "the other cases");
        diag_info(p->diag, DIAG_MISSING, "an \"otherwise\" line", NULL);
    }
    return false;
}

Stmt *parser_parse_table(Parser *p) {
    size_t kind_words;
    size_t n = table_words(p, &kind_words);
    const Token *first = parser_peek(p, 0);
    Name name;
    parser_take_name(p, n, &name);
    Stmt *stmt;
    if (kind_words) {  // the price of a drink: a value of every drink
        parser_advance(p);  // of
        stmt = parser_new_method(p, kind_words, DEFINES_VALUE);
        stmt->pos = ast_pos_join(parser_token_pos(first), stmt->pos);
        stmt->as.function.name = name;
        stmt->as.function.name.text = arena_sprintf(p->arena, "the %s", name.text);
    } else {
        stmt = parser_new_stmt(p, STMT_LET, first);
        stmt->as.assign.name = name;
    }
    parser_consume(p, 2);  // depends on
    Expr *table = ast_new_expr(p->arena, EXPR_TABLE, parser_token_pos(parser_peek(p, 0)));
    const Token *subject_start = parser_peek(p, 0);
    table->as.table.subject = parse_join(p);
    p->ended_with_block = true;
    if (p->sentence_failed) {
        parser_skip_line_and_block(p);
        return NULL;
    }
    SourcePos header = ast_pos_join(parser_token_pos(first), table->as.table.subject->pos);
    Span shown = table->as.table.subject->pos.span;  // with a dropped "the": "the total"
    const Token *first_word = subject_start;
    if (first_word->filler.length && first_word->filler.offset < shown.offset) {
        shown.length += shown.offset - first_word->filler.offset;
        shown.offset = first_word->filler.offset;
    }
    const char *subject = parser_text(p, shown);
    if (!parse_lines(p, table, arena_sprintf(p->arena, "the %s", name.display), subject, header)) return NULL;
    table->pos = header;
    if (kind_words) {
        Stmt *ret = ast_new_stmt(p->arena, STMT_RETURN, header);
        ret->as.returned = table;
        vec_push(p->arena, &stmt->as.function.body, ret);
    } else {
        stmt->as.assign.value = table;
    }
    stmt->pos = header;
    p->ended_with_block = true;  // set last: the lines don't reset it, but keep it clear
    return stmt;
}
