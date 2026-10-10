// Values: the tightest level of the expression grammar. Literals, names,
// calls (NAME of A and B), groups in parentheses, the prefix forms ("length
// of X", "contents of file X"), and the list forms that start with a word
// (parse_list.c, and "only" in parse_expr.c). The operators around them are
// in parse_expr.c.

#include <string.h>
#include "front/parse_internal.h"

static Expr *expected_value(Parser *p) {
    if (parser_reject_bare_decimal(p)) return parser_error_expr(p);
    const Token *token = parser_peek(p, 0);
    bool after_open_paren = p->pos > 0 && p->tokens[p->pos - 1].kind == TOK_LPAREN;
    // A ":" or "," straight after an operator ("if x plus:") also means the value is missing.
    bool ends_here = parser_at_sentence_end(p) || token->kind == TOK_COLON || token->kind == TOK_COMMA;
    if (ends_here || (token->kind == TOK_RPAREN && after_open_paren)) {
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
    bool as_ends_value = p->as_ends_value;  // inside parentheses, "as" converts again
    p->as_ends_value = false;
    Expr *inner = parse_expression(p);
    p->as_ends_value = as_ends_value;
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

static bool is_parenthesized(const Parser *p, const Expr *expr) {
    Span span = expr->pos.span;
    return span.length >= 2 && p->source[span.offset] == '(' && p->source[span.offset + span.length - 1] == ')';
}

// An argument directly followed by arithmetic could belong to the call or
// to the whole sentence ("double using 21 plus 1"), so that's an error.
// A last argument in parentheses is already closed, so then the arithmetic
// belongs outside: "fib of (n minus 1) plus 1".
static Expr *reject_ambiguous_call(Parser *p, Expr *call) {
    if (is_parenthesized(p, call->as.call.args.items[call->as.call.args.len - 1])) return call;
    size_t len;
    if (!parser_match_arithmetic(p, &len)) return call;
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

static bool at_argument_separator(const Parser *p, bool commas) {
    if (parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call")) return true;
    return commas && parser_at(p, 0, TOK_COMMA);
}

void parser_parse_call_args(Parser *p, Expr *call, bool commas) {
    do {
        Expr *arg = parse_unary(p);
        vec_push(p->arena, &call->as.call.args, arg);
        call->pos = ast_pos_join(call->pos, arg->pos);
    } while (at_argument_separator(p, commas) && parser_advance(p));
}

Expr *parser_finish_call(Parser *p, Expr *call) {
    return call->as.call.args.len > 0 ? reject_ambiguous_call(p, call) : call;
}

// NAME of A and B / NAME with A and B / NAME using A, B (and NAME to A for a
// function defined with "to"). Each argument is a unary expression.
static Expr *parse_call(Parser *p, const Name *name) {
    const Token *intro = parser_advance(p);  // "of", "with", "using", or "to"
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, name->pos);
    call->as.call.name = name->text;
    call->display = name->display;
    parser_parse_call_args(p, call, strcmp(intro->text, "using") == 0);
    return parser_finish_call(p, call);
}

// A name (the longest known one, if several words start one), or a call.
static Expr *parse_name_or_call(Parser *p) {
    const KnownName *known = parser_match_name(p, 0, false);
    Name name;
    parser_take_name(p, known ? known->words : 1, &name);
    if (parser_at_call_intro(p, name.text)) return parse_call(p, &name);
    Expr *expr = ast_new_expr(p->arena, EXPR_NAME, name.pos);
    expr->as.name = name.text;
    expr->display = name.display;
    return expr;
}

// "every NAME in" anywhere but the start of a value.
static Expr *reject_inner_every(Parser *p) {
    Span span = parser_advance(p)->span;
    if (parser_error(p, span, "\"every\" has to start the value, because everything after it is worked out for "
                              "each item.")) {
        diag_note(p->diag, "Put it in parentheses, like \"(every score in scores plus 5)\".");
    }
    return parser_error_expr(p);
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
    const KnownName *known = parser_match_name(p, 0, false);
    if (known && known->words > 1) return parse_name_or_call(p);  // "late fee", before any other meaning
    Expr *list = parser_parse_list_word(p);
    if (list) return list;
    if (parser_at_only(p)) return parser_parse_only(p);
    if (parser_at_every(p)) return reject_inner_every(p);

    return parse_name_or_call(p);
}

Expr *parse_primary(Parser *p) {
    if (p->pending_item) {  // "every NAME in L ...": the item comes first
        Expr *item = p->pending_item;
        p->pending_item = NULL;
        return item;
    }
    switch (parser_peek(p, 0)->kind) {
    case TOK_NUMBER: return parse_number(p);
    case TOK_STRING: return parse_text(p);
    case TOK_LPAREN: return parse_group(p);
    case TOK_WORD: return parse_word(p);
    default: return expected_value(p);
    }
}
