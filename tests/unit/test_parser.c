#include <string.h>
#include "front/parse.h"
#include "unit.h"

typedef struct {
    const char *source;
    Diag *diag;
    Block *program;
} Parsed;

static Parsed parse_source(TestContext *t, const char *source) {
    Parsed parsed;
    parsed.source = source;
    parsed.diag = diag_new(t->arena, source, strlen(source));
    TokenList tokens = lex(t->arena, parsed.diag, source, strlen(source));
    parsed.program = parse_program(t->arena, parsed.diag, source, &tokens);
    return parsed;
}

static const char *slice(TestContext *t, const char *source, Span span) {
    char *text = arena_alloc(t->arena, span.length + 1);
    memcpy(text, source + span.offset, span.length);
    return text;
}

// The value of the first statement, which these tests write as "say E".
static Expr *first_expr(const Parsed *parsed) {
    return parsed->program->items[0]->as.value;
}

static void test_operator_span_covers_phrase(TestContext *t) {
    Parsed parsed = parse_source(t, "say score is greater than or equal to 10");
    Expr *expr = first_expr(&parsed);
    CHECK(t, expr->kind == EXPR_BINARY);
    CHECK_STR(t, slice(t, parsed.source, expr->as.binary.op_span), "is greater than or equal to");
    CHECK_STR(t, slice(t, parsed.source, expr->pos.span), "score is greater than or equal to 10");
}

static void test_group_span_includes_parentheses(TestContext *t) {
    Parsed parsed = parse_source(t, "say (1 plus 2) times 3");
    Expr *expr = first_expr(&parsed);
    CHECK_STR(t, slice(t, parsed.source, expr->as.binary.left->pos.span), "(1 plus 2)");
    CHECK_STR(t, slice(t, parsed.source, expr->pos.span), "(1 plus 2) times 3");
}

static void test_call_span_and_args(TestContext *t) {
    Parsed parsed = parse_source(t, "say greet using \"Ada\", 2");
    Expr *expr = first_expr(&parsed);
    CHECK(t, expr->kind == EXPR_CALL);
    CHECK_STR(t, expr->as.call.name, "greet");
    CHECK_SIZE(t, expr->as.call.args.len, 2);
    CHECK_STR(t, slice(t, parsed.source, expr->pos.span), "greet using \"Ada\", 2");
}

static void test_conversion_span(TestContext *t) {
    Parsed parsed = parse_source(t, "say x as a number");
    Expr *expr = first_expr(&parsed);
    CHECK(t, expr->kind == EXPR_CONVERT);
    CHECK_STR(t, slice(t, parsed.source, expr->as.convert.op_span), "as a number");
    CHECK_STR(t, slice(t, parsed.source, expr->pos.span), "x as a number");
}

static void test_one_error_per_sentence(TestContext *t) {
    Parsed parsed = parse_source(t, "say 1 plus plus plus\nsay x is greater 1. say y is less 2.\nsay fine\n");
    CHECK_SIZE(t, diag_count(parsed.diag), 3);
    CHECK_SIZE(t, parsed.program->len, 1);  // statements with errors are left out
}

static void test_every_node_has_a_position(TestContext *t) {
    Parsed parsed = parse_source(t, "say not (x is 1) or length of y as text followed by z\n");
    Expr *stack[32];
    size_t depth = 0;
    stack[depth++] = first_expr(&parsed);
    int ok = 1;
    while (depth > 0) {
        Expr *e = stack[--depth];
        if (e->pos.line == 0 || e->pos.column == 0) ok = 0;
        if (e->kind == EXPR_BINARY) {
            stack[depth++] = e->as.binary.left;
            stack[depth++] = e->as.binary.right;
        } else if (e->kind == EXPR_UNARY) {
            stack[depth++] = e->as.unary.operand;
        } else if (e->kind == EXPR_CONVERT) {
            stack[depth++] = e->as.convert.operand;
        } else if (e->kind == EXPR_LENGTH) {
            stack[depth++] = e->as.operand;
        }
    }
    CHECK(t, ok);
    CHECK_SIZE(t, diag_count(parsed.diag), 0);
}

static void test_statement_spans(TestContext *t) {
    Parsed parsed = parse_source(t, "please Display the total plus 1. ask \"Hi\" and call the answer name\n");
    CHECK_SIZE(t, parsed.program->len, 2);
    Stmt *say = parsed.program->items[0];
    CHECK(t, say->kind == STMT_SAY);
    CHECK_STR(t, slice(t, parsed.source, say->verb), "Display");
    CHECK_STR(t, slice(t, parsed.source, say->pos.span), "Display the total plus 1");
    Stmt *ask = parsed.program->items[1];
    CHECK(t, ask->kind == STMT_ASK);
    CHECK_STR(t, ask->as.ask.answer.text, "name");
    CHECK_STR(t, slice(t, parsed.source, ask->as.ask.answer.pos.span), "name");
    CHECK_STR(t, slice(t, parsed.source, ask->pos.span), "ask \"Hi\" and call the answer name");
}

static void test_change_ops(TestContext *t) {
    Parsed parsed = parse_source(t, "add 1 to x. subtract 1 from x. increase x by 1. decrease x by 1. "
                                    "multiply x by 2. divide x by 2.");
    ChangeOp want[] = {CHANGE_ADD, CHANGE_SUBTRACT, CHANGE_ADD, CHANGE_SUBTRACT, CHANGE_MULTIPLY, CHANGE_DIVIDE};
    CHECK_SIZE(t, parsed.program->len, 6);
    for (size_t i = 0; i < parsed.program->len && i < 6; i++) {
        CHECK(t, parsed.program->items[i]->kind == STMT_CHANGE);
        CHECK(t, parsed.program->items[i]->as.change.op == want[i]);
        CHECK_STR(t, parsed.program->items[i]->as.change.target.text, "x");
    }
}

void parser_tests(TestRunner *runner) {
    unit_run(runner, "parser/operator-span", test_operator_span_covers_phrase);
    unit_run(runner, "parser/group-span", test_group_span_includes_parentheses);
    unit_run(runner, "parser/call", test_call_span_and_args);
    unit_run(runner, "parser/conversion-span", test_conversion_span);
    unit_run(runner, "parser/error-per-sentence", test_one_error_per_sentence);
    unit_run(runner, "parser/positions", test_every_node_has_a_position);
    unit_run(runner, "parser/statement-spans", test_statement_spans);
    unit_run(runner, "parser/change-ops", test_change_ops);
}
