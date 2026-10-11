// Maps: writing them, looking up values, and kinds of map.
//
//   a map of "coffee" to 3, "tea" to 2     an empty map from text to numbers
//   the value for "tea" in prices           the value for "milk" in prices, or 0
//   the keys of prices                      the values of prices
//   the number of entries in prices         a map from text to lists of numbers (a kind)
//
// "M has K" is a comparison (parse_expr.c) and "for each name and age in
// ages:" a loop (parse_loop.c). "set the value for K in M to V" and "add 1
// to the value for K in M" change it through parser_parse_target
// (parse_record.c), and "remove K from M" is next to "remove item N from L"
// (parse_list.c).

#include <string.h>
#include "front/parse_internal.h"

static const char *const map_example = "a map of \"coffee\" to 3, \"tea\" to 2";
static const char *const lookup_example = "the value for \"tea\" in prices";

// --- Kinds of map -------------------------------------------------------------------

bool parser_at_map_kind(const Parser *p, bool plural) {
    return parser_at_word(p, 0, plural ? "maps" : "map") && parser_at_word(p, 1, "from");
}

// "text" or "numbers": what a map's keys are.
static bool parse_key_kind(Parser *p, Type *key) {
    if (parser_at_word(p, 0, "text") || parser_at_word(p, 0, "numbers")) {
        *key = parser_at_word(p, 0, "text") ? TYPE_TEXT : TYPE_NUMBER;
        parser_advance(p);
        return true;
    }
    const Token *token = parser_peek(p, 0);
    bool reported = parser_at_sentence_end(p) || token->kind == TOK_COLON || token->kind == TOK_RPAREN
                        ? parser_error(p, p->prev_span, "Something is missing after \"from\".")
                        : parser_error(p, token->span, "I expected \"text\" or \"numbers\" here, but found %s.",
                                       parser_describe(p, token));
    if (reported) diag_note(p->diag, "A map's keys are text or numbers, like \"a map from text to numbers\".");
    return false;
}

// map from KEYS to VALUES (or "maps from", in a list's or map's kind). The
// values are in the plural, like a list's items: "numbers", "lists of text".
bool parser_parse_map_kind(Parser *p, Type *type) {
    static const char *const to[] = {"to", NULL};
    parser_consume(p, 2);  // map from
    Type key, value;
    if (!parse_key_kind(p, &key) || !parser_expect_word(p, to, "a map from text to numbers")) return false;
    TokenKind next = parser_peek(p, 0)->kind;
    if (parser_at_sentence_end(p) || next == TOK_COLON || next == TOK_RPAREN) {
        if (parser_error(p, p->prev_span, "Something is missing after \"to\".")) {
            diag_note(p->diag, "Say what the map holds, like \"a map from text to numbers\".");
        }
        return false;
    }
    if (!parser_parse_items_kind(p, &value)) return false;
    *type = ast_map_type(p->arena, p->records, key, value);
    return true;
}

// --- Writing maps -------------------------------------------------------------------

// A key or value of "a map of ...": a value up to "to", "and" or ",". A list
// or map written inside needs parentheses, or its "and"s would be ambiguous.
static Expr *parse_entry_part(Parser *p) {
    bool list = parser_match_words(p, "list of") > 0;
    if (list || parser_match_words(p, "map of")) {
        Span span = parser_consume(p, 2);
        if (parser_error(p, span, "A %s inside a map needs parentheses.", list ? "list" : "map")) {
            diag_note(p->diag, "Write it like \"%s\".", list ? "a map of \"odd\" to (a list of 1 and 3)"
                                                              : "a map of \"Ada\" to (a map of \"age\" to 36)");
        }
        return parser_error_expr(p);
    }
    return parse_join(p);
}

// map of K to V, K to V and K to V: commas separate entries until the first
// "and"; after that, only "and" does, as in a list.
static Expr *parse_map_literal(Parser *p) {
    static const char *const to[] = {"to", NULL};
    Expr *map = ast_new_expr(p->arena, EXPR_MAP, parser_token_pos(parser_peek(p, 0)));
    parser_consume(p, 2);  // map of
    bool after_and = false;
    for (;;) {
        Expr *key = parse_entry_part(p);
        if (p->sentence_failed || !parser_expect_word(p, to, map_example)) return parser_error_expr(p);
        Expr *value = parse_entry_part(p);
        vec_push(p->arena, &map->as.items, key);
        vec_push(p->arena, &map->as.items, value);
        map->pos = ast_pos_join(map->pos, value->pos);
        if (parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call")) {
            after_and = true;
        } else if (after_and || !parser_at(p, 0, TOK_COMMA)) {
            return map;
        }
        parser_advance(p);
    }
}

// empty map from KEYS to VALUES
static Expr *parse_empty_map(Parser *p) {
    static const char *const from[] = {"from", NULL};
    Expr *map = ast_new_expr(p->arena, EXPR_EMPTY_MAP, parser_token_pos(parser_peek(p, 0)));
    parser_advance(p);  // empty
    if (!parser_at_map_kind(p, false)) {
        parser_advance(p);  // map
        parser_expect_word(p, from, "an empty map from text to numbers");
        return parser_error_expr(p);
    }
    if (!parser_parse_map_kind(p, &map->as.list_type)) return parser_error_expr(p);
    map->pos = ast_pos_join(map->pos, (SourcePos){p->prev_span, 0, 0, 0});
    return map;
}

// --- Looking up values ------------------------------------------------------------

static bool is_parenthesized(const Parser *p, const Expr *expr) {
    Span span = expr->pos.span;
    return span.length >= 2 && p->source[span.offset] == '(' && p->source[span.offset + span.length - 1] == ')';
}

// ", or D" with arithmetic straight after D could take D alone or the whole
// value ("the value for w in counts, or 0 plus 1"), so that's an error.
static Expr *reject_ambiguous_fallback(Parser *p, Expr *lookup) {
    const Expr *fallback = lookup->as.lookup.fallback;
    size_t len;
    if (is_parenthesized(p, fallback) || !parser_match_arithmetic(p, &len)) return lookup;
    Span op_span = parser_consume(p, len);
    Expr *right = parse_unary(p);
    SourcePos whole = ast_pos_join(lookup->pos, right->pos);
    Span head = {lookup->pos.span.offset, fallback->pos.span.offset - lookup->pos.span.offset};
    const char *op = parser_text(p, op_span), *right_text = parser_text(p, right->pos.span);
    if (parser_error(p, whole.span, "\"%s\" could mean two things.", parser_text(p, whole.span))) {
        diag_note(p->diag, "Use parentheses to say which: \"(%s) %s %s\" or \"%s(%s %s %s)\".",
                  parser_text(p, lookup->pos.span), op, right_text, parser_text(p, head),
                  parser_text(p, fallback->pos.span), op, right_text);
    }
    return ast_new_expr(p->arena, EXPR_ERROR, whole);
}

// The key of "the value for K in M": a value up to "in". A name straight
// before "in" is the key itself, so "the value for item in prices" doesn't
// read "item in" as a list's item with its position missing.
static Expr *parse_key(Parser *p) {
    const KnownName *known = parser_match_name(p, 0, false);
    size_t words = known ? known->words : 1;
    if (!parser_is_name_word(parser_peek(p, 0)) || !parser_at_word(p, words, "in")) return parse_join(p);
    Name name;
    parser_take_name(p, words, &name);
    Expr *key = ast_new_expr(p->arena, EXPR_NAME, name.pos);
    key->as.name = name.text;
    key->display = name.display;
    return key;
}

bool parser_at_lookup(const Parser *p) {
    return parser_at_word(p, 0, "value") && parser_at_word(p, 1, "for") && !parser_is_function(p, "value");
}

// value for K in M [, or D]: the map is a single value, like a list's in
// "item 2 of L"; D is a single value too.
Expr *parser_parse_lookup(Parser *p) {
    static const char *const in[] = {"in", NULL};
    const Token *value = parser_peek(p, 0);
    SourcePos start = parser_token_pos(value);
    if (value->filler.length > 0) {  // it starts at "the value": messages and carets show it whole
        size_t before = value->span.offset - value->filler.offset;  // "the" and spaces, on the same line
        start.span.offset -= before;
        start.span.length += before;
        start.column -= before;
    }
    Expr *lookup = ast_new_expr(p->arena, EXPR_MAP_GET, start);
    parser_consume(p, 2);  // value for
    if (parser_at_word(p, 0, "in") || parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"for\".")) {
            diag_note(p->diag, "Say which key, like \"%s\".", lookup_example);
        }
        return parser_error_expr(p);
    }
    lookup->as.lookup.key = parse_key(p);
    if (p->sentence_failed || !parser_expect_word(p, in, lookup_example)) return parser_error_expr(p);
    lookup->as.lookup.map = parse_primary(p);
    lookup->pos = ast_pos_join(lookup->pos, lookup->as.lookup.map->pos);
    if (p->sentence_failed || !parser_at(p, 0, TOK_COMMA) || !parser_at_word(p, 1, "or")) return lookup;
    parser_consume(p, 2);  // , or
    lookup->as.lookup.fallback = parse_unary(p);
    lookup->pos = ast_pos_join(lookup->pos, lookup->as.lookup.fallback->pos);
    return p->sentence_failed ? lookup : reject_ambiguous_fallback(p, lookup);
}

// --- Questions --------------------------------------------------------------------

// "the keys of M", "the values of M", "the number of entries in M": M is a single value.
static Expr *parse_map_prefix(Parser *p, size_t words, ListOp op) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op_span = parser_consume(p, words);
    Expr *operand = parse_primary(p);
    Expr *expr = ast_new_expr(p->arena, EXPR_LIST_OP, ast_pos_join(start, operand->pos));
    expr->as.list_op.op = op;
    expr->as.list_op.op_span = op_span;
    expr->as.list_op.operand = operand;
    return expr;
}

Expr *parser_parse_map_word(Parser *p) {
    if (parser_match_words(p, "map of")) return parse_map_literal(p);
    if (parser_match_words(p, "empty map")) return parse_empty_map(p);
    if (parser_at_lookup(p)) return parser_parse_lookup(p);
    if (parser_match_words(p, "keys of")) return parse_map_prefix(p, 2, LIST_KEYS);
    if (parser_match_words(p, "values of")) return parse_map_prefix(p, 2, LIST_VALUES);
    if (parser_match_words(p, "number of entries in")) return parse_map_prefix(p, 4, LIST_ENTRIES);
    return NULL;
}
