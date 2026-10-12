// A field across a list, and counting a list field:
//
//   the names of the drinks          each drink's name          (a list of each drink's name)
//   a customer is loyal if they have more than 5 orders.
//   if ann has at least 2 orders, ...
//
// "the FIELDs of L" (the plural of a field that isn't a field itself) and
// "each ITEM's FIELD" (L the list named like ITEM's plural) are "each NAME in
// L's FIELD" (EXPR_EVERY), with a hidden name for the item. "X has|have more
// than|at least|fewer than|less than|at most|exactly N FIELD" is "the number
// of items in X's FIELD" compared with N. In a definition ("a customer is
// loyal if ..."), "they" is "it", like "their" is "its".

#include <string.h>
#include "front/parse_internal.h"

static Expr *every_field(Parser *p, Expr *list, Name field, SourcePos start) {
    Expr *every = ast_new_expr(p->arena, EXPR_EVERY, start);
    every->as.every.var.text = arena_sprintf(p->arena, "0each%zu", ++p->hidden_lists);
    every->as.every.var.display = field.display;
    every->as.every.var.pos = start;
    every->as.every.list = list;
    Expr *item = ast_new_expr(p->arena, EXPR_NAME, start);
    item->as.name = every->as.every.var.text;
    item->display = "it";
    Expr *body = ast_new_expr(p->arena, EXPR_FIELD, field.pos);
    body->as.field.record = item;
    body->as.field.name = field;
    every->as.every.body = body;
    every->pos = ast_pos_join(start, list->pos);
    return every;
}

// "names of": a plural whose singular is a field (and that isn't a field, a
// definition or a known name itself), then "of".
static const char *field_plural_at(const Parser *p) {
    const Token *word = parser_peek(p, 0);
    if (word->kind != TOK_WORD || !parser_at_word(p, 1, "of") || parser_known_field_words(p, 0) > 0 ||
        parser_is_function(p, word->text) || parser_match_name(p, 0, false)) {
        return NULL;
    }
    const char *one = parser_singular_of(p->arena, word->text);
    if (!one) return NULL;
    for (size_t i = 0; i < p->fields.len; i++) {
        if (p->fields.items[i].words == 1 && strcmp(p->fields.items[i].text, one) == 0) return one;
    }
    return NULL;
}

bool parser_at_field_across(const Parser *p) {
    if (field_plural_at(p)) return true;
    if (!parser_at_word(p, 0, "each") || parser_is_function(p, "each")) return false;
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, 1 + n))) n++;
    return n > 0 && parser_at(p, 1 + n, TOK_POSSESSIVE);
}

Expr *parser_parse_field_across(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    const char *one = field_plural_at(p);
    if (one) {  // the names of the drinks
        Name field = {one, one, start};
        parser_consume(p, 2);  // names of
        Expr *list = parser_parse_filters(p, parse_primary(p));
        return every_field(p, list, field, start);
    }
    parser_advance(p);  // each
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, n))) n++;
    Name item;
    parser_take_name(p, n, &item);
    Expr *list = parser_implied_list(p, &item);
    parser_advance(p);  // 's
    size_t words = parser_field_words(p, 0);
    if (words == 0) {
        if (parser_error(p, p->prev_span, "Something is missing after \"'s\".")) {
            diag_note(p->diag, "Write the name of a field after it, like \"each drink's name\".");
            diag_info(p->diag, DIAG_MISSING, "a field", NULL);
        }
        return parser_error_expr(p);
    }
    Name field;
    parser_take_name(p, words, &field);
    Expr *every = every_field(p, list, field, start);
    every->pos = ast_pos_join(start, field.pos);
    return every;
}

// --- they have more than 5 orders ---------------------------------------------------------

static const struct {
    const char *words;
    BinaryOp op;
} counts[] = {
    {"more than", BINARY_GREATER}, {"at least", BINARY_GREATER_EQUAL}, {"fewer than", BINARY_LESS},
    {"less than", BINARY_LESS},    {"at most", BINARY_LESS_EQUAL},     {"exactly", BINARY_EQUAL},
};

// "has a score of at least 60": the field compared with the number ("of 60": equal).
static Expr *parse_field_test(Parser *p, Expr *left, size_t words) {
    Span verb = parser_consume(p, 1);  // has, have
    Expr *field = ast_new_expr(p->arena, EXPR_FIELD, left->pos);
    field->as.field.record = left;
    parser_take_name(p, words, &field->as.field.name);
    parser_advance(p);  // of
    BinaryOp op = BINARY_EQUAL;
    for (size_t i = 0; i < sizeof counts / sizeof counts[0]; i++) {
        size_t n = parser_match_words(p, counts[i].words);
        if (n) {
            verb = parser_consume(p, n);
            op = counts[i].op;
            break;
        }
    }
    Expr *amount = parse_unary(p);
    Expr *test = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(left->pos, amount->pos));
    test->as.binary.op = op;
    test->as.binary.op_span = verb;
    test->as.binary.left = field;
    test->as.binary.right = amount;
    return test;
}

Expr *parser_parse_count_test(Parser *p, Expr *left) {
    if (!parser_at_word(p, 0, "has") && !parser_at_word(p, 0, "have")) return NULL;
    size_t field_words = parser_field_words(p, 1);  // a known field, or a table's column
    if (field_words > 0 && parser_peek(p, 1)->filler.length > 0 && parser_at_word(p, 1 + field_words, "of")) {
        return parse_field_test(p, left, field_words);  // "have a score of at least 60"
    }
    for (size_t i = 0; i < sizeof counts / sizeof counts[0]; i++) {
        size_t n = parser_match_words_at(p, 1, counts[i].words);
        if (!n) continue;
        size_t at = p->pos;
        Span verb = parser_consume(p, 1 + n);
        Expr *amount = parse_unary(p);
        size_t words = parser_field_words(p, 0);
        if (p->sentence_failed) return parser_error_expr(p);
        if (words == 0) {  // "has more than 5": not about a field (the map test has no "more than")
            p->pos = at;
            if (parser_error(p, verb, "I expected what to count after the number, like \"has more than 5 orders\".")) {
                diag_note(p->diag, "Write the name of a list the record has after the number.");
                diag_info(p->diag, DIAG_MISSING, "what to count", NULL);
            }
            return parser_error_expr(p);
        }
        Expr *field = ast_new_expr(p->arena, EXPR_FIELD, left->pos);
        field->as.field.record = left;
        parser_take_name(p, words, &field->as.field.name);
        Expr *length = ast_new_expr(p->arena, EXPR_LENGTH, field->as.field.name.pos);
        length->as.operand = field;
        Expr *test = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(left->pos, field->as.field.name.pos));
        test->as.binary.op = counts[i].op;
        test->as.binary.op_span = verb;
        test->as.binary.left = length;
        test->as.binary.right = amount;
        return test;
    }
    return NULL;
}
