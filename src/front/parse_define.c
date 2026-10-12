// Definitions written as sentences, at the start of a line:
//
//   a customer is loyal if CONDITION.                 an adjective: "if rex is loyal", "the loyal customers"
//   a big order is one whose FIELD ... / that ... / where ...      the same, said another way
//   a customer's profit is VALUE.                      a value worked out from the record: "rex's profit"
//   the best customer is the one with the highest profit.          a named value
//   the big orders are the ones whose amount is over 1000.         (a filter of "orders")
//
// Adjectives and values become methods (STMT_FUNCTION with `defines`), named
// "is loyal" and "the profit" (words a program's own names can't have), whose
// body gives back the condition or value with "it" (and "its", "their") the
// record. A named value is read again from its own words wherever its name is
// used, so it sees the names around that place, like the value written there.

#include <ctype.h>
#include <string.h>
#include "front/parse_internal.h"

static bool at_line_start(const Parser *p) {
    if (p->pos == 0) return true;
    TokenKind before = p->tokens[p->pos - 1].kind;
    return before == TOK_NEWLINE || before == TOK_INDENT || before == TOK_DEDENT;
}

static bool at_filter_word(const Parser *p, size_t ahead) {
    return parser_at_word(p, ahead, "whose") || parser_at_word(p, ahead, "that") || parser_at_word(p, ahead, "where");
}

// Whether the word `ahead` had a dropped "the" before it: "the one" names one
// thing ("the best customer is the one with ..."), "one" alone describes any
// ("a big order is one whose ...").
static bool after_the(const Parser *p, size_t ahead) {
    Span filler = parser_peek(p, ahead)->filler;
    if (filler.length != 3) return false;
    const char *word = p->source + filler.offset;
    return tolower((unsigned char)word[0]) == 't' && tolower((unsigned char)word[1]) == 'h' &&
           tolower((unsigned char)word[2]) == 'e';
}

// --- What starts here ------------------------------------------------------------------

typedef enum { NONE, ADJECTIVE_IF, ADJECTIVE_ONE, VALUE, NAMED, RULE } Shape;

// How many words name the value in "the best customer is the one ...", or 0.
static size_t named_words(const Parser *p, size_t *verb) {
    const Token *first = parser_peek(p, 0);
    if (!parser_is_name_word(first) || parser_is_statement_word(first->text)) return 0;
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, n)) && !parser_at_word(p, n, "are")) n++;
    if (n == 0 || !(parser_at_word(p, n, "is") || parser_at_word(p, n, "are"))) return 0;
    bool one = parser_at_word(p, n + 1, "one") || parser_at_word(p, n + 1, "ones");
    if (!one || !after_the(p, n + 1) || !(parser_at_word(p, n + 2, "with") || at_filter_word(p, n + 2))) return 0;
    *verb = n;
    return n;
}

// How many words name the field before "must", at `ahead`, or 0.
static size_t rule_words(const Parser *p, size_t ahead) {
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, ahead + n)) && !parser_at_word(p, ahead + n, "must")) n++;
    return n > 0 && parser_at_word(p, ahead + n, "must") ? n : 0;
}

static Shape shape_here(const Parser *p, size_t *kind_words) {
    if (!at_line_start(p)) return NONE;
    size_t words, verb;
    Type kind = parser_match_kind_name(p, 0, false, &words);
    if (type_is_record(kind) || type_is_sum(kind)) {
        *kind_words = words;
        if (parser_at_word(p, words, "is") && parser_is_name_word(parser_peek(p, words + 1)) &&
            parser_at_word(p, words + 2, "if")) {
            return ADJECTIVE_IF;
        }
        if (parser_at(p, words, TOK_POSSESSIVE) && parser_is_name_word(parser_peek(p, words + 1))) {
            if (rule_words(p, words + 1) > 0) return RULE;  // a customer's email must ...
            for (size_t i = words + 1; parser_is_name_word(parser_peek(p, i)) || parser_at_word(p, i, "is"); i++) {
                if (parser_at_word(p, i, "is")) return i > words + 1 ? VALUE : NONE;
            }
        }
        return NONE;
    }
    if (rule_words(p, 0) > 0 && !parser_is_statement_word(parser_peek(p, 0)->text)) {  // an age must be ...
        *kind_words = 0;
        return RULE;
    }
    kind = parser_is_name_word(parser_peek(p, 0)) ? parser_match_kind_name(p, 1, false, &words) : TYPE_UNKNOWN;
    if ((type_is_record(kind) || type_is_sum(kind)) &&
        parser_at_word(p, 1 + words, "is") && parser_at_word(p, 2 + words, "one") && !after_the(p, 2 + words) &&
        at_filter_word(p, 3 + words)) {
        *kind_words = words;
        return ADJECTIVE_ONE;
    }
    return named_words(p, &verb) > 0 ? NAMED : NONE;
}

bool parser_at_definition(const Parser *p) {
    size_t words;
    return shape_here(p, &words) != NONE;
}

// --- Pre-scan --------------------------------------------------------------------------

static void add_adjective(Parser *p, const char *word) {
    if (parser_is_adjective(p, word)) return;
    KnownName adjective = {word, 1, true, 0, true};
    vec_push(p->arena, &p->adjectives, adjective);
}

void parser_find_definitions(Parser *p) {
    size_t saved = p->pos;
    for (p->pos = 0; p->pos + 1 < p->count; p->pos++) {
        size_t words = 0, verb = 0, n;
        switch (shape_here(p, &words)) {
        case ADJECTIVE_IF: add_adjective(p, parser_peek(p, words + 1)->text); break;
        case ADJECTIVE_ONE: add_adjective(p, parser_peek(p, 0)->text); break;
        case VALUE:
            for (n = 0; !parser_at_word(p, words + 1 + n, "is"); n++) {}
            Name field;
            size_t at = p->pos;
            p->pos += words + 1;
            parser_take_name(p, n, &field);
            p->pos = at;
            parser_add_field_name(p, field.text, n);
            break;
        case NAMED: {
            n = named_words(p, &verb);
            Name name;
            size_t at = p->pos;
            parser_take_name(p, n, &name);
            p->pos = at;
            const char *last = strrchr(name.text, ' ');
            Definition def = {name.text, n, at + n + 1, last ? last + 1 : name.text, 0, false};
            vec_push(p->arena, &p->definitions, def);
            break;
        }
        case RULE:
        case NONE: break;
        }
    }
    p->pos = saved;
}

bool parser_is_adjective(const Parser *p, const char *word) {
    for (size_t i = 0; i < p->adjectives.len; i++) {
        if (strcmp(p->adjectives.items[i].text, word) == 0) return true;
    }
    return false;
}

// --- Definitions -----------------------------------------------------------------------

// A method for the kind at the current token, named `name`, with the kind as its input.
Stmt *parser_new_method(Parser *p, size_t kind_words, int defines) {
    const Token *first = parser_peek(p, 0);
    Stmt *fn = parser_new_stmt(p, STMT_FUNCTION, first);
    size_t words;
    Type type = parser_match_kind_name(p, 0, false, &words);
    const RecordKind *kind = type_is_sum(type) ? &p->records->sums.items[type_sum_index(type)]
                                               : &p->records->items[type_record_index(type)];
    Name record = {kind->name, kind->display, parser_token_pos(first)};
    parser_consume(p, kind_words);
    record.pos = ast_pos_join(record.pos, (SourcePos){p->prev_span, 0, 0, 0});
    fn->as.function.method = true;
    fn->as.function.defines = defines;
    vec_push(p->arena, &fn->as.function.params, record);
    vec_push(p->arena, &fn->as.function.declared, type);
    fn->as.function.declared_result = defines == DEFINES_ADJECTIVE ? TYPE_YESNO : TYPE_UNKNOWN;
    return fn;
}

static Stmt *give_back(Parser *p, Stmt *fn, Expr *value) {
    Stmt *ret = ast_new_stmt(p->arena, STMT_RETURN, value->pos);
    ret->as.returned = value;
    vec_push(p->arena, &fn->as.function.body, ret);
    fn->pos = ast_pos_join(fn->pos, value->pos);
    return p->sentence_failed ? NULL : fn;
}

static void name_adjective(Parser *p, Stmt *fn, const Token *word) {
    Name *name = &fn->as.function.name;
    name->text = arena_sprintf(p->arena, "is %s", word->text);
    name->display = arena_sprintf(p->arena, "is %s", parser_text(p, word->span));
    name->pos = parser_token_pos(word);
}

// a customer is loyal if CONDITION
static Stmt *parse_adjective_if(Parser *p, size_t kind_words) {
    Stmt *fn = parser_new_method(p, kind_words, DEFINES_ADJECTIVE);
    parser_advance(p);  // is
    name_adjective(p, fn, parser_advance(p));
    parser_advance(p);  // if
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"if\".")) {
            diag_note(p->diag, "Say when it's %s, like \"a dog is old if its age is over 10\".",
                      fn->as.function.name.display + 3);
            diag_info(p->diag, DIAG_MISSING, "a condition", "yes or no");
        }
        return NULL;
    }
    return give_back(p, fn, parse_expression(p));
}

// "whose FIELD ...", "that are ...", "where C" after "one": a test of "it".
static Expr *parse_one_test(Parser *p) {
    Expr *it = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(parser_peek(p, 0)));
    Expr *only = parser_parse_filters(p, it);  // the same words as a filter, on the one item
    if (only->kind != EXPR_ONLY) return parser_error_expr(p);
    return only->as.only.condition;
}

// a big order is one whose amount is over 1000
static Stmt *parse_adjective_one(Parser *p, size_t kind_words) {
    const Token *word = parser_advance(p);
    Stmt *fn = parser_new_method(p, kind_words, DEFINES_ADJECTIVE);
    name_adjective(p, fn, word);
    fn->pos = ast_pos_join(parser_token_pos(word), fn->pos);
    parser_consume(p, 2);  // is one
    return give_back(p, fn, parse_one_test(p));
}

// a customer's profit is VALUE
static Stmt *parse_value(Parser *p, size_t kind_words) {
    Stmt *fn = parser_new_method(p, kind_words, DEFINES_VALUE);
    parser_advance(p);  // 's
    static const char *const is[] = {"is", NULL};
    Name field;
    if (!parser_parse_name(p, &field, is)) return NULL;
    fn->as.function.name = field;
    fn->as.function.name.text = arena_sprintf(p->arena, "the %s", field.text);
    fn->as.function.name.display = field.display;
    parser_advance(p);  // is
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"is\".")) {
            diag_note(p->diag, "Say how it's worked out, like \"a customer's profit is its sales minus its costs\".");
            diag_info(p->diag, DIAG_MISSING, "a value", NULL);
        }
        return NULL;
    }
    return give_back(p, fn, parse_expression(p));
}

// --- Named values ----------------------------------------------------------------------

static Definition *definition_here(Parser *p) {
    Definition *best = NULL;
    for (size_t i = 0; i < p->definitions.len; i++) {
        Definition *def = &p->definitions.items[i];
        if ((!best || def->words > best->words) && parser_match_words(p, def->text) == def->words) best = def;
    }
    return best;
}

// The value of a named value, from its words: "one with the highest ..." or
// "ones whose|that|where ...".
static Expr *read_value(Parser *p, const Definition *def) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    bool plural = parser_at_word(p, 0, "ones");
    Span one = parser_advance(p)->span;
    Name noun = {def->noun, def->noun, {one, start.line, start.column, start.file}};
    if (!plural && parser_at_word(p, 0, "with")) return parser_finish_best(p, parser_new_best(p, &noun, start));
    if (plural && at_filter_word(p, 0)) {
        Expr *list = ast_new_expr(p->arena, EXPR_NAME, noun.pos);
        list->as.name = noun.text;
        list->display = noun.display;
        return parser_parse_filters(p, list);
    }
    if (parser_error(p, one, plural ? "\"the ones with the highest ...\" doesn't say how many." :
                                      "\"the one whose ...\" could be more than one.")) {
        diag_note(p->diag, plural ? "For one, write \"the best customer is the one with the highest profit\". For "
                                    "several, write \"the top 3 customers by profit\" where you use them."
                                  : "Say which one, like \"the one with the highest profit\", or use \"the ones whose "
                                    "...\" for all of them.");
        diag_info(p->diag, DIAG_NOT_ALLOWED, plural ? "a filter after \"the ones\"" : "\"with the highest\"", NULL);
    }
    return parser_error_expr(p);
}

// Reads a named value's words where its name is used.
static Expr *expand(Parser *p, Definition *def) {
    Name name;
    parser_take_name(p, def->words, &name);
    if (def->expanding) {
        if (parser_error(p, name.pos.span, "\"%s\" is worked out from itself.", name.display)) {
            diag_note(p->diag, "A named value can't use its own name in what it means.");
            diag_info(p->diag, DIAG_NOT_ALLOWED, "its own name", NULL);
        }
        return parser_error_expr(p);
    }
    if (def->state == 2) {
        p->sentence_failed = true;  // its mistake was already reported
        return parser_error_expr(p);
    }
    size_t at = p->pos;
    Span prev = p->prev_span;
    bool failed = p->sentence_failed;
    p->sentence_failed = false;
    p->pos = def->value;
    def->expanding = true;
    Expr *value = read_value(p, def);
    def->expanding = false;
    def->state = p->sentence_failed ? 2 : 1;
    p->sentence_failed = p->sentence_failed || failed;
    p->pos = at;
    p->prev_span = prev;
    value->pos.span = name.pos.span;  // shown where it's used
    value->pos.line = name.pos.line;
    value->pos.column = name.pos.column;
    value->pos.file = name.pos.file;
    return value;
}

static Stmt *parse_named(Parser *p) {
    Definition *def = definition_here(p);
    parser_consume(p, def->words + 1);  // the name and "is"/"are"
    if (def->state == 0) {  // read it once here, so its mistakes are reported even if it's never used
        def->expanding = true;
        read_value(p, def);
        def->expanding = false;
        def->state = p->sentence_failed ? 2 : 1;
    }
    while (!parser_at_sentence_end(p)) parser_advance(p);
    return NULL;
}

// --- Rules -----------------------------------------------------------------------------

static Expr *binary(Parser *p, BinaryOp op, Span op_span, Expr *left, Expr *right) {
    Expr *expr = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(left->pos, right->pos));
    expr->as.binary.op = op;
    expr->as.binary.op_span = op_span;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    return expr;
}

// After "must": [not] be between A and B / [not] be ... / [not] contain X / [not] start with X / [not] end with X.
static Expr *parse_must(Parser *p, SourcePos at) {
    bool negated = parser_at_word(p, 0, "not");
    Span not = negated ? parser_advance(p)->span : p->prev_span;
    Expr *it = ast_new_expr(p->arena, EXPR_IT, at);
    Expr *condition = NULL;
    static const struct {
        const char *words;
        BinaryOp op;
    } verbs[] = {{"contain", BINARY_CONTAINS}, {"start with", BINARY_STARTS_WITH}, {"end with", BINARY_ENDS_WITH}};
    if (parser_match_words(p, "be between")) {
        Span between = parser_consume(p, 2);
        Expr *low = parse_unary(p);
        static const char *const and[] = {"and", NULL};
        if (p->sentence_failed || !parser_expect_word(p, and, "an age must be between 0 and 150")) return NULL;
        Expr *high = parse_unary(p);
        Expr *again = ast_new_expr(p->arena, EXPR_IT, at);
        condition = binary(p, BINARY_AND, between, binary(p, BINARY_GREATER_EQUAL, between, it, low),
                           binary(p, BINARY_LESS_EQUAL, between, again, high));
    } else if (parser_at_word(p, 0, "be")) {
        condition = parser_parse_is_rest(p, it, parser_advance(p)->span);
    } else {
        for (size_t i = 0; i < sizeof verbs / sizeof verbs[0] && !condition; i++) {
            size_t n = parser_match_words(p, verbs[i].words);
            if (n) {
                Span op = parser_consume(p, n);
                condition = binary(p, verbs[i].op, op, it, parse_join(p));
            }
        }
    }
    if (!condition) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected \"be\", \"contain\", \"start with\" or \"end with\" here, but "
                                         "found %s.", parser_describe(p, token))) {
            diag_note(p->diag, "Write a rule like \"an age must be between 0 and 150\" or \"an email must contain "
                               "\"@\"\".");
            diag_info(p->diag, DIAG_MISSING, "what it must be", NULL);
        }
        return NULL;
    }
    if (!negated) return condition;
    Expr *opposite = ast_new_expr(p->arena, EXPR_UNARY, condition->pos);
    opposite->as.unary.op = UNARY_NOT;
    opposite->as.unary.op_span = not;
    opposite->as.unary.operand = condition;
    return opposite;
}

// [a KIND's] FIELD must RULE: the message is the sentence as written, with a capital.
static Stmt *parse_rule(Parser *p, size_t kind_words) {
    const Token *first = parser_peek(p, 0);
    size_t start = first->filler.length ? first->filler.offset : first->span.offset;
    Stmt *stmt = parser_new_stmt(p, STMT_RULE, first);
    stmt->as.rule.kind = TYPE_UNKNOWN;
    if (kind_words) {
        size_t words;
        stmt->as.rule.kind = parser_match_kind_name(p, 0, false, &words);
        parser_consume(p, kind_words + 1);  // the kind and 's
    }
    parser_take_name(p, rule_words(p, 0), &stmt->as.rule.field);
    SourcePos at = stmt->as.rule.field.pos;
    parser_advance(p);  // must
    stmt->as.rule.condition = parse_must(p, at);
    if (!stmt->as.rule.condition) return NULL;
    size_t end = p->prev_span.offset + p->prev_span.length;
    char *message = arena_sprintf(p->arena, "%.*s", (int)(end - start), p->source + start);
    message[0] = (char)toupper((unsigned char)message[0]);
    stmt->as.rule.message = message;
    return stmt;
}

Stmt *parser_parse_definition(Parser *p) {
    size_t words = 0;
    switch (shape_here(p, &words)) {
    case RULE: return parse_rule(p, words);
    case ADJECTIVE_IF: return parse_adjective_if(p, words);
    case ADJECTIVE_ONE: return parse_adjective_one(p, words);
    case VALUE: return parse_value(p, words);
    case NAMED: return parse_named(p);
    case NONE: break;
    }
    return NULL;
}

// --- Using them ------------------------------------------------------------------------

Expr *parser_adjective_call(Parser *p, Expr *subject, const char *adjective, SourcePos pos) {
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, pos);
    call->as.call.name = arena_sprintf(p->arena, "is %s", adjective);
    call->display = call->as.call.name;
    vec_push(p->arena, &call->as.call.args, subject);
    return call;
}

Expr *parser_parse_adjective_test(Parser *p, Expr *left, bool plural) {
    static const char *const singular_words[] = {"is not", "isn't", "is"};
    static const char *const plural_words[] = {"are not", "aren't", "are"};
    const char *const *verbs = plural ? plural_words : singular_words;
    for (size_t i = 0; i < 3; i++) {
        size_t n = parser_match_words(p, verbs[i]);
        if (!n || !parser_is_adjective(p, parser_peek(p, n)->text)) continue;
        Span verb = parser_consume(p, n);
        const Token *word = parser_advance(p);
        SourcePos pos = ast_pos_join(left->pos, parser_token_pos(word));
        Expr *call = parser_adjective_call(p, left, word->text, pos);
        if (i == 2) return call;
        Expr *not = ast_new_expr(p->arena, EXPR_UNARY, pos);
        not->as.unary.op = UNARY_NOT;
        not->as.unary.op_span = verb;
        not->as.unary.operand = call;
        return not;
    }
    return NULL;
}

Expr *parser_parse_defined_word(Parser *p) {
    Definition *def = definition_here(p);
    if (def) return expand(p, def);
    const Token *word = parser_peek(p, 0);
    if (word->kind != TOK_WORD || !parser_is_adjective(p, word->text) || parser_is_function(p, word->text) ||
        !parser_is_name_word(parser_peek(p, 1))) {
        return NULL;
    }
    parser_advance(p);  // "the loyal customers": the customers that are loyal
    Expr *list = parse_primary(p);
    Expr *only = ast_new_expr(p->arena, EXPR_ONLY, ast_pos_join(parser_token_pos(word), list->pos));
    only->as.only.list = list;
    Expr *it = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(word));
    only->as.only.condition = parser_adjective_call(p, it, word->text, parser_token_pos(word));
    return only;
}
