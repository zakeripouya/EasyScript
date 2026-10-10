// Text tools: the forms that make new text (or lists of text) from text,
// and the tests on characters. Taking characters and pieces uses the list
// forms with "character" for "item" (parse_list.c); "contains", "starts
// with" and "ends with" are comparisons (parse_expr.c).
//
//   the characters of word        the words of text       the lines of text
//   the code of letter            the character with code 65
//   the position of "x" in word   (also of an item in a list)
//
//   word in uppercase             word in lowercase       text without extra spaces
//   text split by ","             names joined with ", "  text with "a" replaced by "b"
//
//   letter is a letter            letter is a digit       letter is not a space

#include <string.h>
#include "front/parse_internal.h"

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

// --- Prefix forms --------------------------------------------------------------

static Expr *new_text_op(Parser *p, TextOp op, SourcePos start, Span op_span, Expr *operand) {
    Expr *expr = ast_new_expr(p->arena, EXPR_TEXT_OP, ast_pos_join(start, operand->pos));
    expr->as.text_op.op = op;
    expr->as.text_op.op_span = op_span;
    expr->as.text_op.operand = operand;
    return expr;
}

// "the words of T", "the code of T", ...: `words` words, then a single value.
static Expr *parse_prefix(Parser *p, size_t words, TextOp op) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op_span = parser_consume(p, words);
    return new_text_op(p, op, start, op_span, parse_primary(p));
}

// the position of X in T: X and T are single values.
static Expr *parse_position_of(Parser *p) {
    static const char *const in[] = {"in", NULL};
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op_span = parser_consume(p, 2);  // position of
    Expr *wanted = parse_primary(p);
    if (p->sentence_failed || !parser_expect_word(p, in, "the position of \"x\" in word")) return parser_error_expr(p);
    Expr *expr = new_text_op(p, TEXT_POSITION, start, op_span, parse_primary(p));
    expr->as.text_op.arg = wanted;
    return expr;
}

Expr *parser_parse_text_word(Parser *p) {
    if (parser_match_words(p, "characters of")) return parse_prefix(p, 2, TEXT_CHARACTERS);
    if (parser_match_words(p, "words of")) return parse_prefix(p, 2, TEXT_WORDS);
    if (parser_match_words(p, "lines of")) return parse_prefix(p, 2, TEXT_LINES);
    if (parser_match_words(p, "code of")) return parse_prefix(p, 2, TEXT_CODE);
    if (parser_match_words(p, "character with code")) return parse_prefix(p, 3, TEXT_FROM_CODE);
    if (parser_match_words(p, "position of")) return parse_position_of(p);
    return NULL;
}

// --- Postfix forms -------------------------------------------------------------

// "with", a single value, then "replaced" (and "by", or an error).
bool parser_at_replace(const Parser *p) {
    if (!parser_at_word(p, 0, "with")) return false;
    size_t n = parser_value_length(p, 1);
    return n > 0 && parser_at_word(p, 1 + n, "replaced");
}

static const Phrase text_postfix[] = {
    {"without extra spaces", "without extra spaces", TEXT_TRIM, false},
    {"split by", "split by", TEXT_SPLIT, false},
    {"joined with", "joined with", TEXT_JOINED, false},
};

// T with X replaced by Y
static Expr *parse_replace(Parser *p, Expr *text) {
    static const char *const by[] = {"by", NULL};
    Span op_span = parser_consume(p, 1);  // with
    Expr *expr = new_text_op(p, TEXT_REPLACE, text->pos, op_span, text);
    expr->as.text_op.arg = parse_primary(p);
    parser_consume(p, 1);  // replaced (parser_at_replace saw it)
    if (p->sentence_failed || !parser_expect_word(p, by, "word with \"a\" replaced by \"b\"")) return parser_error_expr(p);
    expr->as.text_op.arg2 = parse_primary(p);
    expr->pos = ast_pos_join(text->pos, expr->as.text_op.arg2->pos);
    return expr;
}

Expr *parser_parse_text_postfix(Parser *p, Expr *expr) {
    // "in" alone is never matched as part of a phrase: "count from 1 to n in steps of 2" keeps it.
    bool upper = parser_match_words(p, "in uppercase") > 0;
    if (upper || parser_match_words(p, "in lowercase")) {
        Span op_span = parser_consume(p, 2);
        Expr *result = new_text_op(p, upper ? TEXT_UPPERCASE : TEXT_LOWERCASE, expr->pos, op_span, expr);
        result->pos = ast_pos_join(expr->pos, (SourcePos){op_span, 0, 0});
        return result;
    }
    if (parser_at_replace(p)) return parse_replace(p, expr);
    size_t len;
    const Phrase *op = parser_match_phrase(p, text_postfix, COUNT(text_postfix), &len);
    if (!op) return NULL;
    Span op_span = parser_consume(p, len);
    Expr *result = new_text_op(p, (TextOp)op->value, expr->pos, op_span, expr);
    result->pos = ast_pos_join(expr->pos, (SourcePos){op_span, 0, 0});
    if (op->value == TEXT_TRIM) return result;
    result->as.text_op.arg = parse_primary(p);
    result->pos = ast_pos_join(expr->pos, result->as.text_op.arg->pos);
    return result;
}

// --- Tests on characters -----------------------------------------------------------

// The word after "is" was written with "a" before it ("is a letter"); "is
// letter" or "is the letter" compares with a name instead.
static bool after_a(const Parser *p, const Token *token) {
    return token->filler.length == 1 && (p->source[token->filler.offset] | 0x20) == 'a';
}

Expr *parser_parse_char_test(Parser *p, Expr *left) {
    static const struct {
        const char *word;
        TextOp op;
    } tests[] = {{"letter", TEXT_IS_LETTER}, {"digit", TEXT_IS_DIGIT}, {"space", TEXT_IS_SPACE}};
    size_t verb = parser_match_words(p, "is not") ? 2 : parser_at_word(p, 0, "is") || parser_at_word(p, 0, "isn't") ? 1 : 0;
    if (verb == 0) return NULL;
    const Token *word = parser_peek(p, verb);
    for (size_t i = 0; i < COUNT(tests); i++) {
        if (!parser_at_word(p, verb, tests[i].word) || !after_a(p, word)) continue;
        bool negated = verb == 2 || parser_at_word(p, 0, "isn't");
        Span op_span = parser_consume(p, verb + 1);
        Expr *expr = new_text_op(p, tests[i].op, left->pos, op_span, left);
        expr->pos = ast_pos_join(left->pos, (SourcePos){op_span, 0, 0});
        expr->as.text_op.negated = negated;
        return expr;
    }
    return NULL;
}
