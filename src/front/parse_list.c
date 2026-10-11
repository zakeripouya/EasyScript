// Lists: writing them, naming their items, and the sentences that change
// them. Kinds of values ("a number", "a list of text") are in parse_kind.c.
//
//   a list of 72, 95 and 88              an empty list of text
//   item 2 of scores / item 2 in scores  the first item in scores, the last item of scores
//   the number of items in scores        a copy of scores
//   the biggest of scores                the smallest of scores
//   items 2 to 4 of scores               items 3 to the end of scores
//   the first 3 items in scores          the last 2 items in scores
//   every second item of scores          every 3rd item of scores
//   every other item of scores           (items 1, 3, 5; "every second item" is 2, 4, 6)
//
// Text has the same forms with "character" for "item" (character 2 of word,
// the first 3 characters of word, every other character of word, ...): they
// make EXPR_ITEM and EXPR_SLICE nodes marked `characters`.
//
//   add 5 to the end of scores           insert 7 at position 1 of scores
//   remove item 2 from scores            set item 2 of scores to 100   (remove K from M: a map)
//   sort scores                          sort names by their length, longest first
//
// The forms that depend on precedence ("every", "only", "contains", "is
// empty", "added together", "reversed") are in parse_expr.c, and
// "for each score in scores:" is in parse_loop.c.

#include <stdlib.h>
#include <string.h>
#include "front/parse_internal.h"

static const char *const of_in[] = {"of", "in", NULL};

// How an item form names what it takes: "item"/"items" from a list,
// "character"/"characters" from text.
typedef struct {
    const char *one;
    const char *many;
    bool characters;
} Noun;

static const Noun item_noun = {"item", "items", false};
static const Noun character_noun = {"character", "characters", true};

static bool word_among(const Token *token, const char *const *words) {
    if (token->kind != TOK_WORD) return false;
    for (size_t i = 0; words[i]; i++) {
        if (strcmp(token->text, words[i]) == 0) return true;
    }
    return false;
}

// --- Items -----------------------------------------------------------------------

static bool at_arithmetic(const Parser *p, size_t ahead) {
    static const char *const words[] = {"plus", "minus", "times", "multiplied", "divided", "mod", "followed", NULL};
    TokenKind kind = parser_peek(p, ahead)->kind;
    return kind == TOK_PLUS || kind == TOK_MINUS || kind == TOK_STAR || kind == TOK_SLASH || kind == TOK_PERCENT ||
           word_among(parser_peek(p, ahead), words);
}

// A word that can be a position on its own: a name, or "it". (A name of
// several words is a position too; see position_words.)
static bool is_position_word(const Token *token) {
    if (token->kind != TOK_WORD || parser_is_operator_word(token->text)) return false;
    return strcmp(token->text, "it") == 0 || !parser_is_value_word(token->text);
}

static bool at_element(const Parser *p, const Noun *noun, const char *const *after) {
    if ((parser_at_word(p, 0, "first") || parser_at_word(p, 0, "last")) && parser_at_word(p, 1, noun->one)) {
        return word_among(parser_peek(p, 2), after);
    }
    if (!parser_at_word(p, 0, noun->one)) return false;
    const Token *next = parser_peek(p, 1);
    if (next->kind == TOK_NUMBER || next->kind == TOK_STRING || next->kind == TOK_LPAREN) return true;
    // "item n of", or "item n plus 1 of" (reported); not "add item to total".
    const KnownName *known = parser_match_name(p, 1, false);
    size_t words = known ? known->words : 1;
    return is_position_word(next) && (word_among(parser_peek(p, 1 + words), after) || at_arithmetic(p, 1 + words));
}

bool parser_at_item(const Parser *p, const char *const *after) {
    return at_element(p, &item_noun, after);
}

// A position: a number, a name, or a value in parentheses. A name followed
// by "of" is the position, not a call ("item n of scores").
static Expr *parse_position(Parser *p, const Noun *noun) {
    const Token *token = parser_peek(p, 0);
    Expr *position;
    if (parser_at_word(p, 0, "it")) {
        position = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(parser_advance(p)));
    } else if (is_position_word(token)) {
        const KnownName *known = parser_match_name(p, 0, false);
        Name name;
        parser_take_name(p, known ? known->words : 1, &name);
        position = ast_new_expr(p->arena, EXPR_NAME, name.pos);
        position->as.name = name.text;
        position->display = name.display;
    } else {
        position = parse_primary(p);
    }
    if (!p->sentence_failed && at_arithmetic(p, 0)) {
        if (parser_error(p, parser_peek(p, 0)->span, "A position worked out with arithmetic needs parentheses.")) {
            diag_note(p->diag, "Write it like \"%s\".", noun->characters ? "character (n plus 1) of word" : "item (n plus 1) of scores");
        }
        return parser_error_expr(p);
    }
    return position;
}

// L without item P / T without character P: a new list or text.
Expr *parser_parse_without_at(Parser *p, Expr *list) {
    const Noun *noun = parser_at_word(p, 1, "character") ? &character_noun : &item_noun;
    parser_consume(p, 2);
    if (parser_at_sentence_end(p) || parser_at(p, 0, TOK_COLON) || parser_at(p, 0, TOK_COMMA)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"%s\".", noun->one)) {
            diag_note(p->diag, "Say which %s to leave out, like \"%s\".", noun->one,
                      noun->characters ? "word without character 2" : "scores without item 2");
        }
        return parser_error_expr(p);
    }
    Expr *slice = ast_new_expr(p->arena, EXPR_SLICE, list->pos);
    slice->as.slice.kind = SLICE_WITHOUT_AT;
    slice->as.slice.characters = noun->characters;
    slice->as.slice.list = list;
    slice->as.slice.from = parse_position(p, noun);
    slice->pos = ast_pos_join(list->pos, slice->as.slice.from->pos);
    return slice;
}

// "item P", "first item" or "last item"; `after` are the words that may
// follow ("of"/"in", or "from").
static bool parse_item_ref(Parser *p, const Noun *noun, const char *const *after, ItemWhere *where, Expr **position) {
    *position = NULL;
    if (!parser_at_word(p, 0, noun->one)) {
        *where = parser_at_word(p, 0, "first") ? ITEM_FIRST : ITEM_LAST;
        parser_consume(p, 2);
        return true;
    }
    parser_advance(p);
    *where = ITEM_AT;
    if (word_among(parser_peek(p, 0), after) || parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"%s\".", noun->one)) {
            diag_note(p->diag, "Say which %s, like \"%s 1\", \"the first %s\", or \"the last %s\".", noun->one,
                      noun->one, noun->one, noun->one);
        }
        return false;
    }
    *position = parse_position(p, noun);
    return !p->sentence_failed;
}

// item P of L / the first item in L / the last item of L (or character P
// of T ...). The list is a single value, like "length of"'s.
static Expr *parse_item(Parser *p, const Noun *noun) {
    Expr *item = ast_new_expr(p->arena, EXPR_ITEM, parser_token_pos(parser_peek(p, 0)));
    item->as.item.characters = noun->characters;
    if (!parse_item_ref(p, noun, of_in, &item->as.item.where, &item->as.item.position) ||
        !parser_expect_word(p, of_in, noun->characters ? "character 1 of word" : "item 1 of scores")) {
        return parser_error_expr(p);
    }
    item->as.item.list = parse_primary(p);
    item->pos = ast_pos_join(item->pos, item->as.item.list->pos);
    return item;
}

// --- Slices ----------------------------------------------------------------------

size_t parser_value_length(const Parser *p, size_t at) {
    const Token *token = parser_peek(p, at);
    if (token->kind == TOK_NUMBER || token->kind == TOK_STRING) return 1;
    if (is_position_word(token)) {
        const KnownName *known = parser_match_name(p, at, false);
        return known ? known->words : 1;
    }
    if (token->kind != TOK_LPAREN) return 0;
    size_t depth = 0;
    for (size_t i = at;; i++) {
        TokenKind kind = parser_peek(p, i)->kind;
        if (kind == TOK_NEWLINE || kind == TOK_EOF) return 0;
        if (kind == TOK_LPAREN) depth++;
        if (kind == TOK_RPAREN && --depth == 0) return i - at + 1;
    }
}

// "items P to": a range starts here (arithmetic after P is reported).
static bool at_range(const Parser *p, const Noun *noun) {
    size_t n = parser_at_word(p, 0, noun->many) ? parser_value_length(p, 1) : 0;
    return n > 0 && (parser_at_word(p, 1 + n, "to") || at_arithmetic(p, 1 + n));
}

// "first N items" / "last N items" (a name followed by a value is never
// anything else, so arithmetic after N means a count that needs parentheses)
static bool at_first_last(const Parser *p, const Noun **noun) {
    if (!parser_at_word(p, 0, "first") && !parser_at_word(p, 0, "last")) return false;
    size_t n = parser_value_length(p, 1);
    if (n == 0) return false;
    // The noun after the count (past any arithmetic, which is reported) says what's taken.
    size_t at = 1 + n;
    while (parser_peek(p, at)->kind != TOK_NEWLINE && parser_peek(p, at)->kind != TOK_EOF &&
           !parser_at_word(p, at, "items") && !parser_at_word(p, at, "characters")) {
        at++;
    }
    *noun = parser_at_word(p, at, "characters") ? &character_noun : &item_noun;
    return parser_at_word(p, 1 + n, (*noun)->many) || at_arithmetic(p, 1 + n);  // arithmetic is reported
}

static Expr *new_slice(Parser *p, SliceKind kind, const Noun *noun) {
    Expr *slice = ast_new_expr(p->arena, EXPR_SLICE, parser_token_pos(parser_peek(p, 0)));
    slice->as.slice.kind = kind;
    slice->as.slice.characters = noun->characters;
    return slice;
}

// "of L" / "in L" at the end of a slice.
static Expr *finish_slice(Parser *p, Expr *slice, const char *example) {
    if (p->sentence_failed || !parser_expect_word(p, of_in, example)) return parser_error_expr(p);
    slice->as.slice.list = parse_primary(p);
    slice->pos = ast_pos_join(slice->pos, slice->as.slice.list->pos);
    return slice;
}

// items P to Q of L / items P to the end of L (or characters P to Q of T ...)
static Expr *parse_range(Parser *p, const Noun *noun) {
    static const char *const to[] = {"to", NULL};
    const char *example = noun->characters ? "characters 2 to 4 of word" : "items 2 to 4 of scores";
    Expr *slice = new_slice(p, SLICE_RANGE, noun);
    parser_advance(p);  // items
    slice->as.slice.from = parse_position(p, noun);
    if (p->sentence_failed || !parser_expect_word(p, to, example)) return parser_error_expr(p);
    if (word_among(parser_peek(p, 0), of_in)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"to\".")) {
            diag_note(p->diag, "Say where the %s end, like \"%s\" or \"%s\".", noun->many, example,
                      noun->characters ? "characters 2 to the end of word" : "items 2 to the end of scores");
        }
        return parser_error_expr(p);
    }
    if (parser_match_words(p, "end of") || parser_match_words(p, "end in")) {
        parser_advance(p);  // end ("the end"): the slice goes to the last item
    } else {
        slice->as.slice.to = parse_position(p, noun);
    }
    return finish_slice(p, slice, example);
}

// the first N items in L / the last N items of L (or characters of T)
static Expr *parse_first_last(Parser *p, const Noun *noun) {
    const char *const many[] = {noun->many, NULL};
    bool first = parser_at_word(p, 0, "first");
    Expr *slice = new_slice(p, first ? SLICE_FIRST : SLICE_LAST, noun);
    parser_advance(p);
    slice->as.slice.from = parse_position(p, noun);
    const char *example = noun->characters ? (first ? "the first 3 characters of word" : "the last 2 characters of word")
                                           : (first ? "the first 3 items in scores" : "the last 2 items in scores");
    if (p->sentence_failed || !parser_expect_word(p, many, example)) return parser_error_expr(p);
    return finish_slice(p, slice, example);
}

// "second" ... "tenth" as numbers (2 to 10), or 0 for any other word.
static size_t ordinal_word(const Token *token) {
    static const char *const words[] = {"second", "third", "fourth", "fifth", "sixth", "seventh", "eighth",
                                        "ninth", "tenth", NULL};
    for (size_t i = 0; token->kind == TOK_WORD && words[i]; i++) {
        if (strcmp(token->text, words[i]) == 0) return i + 2;
    }
    return 0;
}

// "every second item", "every 3rd item", "every other item" (then "of"/"in",
// or an error), and the same with "character". It's never "every NAME in"
// with a name like "other item".
bool parser_at_every_nth(const Parser *p) {
    const Token *ordinal = parser_peek(p, 1);
    return parser_at_word(p, 0, "every") &&
           (ordinal->kind == TOK_ORDINAL || ordinal_word(ordinal) > 0 || parser_at_word(p, 1, "other")) &&
           (parser_at_word(p, 2, "item") || parser_at_word(p, 2, "character"));
}

// every second item of L: the items at positions 2, 4, 6, ...
// every other item of L: the items at positions 1, 3, 5, ...
static Expr *parse_every_nth(Parser *p) {
    const Noun *noun = parser_at_word(p, 2, "character") ? &character_noun : &item_noun;
    Expr *slice = new_slice(p, SLICE_EVERY, noun);
    parser_advance(p);  // every
    const Token *ordinal = parser_advance(p);
    bool other = ordinal->kind == TOK_WORD && strcmp(ordinal->text, "other") == 0;
    slice->as.slice.step = other ? 2 : ordinal->kind == TOK_ORDINAL ? strtoull(ordinal->text, NULL, 10) : ordinal_word(ordinal);
    slice->as.slice.start = other ? 1 : slice->as.slice.step;
    if (slice->as.slice.step == 0) {
        if (parser_error(p, ordinal->span, "There's no %s at position 0.", noun->one)) {
            diag_note(p->diag, "Positions start at 1: \"every 2nd %s\" means %s 2, 4, 6 and so on.", noun->one,
                      noun->many);
        }
        return parser_error_expr(p);
    }
    parser_advance(p);  // item
    return finish_slice(p, slice, noun->characters ? "every second character of word" : "every second item of scores");
}

// "item" followed by "of" or "in": which item is missing.
static Expr *report_missing_position(Parser *p, const Noun *noun) {
    Span span = parser_advance(p)->span;
    if (parser_error(p, span, "Something is missing after \"%s\".", noun->one)) {
        diag_note(p->diag, "Say which %s, like \"%s 1\", \"the first %s\", or \"the last %s\".", noun->one,
                  noun->one, noun->one, noun->one);
    }
    return parser_error_expr(p);
}

// --- Writing lists -----------------------------------------------------------------

// One item of "a list of ...": a value up to the next "and" or ",". A list
// written inside it needs parentheses, or its "and"s would be ambiguous.
static Expr *parse_list_item(Parser *p) {
    if (parser_match_words(p, "list of")) {
        Span span = parser_consume(p, 2);
        if (parser_error(p, span, "A list inside a list needs parentheses.")) {
            diag_note(p->diag, "Write it like \"a list of (a list of 1 and 2) and (a list of 3 and 4)\".");
        }
        return parser_error_expr(p);
    }
    return parse_join(p);
}

// list of A, B and C: commas separate items until the first "and"; after
// that, only "and" does, so "if x is a list of 1 and 2, say ..." keeps its comma.
static Expr *parse_list_literal(Parser *p) {
    Expr *list = ast_new_expr(p->arena, EXPR_LIST, parser_token_pos(parser_peek(p, 0)));
    parser_consume(p, 2);  // list of
    bool after_and = false;
    for (;;) {
        Expr *item = parse_list_item(p);
        vec_push(p->arena, &list->as.items, item);
        list->pos = ast_pos_join(list->pos, item->pos);
        if (parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call")) {
            after_and = true;
        } else if (after_and || !parser_at(p, 0, TOK_COMMA)) {
            return list;
        }
        parser_advance(p);
    }
}

static Expr *parse_empty_list(Parser *p) {
    Expr *list = ast_new_expr(p->arena, EXPR_EMPTY_LIST, parser_token_pos(parser_peek(p, 0)));
    parser_consume(p, 3);  // empty list of
    Type item;
    if (!parser_parse_items_kind(p, &item)) return parser_error_expr(p);
    list->as.list_type = type_list_of(item);
    list->pos = ast_pos_join(list->pos, (SourcePos){p->prev_span, 0, 0});
    return list;
}

// A prefix form that takes a single value: "the number of items in L", "a
// copy of L", "the biggest of L".
static Expr *parse_prefix(Parser *p, size_t words, ExprKind kind, ListOp op) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op_span = parser_consume(p, words);
    Expr *operand = parse_primary(p);
    Expr *expr = ast_new_expr(p->arena, kind, ast_pos_join(start, operand->pos));
    if (kind == EXPR_LENGTH) {
        expr->as.operand = operand;
    } else {
        expr->as.list_op.op = op;
        expr->as.list_op.op_span = op_span;
        expr->as.list_op.operand = operand;
    }
    return expr;
}

Expr *parser_parse_list_word(Parser *p) {
    if (parser_match_words(p, "list of")) return parse_list_literal(p);
    if (parser_match_words(p, "empty list of")) return parse_empty_list(p);
    const Noun *noun = NULL;
    if (at_range(p, &item_noun)) return parse_range(p, &item_noun);
    if (at_range(p, &character_noun)) return parse_range(p, &character_noun);
    if (at_first_last(p, &noun)) return parse_first_last(p, noun);
    if (parser_at_every_nth(p)) return parse_every_nth(p);
    if (at_element(p, &item_noun, of_in)) return parse_item(p, &item_noun);
    if (at_element(p, &character_noun, of_in)) return parse_item(p, &character_noun);
    if (parser_at_word(p, 0, "item") && word_among(parser_peek(p, 1), of_in)) return report_missing_position(p, &item_noun);
    if (parser_at_word(p, 0, "character") && word_among(parser_peek(p, 1), of_in)) {
        return report_missing_position(p, &character_noun);
    }
    if (parser_match_words(p, "number of items in")) return parse_prefix(p, 4, EXPR_LENGTH, LIST_SUM);
    if (parser_match_words(p, "number of characters in")) return parse_prefix(p, 4, EXPR_LENGTH, LIST_SUM);
    if (parser_match_words(p, "copy of")) return parse_prefix(p, 2, EXPR_LIST_OP, LIST_COPY);
    if (parser_match_words(p, "biggest of")) return parse_prefix(p, 2, EXPR_LIST_OP, LIST_BIGGEST);
    if (parser_match_words(p, "smallest of")) return parse_prefix(p, 2, EXPR_LIST_OP, LIST_SMALLEST);
    return NULL;
}

// --- Changing lists ---------------------------------------------------------------

Stmt *parser_finish_add_end(Parser *p, Stmt *stmt, Expr *value) {
    parser_consume(p, 2);  // end of
    stmt->kind = STMT_LIST;
    memset(&stmt->as, 0, sizeof stmt->as);
    stmt->as.list.op = LIST_ADD_END;
    stmt->as.list.value = value;
    stmt->as.list.list = parse_primary(p);
    return stmt;
}

// set item P of L to E (after "set" or "change")
Stmt *parser_parse_set_item(Parser *p, const Token *verb) {
    static const char *const to[] = {"to", NULL};
    static const char *const example = "set item 2 of scores to 100";
    Stmt *stmt = parser_new_stmt(p, STMT_LIST, verb);
    stmt->as.list.op = LIST_SET_ITEM;
    if (!parse_item_ref(p, &item_noun, of_in, &stmt->as.list.where, &stmt->as.list.position) ||
        !parser_expect_word(p, of_in, example)) {
        return NULL;
    }
    stmt->as.list.list = parse_primary(p);
    if (p->sentence_failed || !parser_expect_word(p, to, example)) return NULL;
    stmt->as.list.value = parse_expression(p);
    return stmt;
}

// remove K from M (M a map): K is a value up to "from".
static Stmt *parse_remove_key(Parser *p, Stmt *stmt) {
    static const char *const from[] = {"from", NULL};
    stmt->as.list.op = LIST_REMOVE_KEY;
    if (parser_at_word(p, 0, "from") || parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"remove\".")) {
            diag_note(p->diag, "Say what to remove, like \"remove item 2 from scores\" for a list or \"remove \"tea\" "
                               "from prices\" for a map.");
        }
        return NULL;
    }
    stmt->as.list.value = parse_join(p);
    if (p->sentence_failed || !parser_expect_word(p, from, "remove \"tea\" from prices")) return NULL;
    stmt->as.list.list = parse_primary(p);
    return stmt;
}

// remove item P from L / remove the first item from L / remove the last item from L,
// or remove K from M for a map ("item" followed by "from" is a missing position).
Stmt *parser_parse_remove(Parser *p, const Token *verb) {
    static const char *const from[] = {"from", NULL};
    static const char *const example = "remove item 2 from scores";
    Stmt *stmt = parser_new_stmt(p, STMT_LIST, verb);
    stmt->as.list.op = LIST_REMOVE;
    bool first_last = (parser_at_word(p, 0, "first") || parser_at_word(p, 0, "last")) && parser_at_word(p, 1, "item");
    bool item = parser_at_word(p, 0, "item") && (parser_at_word(p, 1, "from") || parser_at_item(p, from));
    if (!first_last && !item) return parse_remove_key(p, stmt);
    if (!parse_item_ref(p, &item_noun, from, &stmt->as.list.where, &stmt->as.list.position) ||
        !parser_expect_word(p, from, example)) {
        return NULL;
    }
    stmt->as.list.list = parse_primary(p);
    return stmt;
}

// insert E at position P of L
Stmt *parser_parse_insert(Parser *p, const Token *verb) {
    static const char *const example = "insert 7 at position 1 of scores";
    Stmt *stmt = parser_new_stmt(p, STMT_LIST, verb);
    stmt->as.list.op = LIST_INSERT;
    if (parser_match_words(p, "at position")) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "Add a value there, like a number, some text in quotes, or a name.");
        }
        return NULL;
    }
    stmt->as.list.value = parse_expression(p);
    if (p->sentence_failed || !parser_expect_phrase(p, "at position", "at position", example)) return NULL;
    stmt->as.list.position = parse_position(p, &item_noun);
    if (p->sentence_failed || !parser_expect_word(p, of_in, example)) return NULL;
    stmt->as.list.list = parse_primary(p);
    return stmt;
}

// sort L [by their length] [,] [smallest | biggest | shortest | longest first]
Stmt *parser_parse_sort(Parser *p, const Token *verb) {
    static const char *const order[] = {"smallest", "biggest", "shortest", "longest", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_LIST, verb);
    stmt->as.list.op = LIST_SORT;
    stmt->as.list.list = parse_primary(p);
    if (p->sentence_failed) return NULL;
    if (parser_match_words(p, "by their length")) {
        parser_consume(p, 3);
        stmt->as.list.by_length = true;
    } else if (parser_at_word(p, 0, "by")) {  // "sort names by length of": a function
        parser_advance(p);
        return parser_finish_sort_by(p, stmt);
    }
    size_t comma = parser_at(p, 0, TOK_COMMA) ? 1 : 0;
    const Token *word = parser_peek(p, comma);
    if (!word_among(word, order) || !parser_at_word(p, comma + 1, "first")) return stmt;
    parser_consume(p, comma);
    Span span = parser_consume(p, 2);
    bool by_length = word->text[0] == 's' ? word->text[1] == 'h' : word->text[0] == 'l';
    if (by_length != stmt->as.list.by_length) {
        if (stmt->as.list.by_length) {
            parser_error(p, span, "With \"by their length\", say \"longest first\" or \"shortest first\".");
        } else if (parser_error(p, span, "\"%s first\" sorts by length, so it needs \"by their length\".",
                                word->text)) {
            diag_note(p->diag, "Write it like \"sort names by their length, %s first\".", word->text);
        }
        return NULL;
    }
    stmt->as.list.descending = strcmp(word->text, "biggest") == 0 || strcmp(word->text, "longest") == 0;
    return stmt;
}
