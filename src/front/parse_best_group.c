// The best group, when the item is a field of the rows rather than a list:
//
//   the store with the highest total amount [of purchases]
//   the store with the most number of purchases     the category with the lowest average amount
//   the top 3 stores by total amount [of purchases]
//
// It means exactly what a person would write by hand: the total (average,
// median, number) per store, sorted biggest first (smallest first for
// "lowest"), then the first key, or the first N keys. The parser writes it
// that way: the question per group (EXPR_AGGREGATE with a group), then the
// map sorted, its keys, and the first item (EXPR_ITEM) or the first N
// (EXPR_SLICE). Without "of L", the list is the one list whose items have the
// field: an EXPR_NAME "0rows FIELD" that the checker resolves (and asks
// about when it's none or several).

#include <string.h>
#include "front/parse_internal.h"

static const char *const lowest_words[] = {"lowest", "smallest", "fewest", "least", NULL};
static const char *const highest_words[] = {"highest", "biggest", "largest", "greatest", "most", NULL};

static bool word_among(const Parser *p, size_t ahead, const char *const *words) {
    for (size_t i = 0; words[i]; i++) {
        if (parser_at_word(p, ahead, words[i])) return true;
    }
    return false;
}

// "total", "average", "median" (one word) or "number of" (two): the kind, or AGG_ANY for none.
static AggregateKind measure_at(const Parser *p, size_t ahead, size_t *words) {
    *words = 1;
    if (parser_at_word(p, ahead, "total")) return AGG_TOTAL;
    if (parser_at_word(p, ahead, "average")) return AGG_AVERAGE;
    if (parser_at_word(p, ahead, "median")) return AGG_MEDIAN;
    *words = 2;
    if (parser_at_word(p, ahead, "number") && parser_at_word(p, ahead + 1, "of")) return AGG_COUNT;
    return AGG_ANY;
}

// A field's name of `words` words at `ahead`, that isn't a kind's name.
static bool group_field_at(const Parser *p, size_t ahead, size_t words) {
    size_t kind_words;
    if (parser_match_record(p, ahead, false, &kind_words) >= 0) return false;
    return words > 0 && parser_field_words(p, ahead) == words;  // a known field, or a table's column
}

bool parser_at_best_group(const Parser *p) {
    size_t n = parser_field_words(p, 0), words;
    return n > 0 && group_field_at(p, 0, n) && parser_at_word(p, n, "with") &&
           (word_among(p, n + 1, highest_words) || word_among(p, n + 1, lowest_words)) &&
           measure_at(p, n + 2, &words) != AGG_ANY;
}

bool parser_at_top_group(const Parser *p) {
    if (!(parser_at_word(p, 0, "top") || parser_at_word(p, 0, "lowest")) || parser_is_function(p, parser_peek(p, 0)->text)) {
        return false;
    }
    const Token *count = parser_peek(p, 1), *plural = parser_peek(p, 2);
    if (!(count->kind == TOK_NUMBER || parser_is_name_word(count)) || plural->kind != TOK_WORD) return false;
    const char *one = parser_singular_of(p->arena, plural->text);
    size_t words, kind_words;
    if (!one || !parser_at_word(p, 3, "by") || measure_at(p, 4, &words) == AGG_ANY) return false;
    if (parser_match_record(p, 2, true, &kind_words) >= 0) return false;  // "the top 3 dogs by ...": a kind's list
    if (p->tables.len > 0) return true;                                    // a table's column
    for (size_t i = 0; i < p->fields.len; i++) {
        if (strcmp(p->fields.items[i].text, one) == 0) return true;
    }
    return false;
}

static Expr *it_field(Parser *p, Name name) {
    Expr *it = ast_new_expr(p->arena, EXPR_IT, name.pos);
    Expr *field = ast_new_expr(p->arena, EXPR_FIELD, name.pos);
    field->as.field.record = it;
    field->as.field.name = name;
    return field;
}

static Expr *list_op(Parser *p, ListOp op, Expr *operand, Span span) {
    Expr *expr = ast_new_expr(p->arena, EXPR_LIST_OP, operand->pos);
    expr->as.list_op.op = op;
    expr->as.list_op.op_span = span;
    expr->as.list_op.operand = operand;
    return expr;
}

// MEASURE [FIELD] [of L], with the group's field: the question per group,
// sorted, and its keys. NULL after an error.
static Expr *grouped_keys(Parser *p, Name group, bool lowest, SourcePos start) {
    size_t words;
    const Token *word = parser_peek(p, 0);
    AggregateKind kind = measure_at(p, 0, &words);
    Span op_span = parser_consume(p, words);
    Expr *question = ast_new_expr(p->arena, EXPR_AGGREGATE, start);
    question->as.aggregate.kind = kind;
    question->as.aggregate.op_span = op_span;
    question->as.aggregate.group = it_field(p, group);
    if (kind == AGG_COUNT) {  // "the number of purchases": the list
        question->as.aggregate.list = parser_parse_filters(p, parse_primary(p));
    } else {
        size_t field = parser_field_words(p, 0);
        if (field == 0) {
            if (parser_error(p, word->span, "I expected a field after \"%s\" here, but found %s.", word->text,
                             parser_describe(p, parser_peek(p, 0)))) {
                diag_note(p->diag, "Write it like \"the store with the highest total amount\".");
                diag_info(p->diag, DIAG_MISSING, arena_sprintf(p->arena, "a field after \"%s\"", word->text), NULL);
            }
            return NULL;
        }
        Name key;
        parser_take_name(p, field, &key);
        question->as.aggregate.key = it_field(p, key);
        if (parser_at_word(p, 0, "of")) {
            parser_advance(p);
            question->as.aggregate.list = parser_parse_filters(p, parse_primary(p));
        } else {  // the one list whose items have the group's field (the checker finds it)
            Expr *rows = ast_new_expr(p->arena, EXPR_NAME, group.pos);
            rows->as.name = arena_sprintf(p->arena, "0rows %s", group.text);
            rows->display = group.display;
            question->as.aggregate.list = rows;
        }
    }
    if (p->sentence_failed) return NULL;
    question->pos = ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0});
    Expr *sorted = list_op(p, lowest ? LIST_SORTED : LIST_SORTED_DOWN, question, op_span);
    return list_op(p, LIST_KEYS, sorted, op_span);
}

// the FIELD with the highest|lowest MEASURE ...: the first key.
Expr *parser_parse_best_group(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Name group;
    parser_take_name(p, parser_field_words(p, 0), &group);
    parser_advance(p);  // with
    bool lowest = word_among(p, 0, lowest_words);
    parser_advance(p);
    Expr *keys = grouped_keys(p, group, lowest, start);
    if (!keys) return parser_error_expr(p);
    Expr *best = ast_new_expr(p->arena, EXPR_ITEM, ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0}));
    best->as.item.where = ITEM_FIRST;
    best->as.item.list = keys;
    return best;
}

// the top|lowest N FIELDs by MEASURE ...: the first N keys.
Expr *parser_parse_top_group(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    bool lowest = parser_at_word(p, 0, "lowest");
    parser_advance(p);
    const Token *n = parser_advance(p);
    Expr *count = ast_new_expr(p->arena, n->kind == TOK_NUMBER ? EXPR_NUMBER : EXPR_NAME, parser_token_pos(n));
    if (n->kind == TOK_NUMBER) {
        count->as.number.text = n->text;
        count->as.number.is_decimal = strchr(n->text, '.') != NULL;
    } else {
        count->as.name = n->text;
        count->display = parser_text(p, n->span);
    }
    const Token *plural = parser_advance(p);
    Name group = {parser_singular_of(p->arena, plural->text), parser_singular_of(p->arena, plural->text),
                  parser_token_pos(plural)};
    parser_advance(p);  // by
    Expr *keys = grouped_keys(p, group, lowest, start);
    if (!keys) return parser_error_expr(p);
    Expr *top = ast_new_expr(p->arena, EXPR_SLICE, ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0}));
    top->as.slice.kind = SLICE_FIRST;
    top->as.slice.list = keys;
    top->as.slice.from = count;
    return top;
}
