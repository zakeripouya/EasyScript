// Expression grammar, lowest to highest precedence:
//
//   every       every NAME in L <the rest>   (only at the start of a value)
//   or          A or B
//   and         A and B           (stops before "and call")
//   not         not A
//   comparison  A is B, A is greater than B, A >= B, L contains X, L is empty, ...   (no chaining)
//   join        A followed by B   (below arithmetic, so math happens first)
//   additive    A plus B, A minus B, + -
//   multiply    A times B, A multiplied by B, A divided by B, A mod B, * / %
//   unary       -A
//   postfix     A as a number, A as text, L added together, L reversed
//   primary     literals, names, calls, ( ), length of X, contents of file X,
//               lists (parse_list.c), only L that are ...

#include <string.h>
#include "front/parse_internal.h"

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

static const Phrase comparison_ops[] = {
    {"is greater than or equal to", "is greater than or equal to", BINARY_GREATER_EQUAL, false},
    {"is more than or equal to", "is more than or equal to", BINARY_GREATER_EQUAL, false},
    {"is bigger than or equal to", "is bigger than or equal to", BINARY_GREATER_EQUAL, false},
    {"is less than or equal to", "is less than or equal to", BINARY_LESS_EQUAL, false},
    {"is smaller than or equal to", "is smaller than or equal to", BINARY_LESS_EQUAL, false},
    {"is greater than", "is greater than", BINARY_GREATER, false},
    {"is more than", "is more than", BINARY_GREATER, false},
    {"is bigger than", "is bigger than", BINARY_GREATER, false},
    {"is above", "is above", BINARY_GREATER, false},
    {"is over", "is over", BINARY_GREATER, false},
    {"is less than", "is less than", BINARY_LESS, false},
    {"is smaller than", "is smaller than", BINARY_LESS, false},
    {"is below", "is below", BINARY_LESS, false},
    {"is under", "is under", BINARY_LESS, false},
    {"is at least", "is at least", BINARY_GREATER_EQUAL, false},
    {"is at most", "is at most", BINARY_LESS_EQUAL, false},
    {"is equal to", "is equal to", BINARY_EQUAL, false},
    {"is not equal to", "is not equal to", BINARY_NOT_EQUAL, false},
    {"isn't equal to", "isn't equal to", BINARY_NOT_EQUAL, false},
    {"is not", "is not", BINARY_NOT_EQUAL, false},
    {"isn't", "isn't", BINARY_NOT_EQUAL, false},
    {"is", "is", BINARY_EQUAL, false},
    {"equals", "equals", BINARY_EQUAL, false},
    {"reaches", "reaches", BINARY_GREATER_EQUAL, false},
    {"=", "=", BINARY_EQUAL, false},
    {"==", "==", BINARY_EQUAL, false},
    {"!=", "!=", BINARY_NOT_EQUAL, false},
    {"<", "<", BINARY_LESS, false},
    {">", ">", BINARY_GREATER, false},
    {"<=", "<=", BINARY_LESS_EQUAL, false},
    {">=", ">=", BINARY_GREATER_EQUAL, false},
    {"contains", "contains", BINARY_CONTAINS, false},
    {"does not contain", "does not contain", BINARY_NOT_CONTAINS, false},
    {"doesn't contain", "doesn't contain", BINARY_NOT_CONTAINS, false},
};

// "only the scores that are at least 70": the comparisons, with "are" for "is".
static const Phrase item_comparisons[] = {
    {"are greater than or equal to", "are greater than or equal to", BINARY_GREATER_EQUAL, false},
    {"are more than or equal to", "are more than or equal to", BINARY_GREATER_EQUAL, false},
    {"are bigger than or equal to", "are bigger than or equal to", BINARY_GREATER_EQUAL, false},
    {"are less than or equal to", "are less than or equal to", BINARY_LESS_EQUAL, false},
    {"are smaller than or equal to", "are smaller than or equal to", BINARY_LESS_EQUAL, false},
    {"are greater than", "are greater than", BINARY_GREATER, false},
    {"are more than", "are more than", BINARY_GREATER, false},
    {"are bigger than", "are bigger than", BINARY_GREATER, false},
    {"are above", "are above", BINARY_GREATER, false},
    {"are over", "are over", BINARY_GREATER, false},
    {"are less than", "are less than", BINARY_LESS, false},
    {"are smaller than", "are smaller than", BINARY_LESS, false},
    {"are below", "are below", BINARY_LESS, false},
    {"are under", "are under", BINARY_LESS, false},
    {"are at least", "are at least", BINARY_GREATER_EQUAL, false},
    {"are at most", "are at most", BINARY_LESS_EQUAL, false},
    {"are equal to", "are equal to", BINARY_EQUAL, false},
    {"are not equal to", "are not equal to", BINARY_NOT_EQUAL, false},
    {"aren't equal to", "aren't equal to", BINARY_NOT_EQUAL, false},
    {"are not", "are not", BINARY_NOT_EQUAL, false},
    {"aren't", "aren't", BINARY_NOT_EQUAL, false},
    {"are", "are", BINARY_EQUAL, false},
};

// "L is empty" (lists and text); "is empty list of ..." compares with an empty list instead.
static const Phrase empty_tests[] = {
    {"is empty", "is empty", LIST_IS_EMPTY, false},
    {"is not empty", "is not empty", LIST_NOT_EMPTY, false},
    {"isn't empty", "isn't empty", LIST_NOT_EMPTY, false},
    {"are empty", "are empty", LIST_IS_EMPTY, false},
    {"are not empty", "are not empty", LIST_NOT_EMPTY, false},
    {"aren't empty", "aren't empty", LIST_NOT_EMPTY, false},
};

static const Phrase list_postfix[] = {
    {"added together", "added together", LIST_SUM, false},
    {"reversed", "reversed", LIST_REVERSED, false},
};

static const Phrase additive_ops[] = {
    {"plus", "plus", BINARY_ADD, false},
    {"+", "+", BINARY_ADD, false},
    {"minus", "minus", BINARY_SUBTRACT, false},
    {"-", "-", BINARY_SUBTRACT, false},
};

static const Phrase join_ops[] = {
    {"followed by", "followed by", BINARY_JOIN, false},
};

// "times" is multiplication only when a value follows, so a sentence such as
// "repeat 3 times:" keeps its "times".
static const Phrase multiplicative_ops[] = {
    {"times", "times", BINARY_MULTIPLY, true},
    {"multiplied by", "multiplied by", BINARY_MULTIPLY, false},
    {"*", "*", BINARY_MULTIPLY, false},
    {"divided by", "divided by", BINARY_DIVIDE, false},
    {"/", "/", BINARY_DIVIDE, false},
    {"mod", "mod", BINARY_MODULO, false},
    {"%", "%", BINARY_MODULO, false},
};

// The lexer drops "a", so "as a number" arrives as "as number".
static const Phrase conversions[] = {
    {"as number", "as a number", CONVERT_TO_NUMBER, false},
    {"as text", "as text", CONVERT_TO_TEXT, false},
};

typedef Expr *(*ParseFn)(Parser *p);

static Expr *parse_or(Parser *p);

static Expr *new_binary(Parser *p, BinaryOp op, Span op_span, Expr *left, Expr *right) {
    Expr *expr = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(left->pos, right->pos));
    expr->as.binary.op = op;
    expr->as.binary.op_span = op_span;
    expr->as.binary.left = left;
    expr->as.binary.right = right;
    return expr;
}

static Expr *new_unary(Parser *p, UnaryOp op, const Token *op_token, Expr *operand) {
    SourcePos start = parser_token_pos(op_token);
    Expr *expr = ast_new_expr(p->arena, EXPR_UNARY, ast_pos_join(start, operand->pos));
    expr->as.unary.op = op;
    expr->as.unary.op_span = op_token->span;
    expr->as.unary.operand = operand;
    return expr;
}

static Expr *new_list_op(Parser *p, ListOp op, Span op_span, Expr *operand) {
    SourcePos end = {op_span, 0, 0};
    Expr *expr = ast_new_expr(p->arena, EXPR_LIST_OP, ast_pos_join(operand->pos, end));
    expr->as.list_op.op = op;
    expr->as.list_op.op_span = op_span;
    expr->as.list_op.operand = operand;
    return expr;
}

// operand (op operand)*, left to right.
static Expr *parse_left_assoc(Parser *p, const Phrase *ops, size_t n, ParseFn operand) {
    Expr *left = operand(p);
    for (;;) {
        size_t len;
        const Phrase *op = parser_match_phrase(p, ops, n, &len);
        if (!op) return left;
        Span op_span = parser_consume(p, len);
        Expr *right = operand(p);
        left = new_binary(p, (BinaryOp)op->value, op_span, left, right);
    }
}

bool parser_at_every(const Parser *p) {
    const Token *name = parser_peek(p, 1);
    return parser_at_word(p, 0, "every") && name->kind == TOK_WORD && !parser_is_operator_word(name->text) &&
           !parser_is_value_word(name->text) && parser_at_word(p, 2, "in");
}

// every NAME in L <the rest>: the rest of the value is worked out for each
// item, as if it started with NAME ("every score in scores plus 5").
static Expr *parse_every(Parser *p) {
    Expr *every = ast_new_expr(p->arena, EXPR_EVERY, parser_token_pos(parser_advance(p)));
    const Token *name = parser_advance(p);
    every->as.every.var.text = name->text;
    every->as.every.var.pos = parser_token_pos(name);
    parser_advance(p);  // in
    every->as.every.list = parse_primary(p);
    Expr *item = ast_new_expr(p->arena, EXPR_NAME, parser_token_pos(name));
    item->as.name = name->text;
    p->pending_item = item;
    every->as.every.body = parse_or(p);
    p->pending_item = NULL;
    every->pos = ast_pos_join(every->pos, every->as.every.body->pos);
    return every;
}

Expr *parse_expression(Parser *p) {
    return parser_at_every(p) ? parse_every(p) : parse_or(p);
}

// --- or, and, not -----------------------------------------------------------

static Expr *parse_not(Parser *p);

static Expr *parse_and(Parser *p) {
    Expr *left = parse_not(p);
    while (parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call")) {
        Span op_span = parser_consume(p, 1);
        Expr *right = parse_not(p);
        left = new_binary(p, BINARY_AND, op_span, left, right);
    }
    return left;
}

static Expr *parse_or(Parser *p) {
    Expr *left = parse_and(p);
    while (parser_at_word(p, 0, "or")) {
        Span op_span = parser_consume(p, 1);
        Expr *right = parse_and(p);
        left = new_binary(p, BINARY_OR, op_span, left, right);
    }
    return left;
}

static Expr *parse_comparison(Parser *p);

static Expr *parse_not(Parser *p) {
    if (p->pending_item || !parser_at_word(p, 0, "not")) return parse_comparison(p);
    const Token *op = parser_advance(p);
    return new_unary(p, UNARY_NOT, op, parse_not(p));
}

// --- Comparisons ------------------------------------------------------------


static bool is_word_among(const Token *token, const char *const *words, size_t n) {
    if (token->kind != TOK_WORD) return false;
    for (size_t i = 0; i < n; i++) {
        if (strcmp(token->text, words[i]) == 0) return true;
    }
    return false;
}

// "is not greater than" isn't a comparison; suggest the one that is ("are
// at most" after "are not").
static bool reject_negated_comparison(Parser *p, BinaryOp op, Span op_span) {
    static const char *const bigger[] = {"greater", "more", "bigger", "above", "over"};
    static const char *const smaller[] = {"less", "smaller", "below", "under"};
    if (op != BINARY_NOT_EQUAL) return false;
    const Token *next = parser_peek(p, 0);
    const char *instead = is_word_among(next, bigger, COUNT(bigger))    ? "at most"
                          : is_word_among(next, smaller, COUNT(smaller)) ? "at least"
                                                                         : NULL;
    if (!instead) return false;
    instead = arena_sprintf(p->arena, "%s %s", p->source[op_span.offset] == 'a' ? "are" : "is", instead);
    const Token *last = parser_at_word(p, 1, "than") ? parser_peek(p, 1) : next;
    Span span = {op_span.offset, last->span.offset + last->span.length - op_span.offset};
    if (parser_error(p, span, "EasyScript doesn't have \"%s\".", parser_text(p, span))) {
        diag_note(p->diag, "Did you mean \"%s\"?", instead);
    }
    return true;
}

// "or equal to" only follows "greater than"-style phrases; after "is above"
// and the symbols it isn't a comparison.
static bool reject_or_equal_to(Parser *p, BinaryOp op, Span op_span) {
    size_t n = parser_match_words(p, "or equal to");
    if (n == 0 || (op != BINARY_GREATER && op != BINARY_LESS)) return false;
    Span tail = parser_consume(p, n);
    Span span = {op_span.offset, tail.offset + tail.length - op_span.offset};
    if (parser_error(p, span, "\"%s\" isn't a comparison.", parser_text(p, span))) {
        bool symbol = p->source[op_span.offset] == '<' || p->source[op_span.offset] == '>';
        const char *verb = p->source[op_span.offset] == 'a' ? "are" : "is";
        const char *instead = symbol ? (op == BINARY_GREATER ? ">=" : "<=")
                                     : arena_sprintf(p->arena, "%s at %s", verb, op == BINARY_GREATER ? "least" : "most");
        diag_note(p->diag, "Did you mean \"%s\"?", instead);
    }
    return true;
}

static Expr *report_chained(Parser *p, Expr *comparison, size_t op_len) {
    Span second_op = parser_consume(p, op_len);
    Expr *last = parse_join(p);
    const Expr *middle = comparison->as.binary.right;
    Span whole = ast_pos_join(comparison->pos, last->pos).span;
    if (parser_error(p, whole, "Comparisons can't be chained like this.")) {
        diag_note(p->diag, "Compare one pair at a time and join them with \"and\": \"%s and %s %s %s\".",
                  parser_text(p, comparison->pos.span), parser_text(p, middle->pos.span),
                  parser_text(p, second_op), parser_text(p, last->pos.span));
    }
    Expr *error = ast_new_expr(p->arena, EXPR_ERROR, ast_pos_join(comparison->pos, last->pos));
    return error;
}

// "is empty", "isn't empty", ... (but not "is empty list of", which compares
// with an empty list). NULL if none is next.
static Expr *parse_empty_test(Parser *p, Expr *left, size_t first, size_t n) {
    for (size_t i = first; i < first + n; i++) {
        size_t len = parser_match_words(p, empty_tests[i].words);
        if (len > 0 && !parser_at_word(p, len, "list")) {
            return new_list_op(p, (ListOp)empty_tests[i].value, parser_consume(p, len), left);
        }
    }
    return NULL;
}

static Expr *parse_comparison(Parser *p) {
    Expr *left = parse_join(p);
    size_t len;
    Expr *empty = parse_empty_test(p, left, 0, 3);
    if (empty && parser_match_phrase(p, comparison_ops, COUNT(comparison_ops), &len)) {
        Span op = parser_consume(p, len);
        if (parser_error(p, op, "Comparisons can't be chained like this.")) {
            diag_note(p->diag, "Compare one pair at a time and join them with \"and\".");
        }
        return parser_error_expr(p);
    }
    if (empty) return empty;
    const Phrase *op = parser_match_phrase(p, comparison_ops, COUNT(comparison_ops), &len);
    if (!op) return left;
    Span op_span = parser_consume(p, len);
    BinaryOp kind = (BinaryOp)op->value;
    if (reject_negated_comparison(p, kind, op_span) || reject_or_equal_to(p, kind, op_span)) {
        return parser_error_expr(p);
    }
    Expr *result = new_binary(p, kind, op_span, left, parse_join(p));
    if (parser_match_phrase(p, comparison_ops, COUNT(comparison_ops), &len)) {
        return report_chained(p, result, len);
    }
    return result;
}

// --- Arithmetic, unary, postfix ---------------------------------------------

const Phrase *parser_match_arithmetic(Parser *p, size_t *len) {
    const Phrase *op = parser_match_phrase(p, additive_ops, COUNT(additive_ops), len);
    if (!op) op = parser_match_phrase(p, multiplicative_ops, COUNT(multiplicative_ops), len);
    if (!op) op = parser_match_phrase(p, join_ops, COUNT(join_ops), len);
    return op;
}

static Expr *parse_multiplicative(Parser *p) {
    return parse_left_assoc(p, multiplicative_ops, COUNT(multiplicative_ops), parse_unary);
}

static Expr *parse_additive(Parser *p) {
    return parse_left_assoc(p, additive_ops, COUNT(additive_ops), parse_multiplicative);
}

Expr *parse_join(Parser *p) {
    return parse_left_assoc(p, join_ops, COUNT(join_ops), parse_additive);
}

static Expr *parse_postfix(Parser *p) {
    Expr *expr = parse_primary(p);
    for (;;) {
        if (p->as_ends_value && parser_at_word(p, 0, "as")) return expr;
        size_t len;
        const Phrase *list_op = parser_match_phrase(p, list_postfix, COUNT(list_postfix), &len);
        if (list_op) {
            expr = new_list_op(p, (ListOp)list_op->value, parser_consume(p, len), expr);
            continue;
        }
        const Phrase *conversion = parser_match_phrase(p, conversions, COUNT(conversions), &len);
        if (!conversion) return expr;
        Span op_span = parser_consume(p, len);
        SourcePos end = {op_span, 0, 0};
        Expr *converted = ast_new_expr(p->arena, EXPR_CONVERT, ast_pos_join(expr->pos, end));
        converted->as.convert.target = (ConvertTarget)conversion->value;
        converted->as.convert.op_span = op_span;
        converted->as.convert.operand = expr;
        expr = converted;
    }
}

Expr *parse_unary(Parser *p) {
    if (p->pending_item || !parser_at(p, 0, TOK_MINUS)) return parse_postfix(p);
    const Token *op = parser_advance(p);
    return new_unary(p, UNARY_NEGATE, op, parse_unary(p));
}

// --- only --------------------------------------------------------------------

// only L that are <comparison>: the items of L for which the comparison is
// yes. The item is the comparison's left side (an "it").
Expr *parser_parse_only(Parser *p) {
    static const char *const that[] = {"that", NULL};
    static const char *const example = "only the scores that are at least 70";
    Expr *only = ast_new_expr(p->arena, EXPR_ONLY, parser_token_pos(parser_advance(p)));
    only->as.only.list = parse_primary(p);
    if (p->sentence_failed || !parser_expect_word(p, that, example)) return parser_error_expr(p);
    Expr *it = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(parser_peek(p, 0)));
    Expr *condition = parse_empty_test(p, it, 3, 3);
    if (!condition) {
        size_t len;
        const Phrase *op = parser_match_phrase(p, item_comparisons, COUNT(item_comparisons), &len);
        if (!op) {
            const Token *token = parser_peek(p, 0);
            if (parser_error(p, token->span, "I expected a comparison like \"are at least\" here, but found %s.",
                             parser_describe(p, token))) {
                diag_note(p->diag, "Write it like \"%s\".", example);
            }
            return parser_error_expr(p);
        }
        Span op_span = parser_consume(p, len);
        BinaryOp kind = (BinaryOp)op->value;
        if (reject_negated_comparison(p, kind, op_span) || reject_or_equal_to(p, kind, op_span)) {
            return parser_error_expr(p);
        }
        it->pos.span = op_span;
        condition = new_binary(p, kind, op_span, it, parse_join(p));
    }
    only->as.only.condition = condition;
    only->pos = ast_pos_join(only->pos, condition->pos);
    return only;
}

// "only scores that" or "only (" starts a choice; "only" alone is a name.
bool parser_at_only(const Parser *p) {
    if (!parser_at_word(p, 0, "only")) return false;
    const Token *next = parser_peek(p, 1);
    return next->kind == TOK_LPAREN || (next->kind == TOK_WORD && parser_at_word(p, 2, "that"));
}
