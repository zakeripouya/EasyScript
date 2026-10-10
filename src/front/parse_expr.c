// Expression grammar, lowest to highest precedence:
//
//   or          A or B
//   and         A and B           (stops before "and call")
//   not         not A
//   comparison  A is B, A is greater than B, A >= B, ...   (no chaining)
//   join        A followed by B   (below arithmetic, so math happens first)
//   additive    A plus B, A minus B, + -
//   multiply    A times B, A multiplied by B, A divided by B, A mod B, * / %
//   unary       -A
//   postfix     A as a number, A as text
//   primary     literals, names, calls, ( ), length of X, contents of file X

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
static Expr *parse_unary(Parser *p);
static Expr *parse_primary(Parser *p);

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

Expr *parse_expression(Parser *p) {
    return parse_or(p);
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
    if (!parser_at_word(p, 0, "not")) return parse_comparison(p);
    const Token *op = parser_advance(p);
    return new_unary(p, UNARY_NOT, op, parse_not(p));
}

// --- Comparisons ------------------------------------------------------------

static Expr *parse_join(Parser *p);

static bool is_word_among(const Token *token, const char *const *words, size_t n) {
    if (token->kind != TOK_WORD) return false;
    for (size_t i = 0; i < n; i++) {
        if (strcmp(token->text, words[i]) == 0) return true;
    }
    return false;
}

// "is not greater than" isn't a comparison; suggest the one that is.
static bool reject_negated_comparison(Parser *p, BinaryOp op, Span op_span) {
    static const char *const bigger[] = {"greater", "more", "bigger", "above", "over"};
    static const char *const smaller[] = {"less", "smaller", "below", "under"};
    if (op != BINARY_NOT_EQUAL) return false;
    const Token *next = parser_peek(p, 0);
    const char *instead = is_word_among(next, bigger, COUNT(bigger))    ? "is at most"
                          : is_word_among(next, smaller, COUNT(smaller)) ? "is at least"
                                                                         : NULL;
    if (!instead) return false;
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
        const char *instead = op == BINARY_GREATER ? (symbol ? ">=" : "is at least") : (symbol ? "<=" : "is at most");
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

static Expr *parse_comparison(Parser *p) {
    Expr *left = parse_join(p);
    size_t len;
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

static Expr *parse_multiplicative(Parser *p) {
    return parse_left_assoc(p, multiplicative_ops, COUNT(multiplicative_ops), parse_unary);
}

static Expr *parse_additive(Parser *p) {
    return parse_left_assoc(p, additive_ops, COUNT(additive_ops), parse_multiplicative);
}

static Expr *parse_join(Parser *p) {
    return parse_left_assoc(p, join_ops, COUNT(join_ops), parse_additive);
}

static Expr *parse_postfix(Parser *p) {
    Expr *expr = parse_primary(p);
    for (;;) {
        size_t len;
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

static Expr *parse_unary(Parser *p) {
    if (!parser_at(p, 0, TOK_MINUS)) return parse_postfix(p);
    const Token *op = parser_advance(p);
    return new_unary(p, UNARY_NEGATE, op, parse_unary(p));
}

// --- Primaries --------------------------------------------------------------

static Expr *expected_value(Parser *p) {
    if (parser_reject_bare_decimal(p)) return parser_error_expr(p);
    const Token *token = parser_peek(p, 0);
    bool after_open_paren = p->pos > 0 && p->tokens[p->pos - 1].kind == TOK_LPAREN;
    if (parser_at_sentence_end(p) || (token->kind == TOK_RPAREN && after_open_paren)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "Add a value there, like a number, some text in quotes, or a name.");
        }
    } else if (token->kind == TOK_RPAREN) {
        parser_report_leftover(p);
    } else if (parser_error(p, token->span, "I expected a value here, but found %s.", parser_describe(p, token))) {
        diag_note(p->diag, "A value is a number, some text in quotes, yes or no, nothing, or a name.");
    }
    return parser_error_expr(p);
}

static Expr *literal(Parser *p, ExprKind kind) {
    return ast_new_expr(p->arena, kind, parser_token_pos(parser_advance(p)));
}

static Expr *parse_number(Parser *p) {
    Expr *expr = literal(p, EXPR_NUMBER);
    const Token *token = &p->tokens[p->pos - 1];
    expr->as.number.text = token->text;
    expr->as.number.is_decimal = strchr(token->text, '.') != NULL;
    return expr;
}

static Expr *parse_text(Parser *p) {
    Expr *expr = literal(p, EXPR_TEXT);
    const Token *token = &p->tokens[p->pos - 1];
    expr->as.text.value = token->text;
    expr->as.text.len = token->text_len;
    return expr;
}

static Expr *parse_group(Parser *p) {
    const Token *open = parser_advance(p);
    Expr *inner = parse_expression(p);
    if (!parser_at(p, 0, TOK_RPAREN)) {
        if (parser_error(p, open->span, "This \"(\" is never closed.")) {
            diag_note(p->diag, "Add a \")\" where the group ends, on the same line.");
        }
        return inner;
    }
    const Token *close = parser_advance(p);
    inner->pos = ast_pos_join(parser_token_pos(open), parser_token_pos(close));
    return inner;
}

// "length of X" and "contents of file X" take a single primary, so
// "length of x plus 1" means "(length of x) plus 1".
static Expr *parse_prefixed(Parser *p, ExprKind kind, size_t words) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    parser_consume(p, words);
    Expr *operand = parse_primary(p);
    Expr *expr = ast_new_expr(p->arena, kind, ast_pos_join(start, operand->pos));
    expr->as.operand = operand;
    return expr;
}

// An argument directly followed by arithmetic could belong to the call or
// to the whole sentence ("double using 21 plus 1"), so that's an error.
static Expr *reject_ambiguous_call(Parser *p, Expr *call) {
    size_t len;
    const Phrase *op = parser_match_phrase(p, additive_ops, COUNT(additive_ops), &len);
    if (!op) op = parser_match_phrase(p, multiplicative_ops, COUNT(multiplicative_ops), &len);
    if (!op) op = parser_match_phrase(p, join_ops, COUNT(join_ops), &len);
    if (!op) return call;
    Span op_span = parser_consume(p, len);
    Expr *right = parse_unary(p);
    const Expr *last_arg = call->as.call.args.items[call->as.call.args.len - 1];
    Span head = {call->pos.span.offset, last_arg->pos.span.offset - call->pos.span.offset};
    SourcePos whole = ast_pos_join(call->pos, right->pos);
    const char *call_text = parser_text(p, call->pos.span);
    const char *op_text = parser_text(p, op_span);
    const char *right_text = parser_text(p, right->pos.span);
    if (parser_error(p, whole.span, "\"%s\" could mean two things.", parser_text(p, whole.span))) {
        diag_note(p->diag, "Use parentheses to say which: \"(%s) %s %s\" or \"%s(%s %s %s)\".", call_text, op_text,
                  right_text, parser_text(p, head), parser_text(p, last_arg->pos.span), op_text, right_text);
    }
    return ast_new_expr(p->arena, EXPR_ERROR, whole);
}

// NAME using ARG, ARG, ...  Each argument is a unary expression.
static Expr *parse_call(Parser *p, const Token *name) {
    parser_advance(p);  // "using"
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, parser_token_pos(name));
    call->as.call.name = name->text;
    do {
        Expr *arg = parse_unary(p);
        vec_push(p->arena, &call->as.call.args, arg);
        call->pos = ast_pos_join(call->pos, arg->pos);
    } while (parser_at(p, 0, TOK_COMMA) && parser_advance(p));
    return reject_ambiguous_call(p, call);
}

static Expr *parse_word(Parser *p) {
    const Token *token = parser_peek(p, 0);
    const char *word = token->text;
    if (strcmp(word, "yes") == 0 || strcmp(word, "true") == 0 || strcmp(word, "no") == 0 ||
        strcmp(word, "false") == 0) {
        Expr *expr = literal(p, EXPR_BOOLEAN);
        expr->as.boolean = word[0] == 'y' || word[0] == 't';
        return expr;
    }
    if (strcmp(word, "nothing") == 0) return literal(p, EXPR_NOTHING);
    if (strcmp(word, "it") == 0) return literal(p, EXPR_IT);
    if (parser_match_words(p, "length of")) return parse_prefixed(p, EXPR_LENGTH, 2);
    if (parser_match_words(p, "contents of file")) return parse_prefixed(p, EXPR_FILE_CONTENTS, 3);
    if (parser_match_words(p, "contents of")) {
        Span span = parser_consume(p, 2);
        if (parser_error(p, span, "Something is missing after \"%s\".", parser_text(p, span))) {
            diag_note(p->diag, "Did you mean \"contents of file\"?");
        }
        return parser_error_expr(p);
    }
    if (parser_is_operator_word(word)) return expected_value(p);

    parser_advance(p);
    if (parser_at_word(p, 0, "using")) return parse_call(p, token);
    Expr *expr = ast_new_expr(p->arena, EXPR_NAME, parser_token_pos(token));
    expr->as.name = word;
    return expr;
}

static Expr *parse_primary(Parser *p) {
    switch (parser_peek(p, 0)->kind) {
    case TOK_NUMBER: return parse_number(p);
    case TOK_STRING: return parse_text(p);
    case TOK_LPAREN: return parse_group(p);
    case TOK_WORD: return parse_word(p);
    default: return expected_value(p);
    }
}
