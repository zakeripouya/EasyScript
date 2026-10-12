// Checking loops: a count's number, the loop's own values (checked outside
// it), a "for each" list (or the plural's, "for each score:"), and the body,
// whose names exist only inside it.

#include <stdlib.h>
#include <string.h>
#include "front/check_internal.h"

// A count's number; returns its kind (a number).
static TypeVar check_loop_variable(Checker *c, const Stmt *stmt) {
    const Name *var = &stmt->as.loop.var;
    TypeVar number = types_new(c, TYPE_NUMBER, var->pos.line);
    const Symbol *existing = checker_find_made(c, var->text);
    if (!existing && checker_clashes(c, var)) return number;
    if (!existing) {
        Symbol symbol = {var->text, var->display, var->pos.line, NULL, true, number, false,
                         c->function ? c->function->definition : NULL, 0};
        vec_push(c->arena, &c->made, symbol);
        return number;
    }
    if (stmt->as.loop.var_named) {
        diag_error(c->diag, var->pos.span, "You already made \"%s\" on line %zu.", existing->display, existing->line);
    } else {
        diag_error(c->diag, var->pos.span, "This loop calls each number \"number\", but you already made "
                                           "\"number\" on line %zu.", existing->line);
    }
    diag_note(c->diag, "Give this loop's number its own name, like \"count from 1 to 10 as n\".");
    return number;
}

// The loop's own values are checked outside it; its number and anything
// made in its body exist only inside.
static void check_loop_value(Checker *c, const Expr *value, Type type, const char *message, const char *hint) {
    if (value) checker_require(c, checker_check_expr(c, value), type, value, message, hint);
}

// A number written out, maybe with a "-" before it: its value and text.
static const char *written_number(Checker *c, const Expr *expr, double *out) {
    bool negative = expr && expr->kind == EXPR_UNARY && expr->as.unary.op == UNARY_NEGATE;
    if (negative) expr = expr->as.unary.operand;
    if (!expr || expr->kind != EXPR_NUMBER) return NULL;
    *out = strtod(expr->as.number.text, NULL) * (negative ? -1 : 1);
    return negative ? arena_sprintf(c->arena, "-%s", expr->as.number.text) : expr->as.number.text;
}

// "count from 10 to 1": counting goes up, so this never runs.
static void warn_never_counts(Checker *c, const Stmt *stmt) {
    if (stmt->as.loop.kind != LOOP_COUNT || stmt->as.loop.down) return;
    double from, to;
    const char *from_text = written_number(c, stmt->as.loop.from, &from);
    const char *to_text = written_number(c, stmt->as.loop.to, &to);
    if (!from_text || !to_text || !(from > to)) return;
    size_t start = stmt->as.loop.from->pos.span.offset;
    Span span = {start, stmt->as.loop.to->pos.span.offset + stmt->as.loop.to->pos.span.length - start};
    diag_warning(c->diag, span, "This count never runs: it starts above where it ends, and counting goes up.");
    diag_note(c->diag, "To count down, write \"count down from %s to %s\".", from_text, to_text);
    diag_info(c->diag, DIAG_MISSING, "\"down\"", NULL);
    diag_choice(c->diag, "count down from %s to %s:", from_text, to_text);
}

void checker_check_loop(Checker *c, const Stmt *stmt) {
    // A check made before the loop doesn't hold in later rounds if the body
    // sets what was checked; the loop's own condition is checked every round.
    size_t checked = c->present.len;
    checker_kill_set_in(c, &stmt->as.loop.body);
    warn_never_counts(c, stmt);
    check_loop_value(c, stmt->as.loop.from, TYPE_NUMBER, "A count has to start at a number", NULL);
    check_loop_value(c, stmt->as.loop.to, TYPE_NUMBER, "A count has to end at a number", NULL);
    check_loop_value(c, stmt->as.loop.step, TYPE_NUMBER, "The step of a count has to be a number", NULL);
    check_loop_value(c, stmt->as.loop.times, TYPE_NUMBER, "The number of times has to be a number", NULL);
    check_loop_value(c, stmt->as.loop.condition, TYPE_YESNO,
                     "A loop needs yes or no to decide whether to keep going",
                     "Compare it with something, like \"while count is less than 10\".");
    TypeVar item = 0, value = 0;
    const Expr *implied = stmt->as.loop.implied ? stmt->as.loop.list : NULL;  // "for each score:" and "scores"
    if (implied && !checker_find_made(c, implied->as.name) && !checker_find_constant(c, implied->as.name)) {
        diag_error(c->diag, stmt->as.loop.var.pos.span, "There's no list called \"%s\" for this to go through.",
                   implied->display);
        diag_note(c->diag, "\"for each %s:\" goes through the list called \"%s\". Make that list first, or say which "
                           "list to go through, like \"for each %s in groceries:\".",
                  stmt->as.loop.var.display, implied->display, stmt->as.loop.var.display);
        diag_info(c->diag, DIAG_MISSING, "the list to go through", "a list");
        for (size_t i = c->floor; i < c->made.len; i++) {  // the lists it could mean
            if (type_is_list(types_of(c, c->made.items[i].var))) {
                diag_choice(c->diag, "for each %s in %s:", stmt->as.loop.var.display, c->made.items[i].display);
            }
        }
        item = types_new(c, TYPE_ERROR, stmt->pos.line);
    } else if (stmt->as.loop.list) {
        item = checker_check_each(c, stmt, &value);
    }
    size_t outer = c->made.len;
    Loop loop = {stmt->as.loop.kind == LOOP_COUNT || stmt->as.loop.kind == LOOP_TIMES, 0, false, NO_VARIANT, false};
    if (stmt->as.loop.kind == LOOP_COUNT) loop.it = check_loop_variable(c, stmt);
    if (stmt->as.loop.kind == LOOP_TIMES) loop.it = types_new(c, TYPE_NUMBER, stmt->pos.line);
    if (stmt->as.loop.kind == LOOP_EACH) {
        checker_make_item_name(c, &stmt->as.loop.var, item);
        if (stmt->as.loop.pair) checker_make_item_name(c, &stmt->as.loop.value_name, value);
        loop.has_it = true;
        loop.it = item;
    }
    vec_push(c->arena, &c->loops, loop);
    if (stmt->as.loop.condition) checker_narrow_condition(c, stmt->as.loop.condition, stmt->as.loop.kind == LOOP_WHILE);
    c->blocks++;
    checker_check_statements(c, &stmt->as.loop.body);
    c->blocks--;
    c->loops.len--;
    c->present.len = checked;
    checker_end_scope(c, outer, stmt->pos.line, "the loop");
}
