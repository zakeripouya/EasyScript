// Functions as values:
//
//   the function double                     a function defined with "to", not called
//   a function taking x that gives back x times 2
//   a function taking word (text):          the same with a block below it, which
//       give back word followed by "!".     ends the sentence
//   a function that gives back 5            (no inputs)
//   call f with 3 / call f                  calling a function kept as a value
//
// A function made inside a value is a STMT_FUNCTION like one defined with
// "to" (function.lambda set), so the checker and the code generator treat
// both alike. "f with 3" (a name followed by its inputs) is parsed as any
// call; the checker finds out that f holds a function.

#include <string.h>
#include "front/parse_internal.h"

static const char *const lambda_example = "a function taking x that gives back x times 2";

static bool at_giving_back(const Parser *p, size_t ahead) {
    return parser_at_word(p, ahead, "giving") && parser_at_word(p, ahead + 1, "back");
}

// What can follow "function" in a function made inside a value.
static bool at_lambda_start(const Parser *p, size_t ahead) {
    return parser_at_word(p, ahead, "taking") || parser_match_words_at(p, ahead, "that gives back") ||
           at_giving_back(p, ahead) || parser_at(p, ahead, TOK_COLON) ||
           (parser_at(p, ahead, TOK_COMMA) && at_giving_back(p, ahead + 1));
}

bool parser_at_function_value(const Parser *p) {
    if (!parser_at_word(p, 0, "function")) return false;
    return parser_match_name(p, 1, true) != NULL || at_lambda_start(p, 1);
}

// taking P [(KIND)] [and Q [(KIND)] ...]
static bool parse_lambda_inputs(Parser *p, Stmt *def) {
    static const char *const after[] = {"and", "that", "giving", "gives", NULL};
    parser_advance(p);  // taking
    do {
        Name param = {0};
        Type declared = TYPE_UNKNOWN;
        if (!parser_parse_name(p, &param, after)) return false;
        if (parser_at(p, 0, TOK_LPAREN) && !parser_parse_param_kind(p, &declared)) return false;
        vec_push(p->arena, &def->as.function.params, param);
        vec_push(p->arena, &def->as.function.declared, declared);
    } while (parser_at_word(p, 0, "and") && parser_advance(p));
    return true;
}

static bool report_lambda_end(Parser *p) {
    const Token *token = parser_peek(p, 0);
    bool reported = parser_at_sentence_end(p)
                        ? parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))
                        : parser_error(p, token->span, "I expected \"that gives back\" or a colon here, but found %s.",
                                       parser_describe(p, token));
    if (reported) {
        diag_note(p->diag, "Write it like \"%s\", or end the line with a colon and write the function's sentences "
                           "on the indented lines below.", lambda_example);
    }
    return false;
}

// "that gives back E": the function's body is "give back E".
static void parse_one_line_body(Parser *p, Stmt *def) {
    const Token *give = parser_peek(p, 1);
    parser_consume(p, 3);  // that gives back
    Stmt *ret = parser_new_stmt(p, STMT_RETURN, give);
    ret->as.returned = parse_expression(p);
    ret->pos = ast_pos_join(ret->pos, ret->as.returned->pos);
    vec_push(p->arena, &def->as.function.body, ret);
}

// [, giving back KIND]: then a block, which ends the sentence the value is in.
static bool parse_block_body(Parser *p, Stmt *def) {
    size_t skip = parser_at(p, 0, TOK_COMMA) && at_giving_back(p, 1) ? 1 : 0;
    if (at_giving_back(p, skip)) {
        parser_consume(p, skip + 2);
        if (!parser_parse_kind(p, true, &def->as.function.declared_result)) return false;
    }
    if (!parser_at(p, 0, TOK_COLON)) return report_lambda_end(p);
    SourcePos header = def->pos;
    bool ok = parser_parse_block(p, &def->as.function.body, &header, NULL, "this function");
    def->pos = header;
    p->ended_with_block = true;
    p->block_end = p->pos + 1;
    return ok;
}

static Expr *parse_lambda(Parser *p) {
    const Token *word = parser_advance(p);  // function
    Stmt *def = parser_new_stmt(p, STMT_FUNCTION, word);
    def->as.function.lambda = true;
    def->as.function.name.text = arena_sprintf(p->arena, "0l%zu", ++p->lambdas);
    def->as.function.name.display = arena_sprintf(p->arena, "the function made on line %zu", def->pos.line);
    def->as.function.name.pos = def->pos;
    bool ok = !parser_at_word(p, 0, "taking") || parse_lambda_inputs(p, def);
    if (ok && parser_match_words(p, "that gives back")) {
        parse_one_line_body(p, def);
    } else if (ok) {
        ok = parse_block_body(p, def);
    }
    if (!ok) {
        if (!p->ended_with_block) {  // a broken header: skip its line and block
            parser_skip_line_and_block(p);
            p->ended_with_block = true;
            p->block_end = p->pos + 1;
        }
        return parser_error_expr(p);
    }
    Expr *expr = ast_new_expr(p->arena, EXPR_LAMBDA, def->pos);
    expr->pos = ast_pos_join(def->pos, (SourcePos){p->prev_span, 0, 0});
    expr->as.lambda = def;
    return expr;
}

Expr *parser_parse_function_value(Parser *p) {
    if (at_lambda_start(p, 1)) return parse_lambda(p);
    SourcePos start = parser_token_pos(parser_advance(p));  // function
    Expr *expr = ast_new_expr(p->arena, EXPR_FUNCTION_REF, start);
    Name name;
    parser_take_name(p, parser_match_name(p, 0, true)->words, &name);
    expr->as.ref.name = name.text;
    expr->display = name.display;
    expr->pos = ast_pos_join(start, name.pos);
    return expr;
}

// --- Calling a function value -------------------------------------------------------

bool parser_parse_called(Parser *p, Expr *call) {
    bool no_call = p->no_call;
    p->no_call = true;  // "call handler with 3": handler is what's called, not called with 3 itself
    Expr *callee = parse_primary(p);
    p->no_call = no_call;
    if (callee->kind == EXPR_ERROR) return false;
    call->pos = callee->pos;
    if (callee->kind == EXPR_NAME) {
        call->as.call.name = callee->as.name;
        call->display = callee->display;
    } else {
        call->as.call.callee = callee;
    }
    if (!parser_at_word(p, 0, "with") && !parser_at_word(p, 0, "of") && !parser_at_word(p, 0, "using")) return true;
    bool commas = parser_at_word(p, 0, "using");
    parser_advance(p);
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "Add the values to give it, like \"call handler with 3\", or leave out \"%s\".",
                      parser_text(p, p->prev_span));
        }
        return false;
    }
    parser_parse_call_args(p, call, commas);
    return true;
}

bool parser_at_call_value(const Parser *p) {
    if (!parser_at_word(p, 0, "call")) return false;
    const Token *next = parser_peek(p, 1);
    return next->kind == TOK_LPAREN || parser_is_name_word(next);
}

Expr *parser_parse_call_value(Parser *p) {
    SourcePos start = parser_token_pos(parser_advance(p));  // call
    const KnownName *function = parser_match_name(p, 0, true);
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, start);
    if (function) {  // "call greet with ...": a function defined with "to"
        bool no_call = p->no_call;
        p->no_call = false;
        Expr *named = parse_primary(p);
        p->no_call = no_call;
        if (named->kind == EXPR_NAME) {  // no inputs
            call->as.call.name = named->as.name;
            call->display = named->display;
            named = call;
        }
        named->pos = ast_pos_join(start, named->pos);
        return named;
    }
    if (!parser_parse_called(p, call)) return parser_error_expr(p);
    call->pos = ast_pos_join(start, call->pos);
    return parser_finish_call(p, call);
}
