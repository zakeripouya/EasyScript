// The built-ins that take a function, each as a value and (where it changes
// a list) a sentence:
//
//   double applied to every number in scores      apply double to every number in scores.
//   the numbers in scores where it is above 50    keep only the numbers in scores where it is above 50.
//   the numbers in scores that pass is even       keep only the numbers in scores that pass is even.
//   scores combined using plus, starting from 0   (the numbers in scores combined using ... too)
//   names sorted by length of [biggest first]     sort names by length of [, biggest first].
//
// A function here is any single value holding one; a function's name means
// the function itself, not a call of it. A sentence works its new items out
// from the list (held in a hidden name, "0listN") and then replaces the
// list's items with them, so every name for the list sees the change.

#include <string.h>
#include "front/parse_internal.h"

// A function given to a built-in: a single value, whose name isn't called ("using double").
static Expr *parse_function_operand(Parser *p) {
    bool no_call = p->no_call;
    p->no_call = true;
    Expr *f = parse_primary(p);
    p->no_call = no_call;
    return f;
}

static Expr *new_apply(Parser *p, ApplyKind kind, Expr *function, Expr *list, SourcePos start) {
    Expr *expr = ast_new_expr(p->arena, EXPR_APPLY, ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0}));
    expr->as.apply.kind = kind;
    expr->as.apply.function = function;
    expr->as.apply.list = list;
    return expr;
}

static bool expect(Parser *p, const char *words, const char *display, const char *example) {
    if (p->sentence_failed) return false;
    return parser_expect_phrase(p, words, display, example);
}

// The words naming the items, up to "in" (or "of"): "number" in "every number in scores".
static bool skip_item_words(Parser *p, const char *example) {
    static const char *const in[] = {"in", "of", NULL};
    Name ignored;
    return parser_parse_name(p, &ignored, in) && parser_expect_word(p, in, example);
}

// [,] biggest first / smallest first
static bool parse_order(Parser *p, bool *descending) {
    size_t comma = parser_at(p, 0, TOK_COMMA) ? 1 : 0;
    bool biggest = parser_at_word(p, comma, "biggest"), smallest = parser_at_word(p, comma, "smallest");
    if ((!biggest && !smallest) || !parser_at_word(p, comma + 1, "first")) return false;
    parser_consume(p, comma + 2);
    *descending = biggest;
    return true;
}

// combined using F [, starting from S] (at "combined")
static Expr *parse_combined(Parser *p, Expr *list, SourcePos start) {
    static const char *const example = "scores combined using sum, starting from 0";
    parser_consume(p, 2);  // combined using
    Expr *f = parse_function_operand(p);
    if (p->sentence_failed) return parser_error_expr(p);
    Expr *expr = new_apply(p, APPLY_COMBINE, f, list, start);
    if (parser_at(p, 0, TOK_COMMA) && parser_match_words_at(p, 1, "starting from")) {
        parser_consume(p, 3);
        if (parser_at_sentence_end(p)) {
            expect(p, "starting from 0", "a value", example);
            return parser_error_expr(p);
        }
        expr->as.apply.start = parse_unary(p);
        expr->pos = ast_pos_join(expr->pos, expr->as.apply.start->pos);
    }
    return expr;
}

// --- Values -------------------------------------------------------------------------------

// How many words name the items before "in" ("numbers in", "big scores in"), or 0.
static size_t item_words(const Parser *p) {
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, n)) && !parser_at_word(p, n, "in")) n++;
    return n > 0 && parser_at_word(p, n, "in") ? n : 0;
}

bool parser_at_selection(const Parser *p) {
    size_t n = item_words(p);
    if (n == 0) return false;
    int depth = 0;
    for (size_t i = n + 1; ; i++) {
        TokenKind kind = parser_peek(p, i)->kind;
        if (kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT ||
            kind == TOK_INDENT || kind == TOK_COLON) {
            return false;
        }
        if (kind == TOK_LPAREN) depth++;
        if (kind == TOK_RPAREN && --depth < 0) return false;
        if (depth == 0 && (parser_at_word(p, i, "where") || parser_match_words_at(p, i, "that pass") ||
                           parser_match_words_at(p, i, "that passes") ||
                           parser_match_words_at(p, i, "combined using"))) {
            return true;
        }
    }
}

// the Xs in L where C / that pass F / combined using F
Expr *parser_parse_selection(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    parser_consume(p, item_words(p) + 1);  // the items' name, and "in"
    Expr *list = parse_primary(p);
    if (p->sentence_failed) return parser_error_expr(p);
    if (parser_at_word(p, 0, "where")) {
        parser_advance(p);
        Expr *only = ast_new_expr(p->arena, EXPR_ONLY, start);
        only->as.only.list = list;
        only->as.only.condition = parse_expression(p);
        only->pos = ast_pos_join(start, only->as.only.condition->pos);
        return only;
    }
    size_t that = parser_match_words(p, "that passes");
    if (!that) that = parser_match_words(p, "that pass");
    if (that) {
        parser_consume(p, that);
        Expr *f = parse_function_operand(p);
        return new_apply(p, APPLY_FILTER, f, list, start);
    }
    if (parser_match_words(p, "combined using")) return parse_combined(p, list, start);
    const Token *token = parser_peek(p, 0);
    if (parser_error(p, token->span, "I expected \"where\", \"that pass\" or \"combined using\" here, but found %s.",
                     parser_describe(p, token))) {
        diag_note(p->diag, "Put the list in parentheses if it's more than one word, like \"the numbers in (item 1 "
                           "of tables) where it is above 50\".");
    }
    return parser_error_expr(p);
}

// L where C / L that pass F / L that are COMPARISON V / L whose FIELD ...:
// the items of L the condition is yes for (in the condition, "it" is the item).
static Expr *parse_filter(Parser *p, Expr *list) {
    SourcePos start = list->pos;
    if (parser_at_word(p, 0, "where")) {
        parser_advance(p);
        Expr *only = ast_new_expr(p->arena, EXPR_ONLY, start);
        only->as.only.list = list;
        only->as.only.condition = parse_expression(p);
        only->pos = ast_pos_join(start, only->as.only.condition->pos);
        return only;
    }
    size_t that = parser_match_words(p, "that passes");
    if (!that) that = parser_match_words(p, "that pass");
    if (that) {
        parser_consume(p, that);
        Expr *f = parse_function_operand(p);
        return new_apply(p, APPLY_FILTER, f, list, start);
    }
    Expr *only = ast_new_expr(p->arena, EXPR_ONLY, start);
    only->as.only.list = list;
    if (parser_at_word(p, 0, "that")) {
        parser_advance(p);
        only->as.only.condition = parser_parse_item_test(p, "the scores that are at least 70");
    } else {
        Span whose = parser_advance(p)->span;
        if (parser_field_words(p, 0) == 0) {
            const Token *token = parser_peek(p, 0);
            if (parser_error(p, token->kind == TOK_WORD ? token->span : whose,
                             "\"whose\" needs a field after it, but no kind of record has one called %s.",
                             parser_describe(p, token))) {
                diag_note(p->diag, "Write it like \"the dogs whose age is over 3\".");
                diag_info(p->diag, DIAG_UNKNOWN, "a field after \"whose\"", NULL);
                for (size_t i = 0; i < p->fields.len; i++) diag_choice(p->diag, "whose %s", p->fields.items[i].text);
            }
            return parser_error_expr(p);
        }
        Expr *it = ast_new_expr(p->arena, EXPR_IT, (SourcePos){whose, 0, 0, 0});
        it->pos = parser_token_pos(&p->tokens[p->pos - 1]);
        Expr *field = ast_new_expr(p->arena, EXPR_FIELD, it->pos);
        field->as.field.record = it;
        parser_take_name(p, parser_field_words(p, 0), &field->as.field.name);
        field->pos = field->as.field.name.pos;
        only->as.only.condition = parser_parse_condition_after(p, field);
    }
    only->pos = ast_pos_join(start, only->as.only.condition->pos);
    return only;
}

// Where a filter starts: "where", "that pass", "that are ...", or "whose" and a field.
static bool at_filter(const Parser *p) {
    return parser_at_word(p, 0, "where") || parser_match_words(p, "that pass") || parser_match_words(p, "that passes") ||
           (parser_at_word(p, 0, "that") && parser_at_item_test(p, 1)) ||
           (parser_at_word(p, 0, "whose") && parser_peek(p, 1)->kind == TOK_WORD);
}

Expr *parser_parse_filters(Parser *p, Expr *list) {
    while (!p->sentence_failed && at_filter(p)) list = parse_filter(p, list);
    return list;
}

Expr *parser_parse_apply_postfix(Parser *p, Expr *expr) {
    if (at_filter(p)) return parse_filter(p, expr);
    if (parser_match_words(p, "applied to every")) {
        parser_consume(p, 3);
        if (!skip_item_words(p, "double applied to every number in scores")) return parser_error_expr(p);
        Expr *list = parse_primary(p);
        return new_apply(p, APPLY_MAP, expr, list, expr->pos);
    }
    if (parser_match_words(p, "combined using")) return parse_combined(p, expr, expr->pos);
    if (parser_match_words(p, "sorted by") && !parser_at_word(p, 2, "their")) {
        parser_consume(p, 2);
        Expr *f = parse_function_operand(p);
        Expr *sorted = new_apply(p, APPLY_SORT, f, expr, expr->pos);
        if (parse_order(p, &sorted->as.apply.descending)) sorted->pos = ast_pos_join(sorted->pos, (SourcePos){p->prev_span, 0, 0, 0});
        return sorted;
    }
    return NULL;
}

// --- Sentences --------------------------------------------------------------------------

// The list a sentence changes, kept in a hidden name while the new items are worked out.
static Expr *hide(Parser *p, Stmt *stmt, Expr *list) {
    stmt->as.list.list = list;
    Expr *hidden = ast_new_expr(p->arena, EXPR_NAME, list->pos);
    hidden->as.name = arena_sprintf(p->arena, "0list%zu", ++p->hidden_lists);
    hidden->display = parser_text(p, list->pos.span);
    stmt->as.list.hidden = hidden;
    return hidden;
}

// apply F to every X in L
Stmt *parser_parse_apply(Parser *p, const Token *verb) {
    static const char *const example = "apply double to every number in scores";
    Stmt *stmt = parser_new_stmt(p, STMT_LIST, verb);
    stmt->as.list.op = LIST_APPLY;
    Expr *f = parse_function_operand(p);
    if (!expect(p, "to every", "to every", example) || !skip_item_words(p, example)) return NULL;
    Expr *list = parse_primary(p);
    if (p->sentence_failed) return NULL;
    stmt->as.list.value = new_apply(p, APPLY_MAP, f, hide(p, stmt, list), f->pos);
    return stmt;
}

bool parser_at_keep_only(const Parser *p) {
    return parser_at_word(p, 0, "only") && !parser_at_word(p, 1, "as") && !parser_at_word(p, 1, "at");
}

// keep only the Xs in L where C / that pass F (at "only")
Stmt *parser_parse_keep_only(Parser *p, const Token *verb) {
    static const char *const example = "keep only the numbers in scores where it is above 50";
    Stmt *stmt = parser_new_stmt(p, STMT_LIST, verb);
    stmt->as.list.op = LIST_KEEP;
    parser_advance(p);  // only
    if (!item_words(p)) {
        expect(p, "numbers in", "the items in", example);
        return NULL;
    }
    parser_consume(p, item_words(p) + 1);
    Expr *list = parse_primary(p);
    if (p->sentence_failed) return NULL;
    Expr *hidden = hide(p, stmt, list);
    if (parser_at_word(p, 0, "where")) {
        parser_advance(p);
        Expr *only = ast_new_expr(p->arena, EXPR_ONLY, list->pos);
        only->as.only.list = hidden;
        only->as.only.condition = parse_expression(p);
        stmt->as.list.value = only;
        return stmt;
    }
    size_t that = parser_match_words(p, "that passes");
    if (!that) that = parser_match_words(p, "that pass");
    if (!that) {
        expect(p, "where", "where", example);
        return NULL;
    }
    parser_consume(p, that);
    Expr *f = parse_function_operand(p);
    stmt->as.list.value = new_apply(p, APPLY_FILTER, f, hidden, list->pos);
    return stmt;
}

// sort L by F [, biggest first] (after "sort L by", when the function is next)
Stmt *parser_finish_sort_by(Parser *p, Stmt *stmt) {
    stmt->as.list.op = LIST_SORT_BY;
    Expr *list = stmt->as.list.list;
    Expr *f = parse_function_operand(p);
    if (p->sentence_failed) return NULL;
    Expr *sorted = new_apply(p, APPLY_SORT, f, hide(p, stmt, list), list->pos);
    parse_order(p, &sorted->as.apply.descending);
    stmt->as.list.descending = sorted->as.apply.descending;
    stmt->as.list.value = sorted;
    return stmt;
}
