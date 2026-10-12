// Questions about a list, each an EXPR_AGGREGATE worked out in one pass:
//
//   how many L are|is TEST, how many L there are, the number of L [per FIELD]
//   any|every|no ITEM TEST, any|no ITEM in L TEST, all L TEST, any|all|none of L TEST
//   the total|average|median [FIELD] of L [per FIELD]
//   the top|lowest N L [by FIELD], the N most common L
//   the ITEM [in L] with the highest|lowest FIELD (or most, fewest, ...)
//
// L is a single value with its filters ("the dogs whose age is over 3"). In
// the test, key and group, "it" is the item. A word here that's also the name
// of a function the program defines keeps meaning the function.

#include <string.h>
#include "front/parse_internal.h"

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

static Expr *new_aggregate(Parser *p, AggregateKind kind, SourcePos start, Span op_span) {
    Expr *expr = ast_new_expr(p->arena, EXPR_AGGREGATE, start);
    expr->as.aggregate.kind = kind;
    expr->as.aggregate.op_span = op_span;
    return expr;
}

static Expr *finish(Expr *expr, const Expr *last) {
    if (last) expr->pos = ast_pos_join(expr->pos, last->pos);
    return expr;
}

// The list and its filters: "dogs whose age is over 3".
static Expr *parse_list_part(Parser *p) {
    return parser_parse_filters(p, parse_primary(p));
}

// The item's field, at its first word: "amount" in "the total amount of orders".
static Expr *parse_item_field(Parser *p, size_t words) {
    Expr *it = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(parser_peek(p, 0)));
    Expr *field = ast_new_expr(p->arena, EXPR_FIELD, it->pos);
    field->as.field.record = it;
    parser_take_name(p, words, &field->as.field.name);
    field->pos = field->as.field.name.pos;
    return field;
}

// "per region" / "by amount": a field of the item after `word`, or an error showing `example`.
static Expr *parse_field_after(Parser *p, const char *word, const char *example) {
    Span at = parser_advance(p)->span;
    size_t words = parser_field_words(p, 0);
    if (words == 0) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->kind == TOK_WORD ? token->span : at,
                         "I expected a field after \"%s\" here, but found %s.", word, parser_describe(p, token))) {
            diag_note(p->diag, "Write it like \"%s\".", example);
            diag_info(p->diag, DIAG_MISSING, arena_sprintf(p->arena, "a field after \"%s\"", word), NULL);
            for (size_t i = 0; i < p->fields.len; i++) diag_choice(p->diag, "%s %s", word, p->fields.items[i].text);
        }
        return NULL;
    }
    return parse_item_field(p, words);
}

// "per month", "per year of signup": grouped by a part of the item's date (the
// field named after "of", or its one date field, found by the checker).
static Expr *parse_period(Parser *p) {
    static const struct {
        const char *word;
        DatePart part;
    } periods[] = {{"month", DATE_MONTH}, {"year", DATE_YEAR}, {"day", DATE_DAY}, {"weekday", DATE_WEEKDAY}};
    for (size_t i = 0; i < COUNT(periods); i++) {
        if (!parser_at_word(p, 1, periods[i].word)) continue;
        const Token *per = parser_advance(p);
        Span word = parser_advance(p)->span;
        Expr *date;
        if (parser_at_word(p, 0, "of") && parser_field_words(p, 1) > 0) {
            parser_advance(p);
            date = parse_item_field(p, parser_field_words(p, 0));
        } else {
            date = ast_new_expr(p->arena, EXPR_FIELD, parser_token_pos(&p->tokens[p->pos - 1]));
            date->as.field.record = ast_new_expr(p->arena, EXPR_IT, date->pos);
            date->as.field.name.pos = date->pos;
        }
        Expr *group = ast_new_expr(p->arena, EXPR_LIBRARY, ast_pos_join(parser_token_pos(per), date->pos));
        group->as.library.op = LIB_PERIOD_KEY;
        group->as.library.op_span = word;
        group->as.library.a = date;
        group->as.library.part = (int)periods[i].part;
        return group;
    }
    return NULL;
}

static bool missing_after(Parser *p, Span op_span, const char *example) {
    if (!parser_at_sentence_end(p) && !parser_at(p, 0, TOK_COMMA) && !parser_at(p, 0, TOK_COLON)) return false;
    if (parser_error(p, op_span, "Something is missing after \"%s\".", parser_text(p, op_span))) {
        diag_note(p->diag, "Write it like \"%s\".", example);
        diag_info(p->diag, DIAG_MISSING, "a list", "a list");
    }
    return true;
}

// The word at `ahead` names a function the program defines: it keeps that
// meaning. Written with "the" ("the total of scores"), it could mean either,
// so that's an error showing both.
static bool is_function_word(Parser *p, size_t ahead, const char *instead) {
    const Token *token = parser_peek(p, ahead);
    if (!parser_is_function(p, token->text)) return false;
    if (token->filler.length == 0 || !instead) return true;
    if (parser_error(p, token->span, "\"the %s\" could mean two things here.", token->text)) {
        diag_note(p->diag, "\"%s\" is also a definition in this program. To use the definition, write \"%s of ...\" "
                           "without \"the\". %s",
                  token->text, token->text, instead);
        diag_info(p->diag, DIAG_TWO_MEANINGS, arena_sprintf(p->arena, "which \"%s\" is meant", token->text), NULL);
        diag_choice(p->diag, "%s of ...", token->text);
    }
    return true;
}

// --- how many ------------------------------------------------------------------

static bool at_singular_test(const Parser *p) {
    return parser_at_word(p, 0, "is") || parser_at_word(p, 0, "isn't");
}

// how many L are|is TEST / how many L there are / how many L whose ...
static Expr *parse_how_many(Parser *p) {
    static const char *const example = "how many scores are above 70";
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op = parser_consume(p, 2);
    if (parser_at_word(p, 0, "of")) parser_advance(p);  // how many of the scores ...
    Expr *count = new_aggregate(p, AGG_COUNT, start, op);
    if (missing_after(p, op, example)) return parser_error_expr(p);
    Expr *list = parse_list_part(p);
    count->as.aggregate.list = list;
    if (p->sentence_failed) return parser_error_expr(p);
    size_t there = parser_match_words(p, "there are");
    if (!there) there = parser_match_words(p, "there is");
    if (there) {
        count->pos = ast_pos_join(count->pos, (SourcePos){parser_consume(p, there), 0, 0, 0});
        return count;
    }
    if (at_singular_test(p) || parser_at_item_test(p, 0)) {
        count->as.aggregate.condition = parser_parse_item_test(p, example);
        return finish(count, count->as.aggregate.condition);
    }
    if (list->kind == EXPR_ONLY) return finish(count, list);  // how many dogs whose age is over 3
    const Token *token = parser_peek(p, 0);
    if (parser_error(p, token->span, "I expected \"there are\" or a test like \"are above 70\" here, but found %s.",
                     parser_describe(p, token))) {
        diag_note(p->diag, "Write it like \"%s\" or \"how many scores there are\".", example);
        diag_info(p->diag, DIAG_MISSING, "what to count", NULL);
        diag_choice(p->diag, "there are");
        diag_choice(p->diag, "are above 70");
    }
    return parser_error_expr(p);
}

// --- any, every, no, all, none -----------------------------------------------------

// A word that starts the test after an item: "is", "'s", "contains", ...
static bool at_test_word(const Parser *p, size_t ahead) {
    static const char *const words[] = {"is", "isn't", "contains", "does", "doesn't", "starts", "ends", "has",
                                        "equals", "reaches"};
    const Token *token = parser_peek(p, ahead);
    switch (token->kind) {
    case TOK_POSSESSIVE:
    case TOK_EQUAL:
    case TOK_EQUAL_EQUAL:
    case TOK_NOT_EQUAL:
    case TOK_LESS:
    case TOK_GREATER:
    case TOK_LESS_EQUAL:
    case TOK_GREATER_EQUAL: return true;
    case TOK_WORD:
        for (size_t i = 0; i < COUNT(words); i++) {
            if (strcmp(token->text, words[i]) == 0) return true;
        }
        return false;
    default: return false;
    }
}

// After "any", "every" or "no": how many words name the item before its test
// ("any score is ...", "every big order's ...") or before "in" ("any score in
// results is ..."), with *in set for the second; 0 if neither follows.
static size_t item_words(const Parser *p, bool *in) {
    size_t n = 0;
    *in = false;
    if (parser_at_word(p, 1, "of")) return 0;  // "any of scores are ..."
    while (parser_is_name_word(parser_peek(p, 1 + n))) {
        if (at_test_word(p, 1 + n)) break;
        if (parser_at_word(p, 1 + n, "in")) {
            *in = true;
            break;
        }
        n++;
    }
    if (n == 0) return 0;
    return *in || at_test_word(p, 1 + n) ? n : 0;
}

static AggregateKind quantifier_kind(const char *word) {
    return strcmp(word, "every") == 0 || strcmp(word, "all") == 0 ? AGG_ALL
           : strcmp(word, "any") == 0                               ? AGG_ANY
                                                                    : AGG_NONE;
}

// any|every|no ITEM TEST (the list named like the item's plural), any|no ITEM in L TEST.
static Expr *parse_singular(Parser *p, size_t words, bool in) {
    const Token *word = parser_advance(p);
    Expr *test = new_aggregate(p, quantifier_kind(word->text), parser_token_pos(word), word->span);
    Name *item = &test->as.aggregate.item;
    parser_take_name(p, words, item);
    if (in) {
        parser_advance(p);
        test->as.aggregate.list = parse_list_part(p);
        if (p->sentence_failed) return parser_error_expr(p);
    } else {
        test->as.aggregate.list = parser_implied_list(p, item);
        test->as.aggregate.implied = test->as.aggregate.list->kind == EXPR_NAME;
    }
    Expr *it = ast_new_expr(p->arena, EXPR_IT, item->pos);
    test->as.aggregate.condition = parser_parse_comparison_after(p, it);
    return finish(test, test->as.aggregate.condition);
}

// all L TEST, all of L TEST, any of L TEST, none of L TEST.
static Expr *parse_plural(Parser *p) {
    const Token *word = parser_advance(p);
    if (parser_at_word(p, 0, "of")) parser_advance(p);
    Expr *test = new_aggregate(p, quantifier_kind(word->text), parser_token_pos(word), word->span);
    test->as.aggregate.list = parse_list_part(p);
    if (p->sentence_failed) return parser_error_expr(p);
    if (!at_singular_test(p) && !parser_at_item_test(p, 0)) {
        const Token *token = parser_peek(p, 0);
        const char *list = parser_text(p, ast_pos_join(test->pos, test->as.aggregate.list->pos).span);
        if (parser_error(p, token->span, "I expected a test like \"are above 50\" after \"%s\", but found %s.", list,
                         parser_describe(p, token))) {
            diag_note(p->diag, "Write it like \"%s are at least 50\".", list);
            diag_info(p->diag, DIAG_MISSING, "a test", NULL);
        }
        return parser_error_expr(p);
    }
    test->as.aggregate.condition = parser_parse_item_test(p, "all scores are at least 50");
    return finish(test, test->as.aggregate.condition);
}

static bool starts_list(const Parser *p, size_t ahead) {
    return parser_is_name_word(parser_peek(p, ahead)) || parser_at(p, ahead, TOK_LPAREN);
}

// --- totals ----------------------------------------------------------------------

static const struct {
    const char *word;
    AggregateKind kind;
    const char *example;
} totals[] = {
    {"total", AGG_TOTAL, "To add up a list, write it like \"scores added together\"."},
    {"sum", AGG_TOTAL, "To add up a list, write it like \"scores added together\"."},
    {"average", AGG_AVERAGE, "To find the average of a list, give the definition another name."},
    {"median", AGG_MEDIAN, "To find the middle of a list, give the definition another name."},
};

// the total|average|median [FIELD] of L [per FIELD] / the number of L [per FIELD]
static Expr *parse_total(Parser *p, AggregateKind kind, size_t field) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op = parser_advance(p)->span;
    Expr *total = new_aggregate(p, kind, start, op);
    if (field > 0) total->as.aggregate.key = parse_item_field(p, field);
    Span of = parser_advance(p)->span;
    const char *example = arena_sprintf(p->arena, "the %s of scores", parser_text(p, op));
    if (missing_after(p, of, example)) return parser_error_expr(p);
    total->as.aggregate.list = parse_list_part(p);
    if (p->sentence_failed) return parser_error_expr(p);
    if (parser_at_word(p, 0, "per") && !parser_parse_group(p, total)) return parser_error_expr(p);
    return finish(total, total->as.aggregate.group ? total->as.aggregate.group : total->as.aggregate.list);
}

// "per region", "per month", "per year of signup", at "per".
bool parser_parse_group(Parser *p, Expr *aggregate) {
    if ((parser_known_field_words(p, 1) == 0 || parser_at_word(p, 2, "of")) &&
        (aggregate->as.aggregate.group = parse_period(p)) != NULL) {
        return true;
    }
    aggregate->as.aggregate.group = parse_field_after(p, "per", "the total amount of orders per region");
    return aggregate->as.aggregate.group != NULL;
}

static Expr *parse_totals(Parser *p) {
    const char *word = parser_peek(p, 0)->text;
    bool number = strcmp(word, "number") == 0;
    for (size_t i = 0; i < COUNT(totals) && !number; i++) {
        if (strcmp(word, totals[i].word) != 0) continue;
        size_t field = parser_field_words(p, 1);
        if (!parser_at_word(p, 1 + field, "of") && !parser_at_word(p, 1, "of")) return NULL;
        if (!parser_at_word(p, 1 + field, "of")) field = 0;
        if (is_function_word(p, 0, totals[i].example)) return p->sentence_failed ? parser_error_expr(p) : NULL;
        return parse_total(p, totals[i].kind, field);
    }
    if (!number || !parser_at_word(p, 1, "of")) return NULL;
    if (is_function_word(p, 0, "To count a list, write it like \"the number of items in scores\".")) return p->sentence_failed ? parser_error_expr(p) : NULL;
    return parse_total(p, AGG_COUNT, 0);
}

// --- top, lowest, most common, with the highest ------------------------------------

// How many to take: a number, a name, or a value in parentheses.
static Expr *parse_how_many_to_take(Parser *p) {
    if (parser_at(p, 0, TOK_WORD)) {
        Expr *name = ast_new_expr(p->arena, EXPR_NAME, parser_token_pos(parser_peek(p, 0)));
        Name taken;
        parser_take_name(p, 1, &taken);
        name->as.name = taken.text;
        name->display = taken.display;
        return name;
    }
    if (parser_at(p, 0, TOK_NUMBER)) {  // not parse_primary: "3 most common" would start again
        const Token *token = parser_advance(p);
        Expr *number = ast_new_expr(p->arena, EXPR_NUMBER, parser_token_pos(token));
        number->as.number.text = token->text;
        number->as.number.is_decimal = strchr(token->text, '.') != NULL;
        return number;
    }
    return parse_primary(p);
}

// the top|lowest N L [by FIELD]
static Expr *parse_top(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    const Token *word = parser_advance(p);
    Expr *top = new_aggregate(p, AGG_TOP, start, word->span);
    top->as.aggregate.lowest = strcmp(word->text, "lowest") == 0;
    top->as.aggregate.count = parse_how_many_to_take(p);
    if (p->sentence_failed) return parser_error_expr(p);
    if (missing_after(p, p->prev_span, "the top 3 scores")) return parser_error_expr(p);
    top->as.aggregate.list = parse_list_part(p);
    if (p->sentence_failed) return parser_error_expr(p);
    Expr *last = top->as.aggregate.list;
    if (parser_at_word(p, 0, "by")) {
        top->as.aggregate.key = parse_field_after(p, "by", "the top 3 orders by amount");
        if (!top->as.aggregate.key) return parser_error_expr(p);
        last = top->as.aggregate.key;
    }
    return finish(top, last);
}

static bool at_top(const Parser *p) {
    if (!parser_at_word(p, 0, "top") && !parser_at_word(p, 0, "lowest")) return false;
    if (parser_is_function(p, parser_peek(p, 0)->text)) return false;
    return parser_at(p, 1, TOK_NUMBER) || parser_at(p, 1, TOK_LPAREN) ||
           (parser_is_name_word(parser_peek(p, 1)) && starts_list(p, 2));
}

bool parser_at_most_common(const Parser *p) {
    const Token *token = parser_peek(p, 0);
    bool count = token->kind == TOK_NUMBER || (parser_is_name_word(token) && !parser_is_function(p, token->text));
    return count && parser_match_words_at(p, 1, "most common") && starts_list(p, 3);
}

// the N most common L
Expr *parser_parse_most_common(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Expr *common = new_aggregate(p, AGG_MOST_COMMON, start, parser_peek(p, 1)->span);
    common->as.aggregate.count = parse_how_many_to_take(p);
    common->as.aggregate.op_span = parser_consume(p, 2);
    common->as.aggregate.list = parse_list_part(p);
    if (p->sentence_failed) return parser_error_expr(p);
    return finish(common, common->as.aggregate.list);
}

static const struct {
    const char *word;
    bool lowest;
} superlatives[] = {
    {"highest", false}, {"biggest", false}, {"largest", false}, {"greatest", false}, {"most", false},
    {"lowest", true},   {"smallest", true}, {"fewest", true},   {"least", true},
};

static int superlative_at(const Parser *p, size_t ahead) {
    for (size_t i = 0; i < COUNT(superlatives); i++) {
        if (parser_at_word(p, ahead, superlatives[i].word)) return (int)i;
    }
    return -1;
}

// After the item's name at token n: "with the highest" straight away, or
// "in L ... with the highest" (at depth 0, before the sentence ends).
static bool at_with_superlative(const Parser *p, size_t n) {
    if (parser_at_word(p, n, "with")) return superlative_at(p, n + 1) >= 0;
    if (!parser_at_word(p, n, "in")) return false;
    int depth = 0;
    for (size_t i = n + 1;; i++) {
        TokenKind kind = parser_peek(p, i)->kind;
        if (kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT ||
            kind == TOK_INDENT || kind == TOK_COLON || kind == TOK_COMMA) {
            return false;
        }
        if (kind == TOK_LPAREN) depth++;
        if (kind == TOK_RPAREN && --depth < 0) return false;
        if (depth == 0 && parser_at_word(p, i, "with") && superlative_at(p, i + 1) >= 0) return true;
    }
}

// How many words name the item in "the customer [in L] with the highest ...", or 0.
static size_t best_item_words(const Parser *p) {
    if (p->no_best) return 0;
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, n)) && !parser_at_word(p, n, "with") && !parser_at_word(p, n, "in")) n++;
    if (n == 0 || !at_with_superlative(p, n)) return 0;
    const KnownName *known = parser_match_name(p, 0, true);
    return known && known->words >= n ? 0 : n;  // "customer with ..." calls a function called "customer"
}

// The list named like the item's plural ("scores" for "any score"); for an
// item whose first word is an adjective ("every loyal customer"), the items
// of the rest's plural that it describes.
Expr *parser_implied_list(Parser *p, const Name *item) {
    Expr *list = ast_new_expr(p->arena, EXPR_NAME, item->pos);
    const char *space = strchr(item->text, ' ');
    if (space && parser_is_describing(p, arena_sprintf(p->arena, "%.*s", (int)(space - item->text), item->text))) {
        const char *shown = strchr(item->display, ' ');
        list->as.name = parser_plural_of(p->arena, space + 1);
        list->display = parser_plural_of(p->arena, shown ? shown + 1 : space + 1);
        Expr *it = ast_new_expr(p->arena, EXPR_IT, item->pos);
        Expr *only = ast_new_expr(p->arena, EXPR_ONLY, item->pos);
        only->as.only.list = list;
        only->as.only.condition = parser_adjective_call(p, it, arena_sprintf(p->arena, "%.*s", (int)(space - item->text),
                                                                              item->text), item->pos);
        return only;
    }
    list->as.name = parser_plural_of(p->arena, item->text);
    list->display = parser_plural_of(p->arena, item->display);
    return list;
}

// "with the highest|lowest|most ... FIELD", after the item and its list.
Expr *parser_finish_best(Parser *p, Expr *best) {
    if (!(parser_at_word(p, 0, "with") || parser_at_word(p, 0, "has")) || superlative_at(p, 1) < 0) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected \"with the highest\" here, but found %s.",
                         parser_describe(p, token))) {
            diag_note(p->diag, "Write it like \"the customer in buyers with the highest total\".");
            diag_info(p->diag, DIAG_MISSING, "\"with the highest\"", NULL);
        }
        return parser_error_expr(p);
    }
    parser_advance(p);
    best->as.aggregate.lowest = superlatives[superlative_at(p, 0)].lowest;
    best->as.aggregate.op_span = ast_pos_join((SourcePos){p->prev_span, 0, 0, 0},
                                              parser_token_pos(parser_peek(p, 0))).span;
    best->as.aggregate.key = parse_field_after(p, parser_peek(p, 0)->text, "the dog with the highest age");
    if (!best->as.aggregate.key) return parser_error_expr(p);
    return finish(best, best->as.aggregate.key);
}

Expr *parser_new_best(Parser *p, const Name *item, SourcePos start) {
    Expr *best = new_aggregate(p, AGG_BEST, start, item->pos.span);
    best->as.aggregate.item = *item;
    best->as.aggregate.list = parser_implied_list(p, item);
    best->as.aggregate.implied = best->as.aggregate.list->kind == EXPR_NAME;
    return best;
}

// the ITEM [in L] with the highest|lowest|most|fewest ... FIELD
static Expr *parse_best(Parser *p, size_t words) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Name item;
    parser_take_name(p, words, &item);
    Expr *best = parser_new_best(p, &item, start);
    if (parser_at_word(p, 0, "in")) {
        best->as.aggregate.implied = false;
        parser_advance(p);
        const KnownName *known = parser_match_name(p, 0, false);
        size_t words = known ? known->words : 1;
        if (parser_is_name_word(parser_peek(p, 0)) && parser_at_word(p, words, "with")) {
            // "in buyers with the highest": a name, not a question or a call of its own
            Expr *list = ast_new_expr(p->arena, EXPR_NAME, parser_token_pos(parser_peek(p, 0)));
            Name name;
            parser_take_name(p, words, &name);
            list->as.name = name.text;
            list->display = name.display;
            list->pos = name.pos;
            best->as.aggregate.list = list;
        } else {
            bool no_best = p->no_best;
            p->no_best = true;
            best->as.aggregate.list = parse_list_part(p);
            p->no_best = no_best;
        }
        if (p->sentence_failed) return parser_error_expr(p);
    }
    return parser_finish_best(p, best);
}

// --- Where each starts -----------------------------------------------------------

Expr *parser_parse_aggregate_word(Parser *p) {
    const Token *token = parser_peek(p, 0);
    if (token->kind != TOK_WORD) return NULL;
    const char *word = token->text;
    if (strcmp(word, "how") == 0 && parser_at_word(p, 1, "many") && !parser_is_function(p, "how")) {
        return parse_how_many(p);
    }
    bool in;
    size_t words;
    bool quantifier = strcmp(word, "any") == 0 || strcmp(word, "every") == 0 || strcmp(word, "no") == 0;
    if (quantifier && !parser_is_function(p, word) && (words = item_words(p, &in)) > 0 &&
        !(in && strcmp(word, "every") == 0)) {
        return parse_singular(p, words, in);
    }
    bool plural = strcmp(word, "all") == 0 || ((strcmp(word, "any") == 0 || strcmp(word, "none") == 0) &&
                                               parser_at_word(p, 1, "of"));
    if (plural && !parser_is_function(p, word) && (parser_at_word(p, 1, "of") || starts_list(p, 1))) {
        return parse_plural(p);
    }
    Expr *total = parse_totals(p);
    if (total) return total;
    if (parser_at_top_group(p)) return parser_parse_top_group(p);    // the top 3 stores by total amount
    if (parser_at_best_group(p)) return parser_parse_best_group(p);  // the store with the highest total amount
    if (at_top(p)) return parse_top(p);
    if (parser_at_most_common(p)) return parser_parse_most_common(p);
    if ((words = best_item_words(p)) > 0) return parse_best(p, words);
    return NULL;
}
