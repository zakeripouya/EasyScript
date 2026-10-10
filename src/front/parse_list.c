// Lists: writing them, naming their items, and the sentences that change
// them. Kinds of values ("a number", "a list of text") are here too, for
// function definitions and empty lists.
//
//   a list of 72, 95 and 88              an empty list of text
//   item 2 of scores / item 2 in scores  the first item in scores, the last item of scores
//   the number of items in scores        a copy of scores
//   the biggest of scores                the smallest of scores
//   items 2 to 4 of scores               items 3 to the end of scores
//   the first 3 items in scores          the last 2 items in scores
//   every second item of scores          every 3rd item of scores
//
//   add 5 to the end of scores           insert 7 at position 1 of scores
//   remove item 2 from scores            set item 2 of scores to 100
//   sort scores                          sort names by their length, longest first
//
// The forms that depend on precedence ("every", "only", "contains", "is
// empty", "added together", "reversed") are in parse_expr.c, and
// "for each score in scores:" is in parse_loop.c.

#include <stdlib.h>
#include <string.h>
#include "front/parse_internal.h"

static const char *const of_in[] = {"of", "in", NULL};

static bool word_among(const Token *token, const char *const *words) {
    if (token->kind != TOK_WORD) return false;
    for (size_t i = 0; words[i]; i++) {
        if (strcmp(token->text, words[i]) == 0) return true;
    }
    return false;
}

// --- Kinds of values -------------------------------------------------------------

static bool report_kind(Parser *p, const char *kinds, const char *missing) {
    const Token *word = parser_peek(p, 0);
    if (word->kind == TOK_COLON || word->kind == TOK_RPAREN || parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "%s", missing);
        }
    } else if (parser_error(p, word->span, "I don't know the kind of value %s.", parser_describe(p, word))) {
        diag_note(p->diag, "%s", kinds);
    }
    return false;
}

// What a list holds, in the plural: "numbers", "text", "yes/no values" (or
// "yes or no values"), or "lists of" ... .
static bool parse_items_kind(Parser *p, Type *type) {
    size_t words = 0;
    if (parser_at_word(p, 0, "numbers")) {
        *type = TYPE_NUMBER, words = 1;
    } else if (parser_at_word(p, 0, "text")) {
        *type = TYPE_TEXT, words = 1;
    } else if ((words = parser_match_words(p, "yes or no values")) > 0) {
        *type = TYPE_YESNO;
    } else if (parser_at_word(p, 0, "yes") && parser_at(p, 1, TOK_SLASH) && parser_at_word(p, 2, "no") &&
               parser_at_word(p, 3, "values")) {
        *type = TYPE_YESNO, words = 4;
    } else if (parser_match_words(p, "lists of")) {
        parser_consume(p, 2);
        if (!parse_items_kind(p, type)) return false;
        *type = type_list_of(*type);
        return true;
    }
    if (words > 0) {
        parser_consume(p, words);
        return true;
    }
    return report_kind(p, "A list holds \"numbers\", \"text\", \"yes/no values\", or \"lists of\" something, "
                          "like \"a list of numbers\".",
                       "Say what the list holds, like \"a list of numbers\".");
}

bool parser_parse_kind(Parser *p, bool nothing_ok, Type *type) {
    size_t words = 0;
    if (parser_match_words(p, "list of")) {
        parser_consume(p, 2);
        if (!parse_items_kind(p, type)) return false;
        *type = type_list_of(*type);
        return true;
    }
    if (parser_at_word(p, 0, "number")) {
        *type = TYPE_NUMBER, words = 1;
    } else if (parser_at_word(p, 0, "text")) {
        *type = TYPE_TEXT, words = 1;
    } else if ((words = parser_match_words(p, "yes or no")) > 0) {
        *type = TYPE_YESNO;
    } else if (parser_at_word(p, 0, "yes") && parser_at(p, 1, TOK_SLASH) && parser_at_word(p, 2, "no")) {
        *type = TYPE_YESNO, words = 3;
    } else if (nothing_ok && parser_at_word(p, 0, "nothing")) {
        *type = TYPE_NOTHING, words = 1;
    }
    if (words > 0) {
        parser_consume(p, words);
        return true;
    }
    return report_kind(p,
                       nothing_ok ? "The kinds are \"a number\", \"text\", \"yes or no\", and lists like \"a list of "
                                    "numbers\" (and \"nothing\" for a function that gives back no value)."
                                  : "The kinds are \"a number\", \"text\", \"yes or no\", and lists like \"a list of "
                                    "numbers\".",
                       "Say which kind of value: \"a number\", \"text\", \"yes or no\", or a list like \"a list of "
                       "numbers\".");
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

bool parser_at_item(const Parser *p, const char *const *after) {
    if ((parser_at_word(p, 0, "first") || parser_at_word(p, 0, "last")) && parser_at_word(p, 1, "item")) {
        return word_among(parser_peek(p, 2), after);
    }
    if (!parser_at_word(p, 0, "item")) return false;
    const Token *next = parser_peek(p, 1);
    if (next->kind == TOK_NUMBER || next->kind == TOK_STRING || next->kind == TOK_LPAREN) return true;
    // "item n of", or "item n plus 1 of" (reported); not "add item to total".
    const KnownName *known = parser_match_name(p, 1, false);
    size_t words = known ? known->words : 1;
    return is_position_word(next) && (word_among(parser_peek(p, 1 + words), after) || at_arithmetic(p, 1 + words));
}

// A position: a number, a name, or a value in parentheses. A name followed
// by "of" is the position, not a call ("item n of scores").
static Expr *parse_position(Parser *p) {
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
            diag_note(p->diag, "Write it like \"item (n plus 1) of scores\".");
        }
        return parser_error_expr(p);
    }
    return position;
}

// "item P", "first item" or "last item"; `after` are the words that may
// follow ("of"/"in", or "from").
static bool parse_item_ref(Parser *p, const char *const *after, ItemWhere *where, Expr **position) {
    *position = NULL;
    if (!parser_at_word(p, 0, "item")) {
        *where = parser_at_word(p, 0, "first") ? ITEM_FIRST : ITEM_LAST;
        parser_consume(p, 2);
        return true;
    }
    parser_advance(p);
    *where = ITEM_AT;
    if (word_among(parser_peek(p, 0), after) || parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"item\".")) {
            diag_note(p->diag, "Say which item, like \"item 1\", \"the first item\", or \"the last item\".");
        }
        return false;
    }
    *position = parse_position(p);
    return !p->sentence_failed;
}

// item P of L / the first item in L / the last item of L. The list is a
// single value, like "length of"'s.
static Expr *parse_item(Parser *p) {
    Expr *item = ast_new_expr(p->arena, EXPR_ITEM, parser_token_pos(parser_peek(p, 0)));
    if (!parse_item_ref(p, of_in, &item->as.item.where, &item->as.item.position) ||
        !parser_expect_word(p, of_in, "item 1 of scores")) {
        return parser_error_expr(p);
    }
    item->as.item.list = parse_primary(p);
    item->pos = ast_pos_join(item->pos, item->as.item.list->pos);
    return item;
}

// --- Slices ----------------------------------------------------------------------

// How many tokens a position at `at` takes (a number, a word, or a group in
// parentheses), or 0 if none starts there.
static size_t position_length(const Parser *p, size_t at) {
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
static bool at_range(const Parser *p) {
    size_t n = parser_at_word(p, 0, "items") ? position_length(p, 1) : 0;
    return n > 0 && (parser_at_word(p, 1 + n, "to") || at_arithmetic(p, 1 + n));
}

// "first N items" / "last N items" (a name followed by a value is never
// anything else, so arithmetic after N means a count that needs parentheses)
static bool at_first_last(const Parser *p) {
    if (!parser_at_word(p, 0, "first") && !parser_at_word(p, 0, "last")) return false;
    size_t n = position_length(p, 1);
    return n > 0 && (parser_at_word(p, 1 + n, "items") || at_arithmetic(p, 1 + n));  // arithmetic is reported
}

static Expr *new_slice(Parser *p, SliceKind kind) {
    Expr *slice = ast_new_expr(p->arena, EXPR_SLICE, parser_token_pos(parser_peek(p, 0)));
    slice->as.slice.kind = kind;
    return slice;
}

// "of L" / "in L" at the end of a slice.
static Expr *finish_slice(Parser *p, Expr *slice, const char *example) {
    if (p->sentence_failed || !parser_expect_word(p, of_in, example)) return parser_error_expr(p);
    slice->as.slice.list = parse_primary(p);
    slice->pos = ast_pos_join(slice->pos, slice->as.slice.list->pos);
    return slice;
}

// items P to Q of L / items P to the end of L
static Expr *parse_range(Parser *p) {
    static const char *const to[] = {"to", NULL};
    static const char *const example = "items 2 to 4 of scores";
    Expr *slice = new_slice(p, SLICE_RANGE);
    parser_advance(p);  // items
    slice->as.slice.from = parse_position(p);
    if (p->sentence_failed || !parser_expect_word(p, to, example)) return parser_error_expr(p);
    if (word_among(parser_peek(p, 0), of_in)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"to\".")) {
            diag_note(p->diag, "Say where the items end, like \"%s\" or \"items 2 to the end of scores\".", example);
        }
        return parser_error_expr(p);
    }
    if (parser_match_words(p, "end of") || parser_match_words(p, "end in")) {
        parser_advance(p);  // end ("the end"): the slice goes to the last item
    } else {
        slice->as.slice.to = parse_position(p);
    }
    return finish_slice(p, slice, example);
}

// the first N items in L / the last N items of L
static Expr *parse_first_last(Parser *p) {
    static const char *const items[] = {"items", NULL};
    bool first = parser_at_word(p, 0, "first");
    Expr *slice = new_slice(p, first ? SLICE_FIRST : SLICE_LAST);
    parser_advance(p);
    slice->as.slice.from = parse_position(p);
    if (p->sentence_failed || !parser_expect_word(p, items, "the first 3 items in scores")) return parser_error_expr(p);
    return finish_slice(p, slice, first ? "the first 3 items in scores" : "the last 2 items in scores");
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

// "every second item", "every 3rd item" (then "of"/"in", or an error)
static bool at_every_nth(const Parser *p) {
    const Token *ordinal = parser_peek(p, 1);
    return parser_at_word(p, 0, "every") && (ordinal->kind == TOK_ORDINAL || ordinal_word(ordinal) > 0) &&
           parser_at_word(p, 2, "item");
}

// every second item of L: the items at positions 2, 4, 6, ...
static Expr *parse_every_nth(Parser *p) {
    Expr *slice = new_slice(p, SLICE_EVERY);
    parser_advance(p);  // every
    const Token *ordinal = parser_advance(p);
    slice->as.slice.step = ordinal->kind == TOK_ORDINAL ? strtoull(ordinal->text, NULL, 10) : ordinal_word(ordinal);
    if (slice->as.slice.step == 0) {
        if (parser_error(p, ordinal->span, "There's no item at position 0.")) {
            diag_note(p->diag, "Positions start at 1: \"every 2nd item\" means items 2, 4, 6 and so on.");
        }
        return parser_error_expr(p);
    }
    parser_advance(p);  // item
    return finish_slice(p, slice, "every second item of scores");
}

// "item" followed by "of" or "in": which item is missing.
static Expr *report_missing_position(Parser *p) {
    Span span = parser_advance(p)->span;
    if (parser_error(p, span, "Something is missing after \"item\".")) {
        diag_note(p->diag, "Say which item, like \"item 1\", \"the first item\", or \"the last item\".");
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
    if (!parse_items_kind(p, &item)) return parser_error_expr(p);
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
    if (at_range(p)) return parse_range(p);
    if (at_first_last(p)) return parse_first_last(p);
    if (at_every_nth(p)) return parse_every_nth(p);
    if (parser_at_item(p, of_in)) return parse_item(p);
    if (parser_at_word(p, 0, "item") && word_among(parser_peek(p, 1), of_in)) return report_missing_position(p);
    if (parser_match_words(p, "number of items in")) return parse_prefix(p, 4, EXPR_LENGTH, LIST_SUM);
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
    if (!parse_item_ref(p, of_in, &stmt->as.list.where, &stmt->as.list.position) ||
        !parser_expect_word(p, of_in, example)) {
        return NULL;
    }
    stmt->as.list.list = parse_primary(p);
    if (p->sentence_failed || !parser_expect_word(p, to, example)) return NULL;
    stmt->as.list.value = parse_expression(p);
    return stmt;
}

// remove item P from L / remove the first item from L / remove the last item from L
Stmt *parser_parse_remove(Parser *p, const Token *verb) {
    static const char *const from[] = {"from", NULL};
    static const char *const item[] = {"item", NULL};
    static const char *const example = "remove item 2 from scores";
    Stmt *stmt = parser_new_stmt(p, STMT_LIST, verb);
    stmt->as.list.op = LIST_REMOVE;
    bool first_last = (parser_at_word(p, 0, "first") || parser_at_word(p, 0, "last")) && parser_at_word(p, 1, "item");
    if (!first_last && !parser_at_word(p, 0, "item")) {
        parser_expect_word(p, item, example);
        return NULL;
    }
    if (!parse_item_ref(p, from, &stmt->as.list.where, &stmt->as.list.position) ||
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
    stmt->as.list.position = parse_position(p);
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
