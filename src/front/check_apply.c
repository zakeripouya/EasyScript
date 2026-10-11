// The built-ins that take a function: "F applied to every X in L", "the Xs
// in L that pass F", "L combined using F", "L sorted by F", and the
// sentences that change a list with them ("apply F to every X in L", "keep
// only the Xs in L where ...", "sort L by F"). The function's kind is
// required like a call's: its inputs are the list's items (and, when
// combining, what's combined so far).

#include <string.h>
#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

static const char *const explanations[] = {
    "It's given each item, and what it gives back goes in the new list.",
    "It's given each item, and the items it gives back yes for are kept.",
    "It's given what's combined so far and the next item, and gives back what they combine into.",
    "It's given each item, and the items are put in the order of what it gives back.",
};

// A function's name given to a built-in is the function itself ("using
// double"), not a call of it: it becomes "the function double".
static TypeVar check_function_operand(Checker *c, Expr *f) {
    if (f->kind == EXPR_NAME && !checker_find_made(c, f->as.name) && checker_find_function(c, f->as.name)) {
        const char *name = f->as.name;
        f->kind = EXPR_FUNCTION_REF;
        f->as.ref.name = name;
        f->as.ref.target = NULL;
    }
    return checker_check_expr(c, f);
}

// The function has to take `n` inputs of the kinds in `params` and give back `result`.
static void require_function(Checker *c, const Expr *expr, TypeVar f, const TypeVar *params, size_t n,
                             TypeVar result) {
    const Expr *at = expr->as.apply.function;
    Type type = types_of(c, f);
    if (type == TYPE_ERROR) return;
    TypeVar expected = types_new_func(c, params, n, result, at->pos.line);
    const char *wanted = checker_kind(c, expected);
    if ((type == TYPE_UNKNOWN || type_is_func(type)) && types_unify(c, f, expected)) return;
    diag_error(c->diag, at->pos.span, "This has to be %s, but it's %s.", wanted, checker_kind(c, f));
    diag_note(c->diag, "%s", explanations[expr->as.apply.kind]);
}

// What the function gives back has to be a value (not nothing).
static void require_value(Checker *c, const Expr *expr, TypeVar result) {
    types_defer_one_of(c, expr->as.apply.function, result, result,
                       KIND_NUMBER | KIND_TEXT | KIND_YESNO | KIND_LIST | KIND_RECORD | KIND_MAP | KIND_OTHER | KIND_DATE,
                       "The function has to give back a value");
}

TypeVar checker_check_apply(Checker *c, const Expr *expr) {
    TypeVar f = check_function_operand(c, expr->as.apply.function);
    const Expr *list_expr = expr->as.apply.list;
    TypeVar list = checker_check_expr(c, list_expr);
    TypeVar item = checker_require_list(c, list, list_expr, "I can only go through the items of a list");
    size_t line = expr->pos.line;
    switch (expr->as.apply.kind) {
    case APPLY_MAP: {
        TypeVar result = types_new(c, TYPE_UNKNOWN, 0);
        require_function(c, expr, f, &item, 1, result);
        require_value(c, expr, result);
        return types_new_list(c, result, line);
    }
    case APPLY_FILTER:
        require_function(c, expr, f, &item, 1, types_new(c, TYPE_YESNO, line));
        return list;
    case APPLY_COMBINE: {
        TypeVar so_far = expr->as.apply.start ? checker_check_value(c, expr->as.apply.start) : item;
        TypeVar inputs[2] = {so_far, item};
        require_function(c, expr, f, inputs, 2, so_far);
        return so_far;
    }
    case APPLY_SORT: {
        TypeVar key = types_new(c, TYPE_UNKNOWN, 0);
        require_function(c, expr, f, &item, 1, key);
        types_defer_one_of(c, expr->as.apply.function, key, key, KIND_NUMBER | KIND_TEXT | KIND_DATE,
                           "To sort by it, the function has to give back numbers or text");
        return list;
    }
    }
    return error_var(c);
}

// "apply F to every X in L", "keep only the Xs in L ...", "sort L by F":
// the list's new items are worked out from it (through its hidden name),
// and have to be the kind it holds.
void checker_check_replace(Checker *c, const Stmt *stmt) {
    const Expr *list_expr = stmt->as.list.list;
    if (checker_reject_fixed(c, list_expr)) return;
    TypeVar list = checker_check_expr(c, list_expr);
    TypeVar item = checker_require_list(c, list, list_expr, "I can only change the items of a list");
    if (types_of(c, list) == TYPE_ERROR) return;
    const Expr *hidden = stmt->as.list.hidden;
    Symbol symbol = {hidden->as.name, hidden->display, hidden->pos.line, NULL, false, list, false,
                     c->function ? c->function->definition : NULL, 0};
    vec_push(c->arena, &c->made, symbol);
    TypeVar value = checker_check_expr(c, stmt->as.list.value);
    c->made.len--;
    if (types_unify(c, list, value) || types_of(c, value) == TYPE_ERROR) return;
    const char *name = list_expr->kind == EXPR_NAME ? arena_sprintf(c->arena, "\"%s\"", list_expr->display) : "It";
    diag_error(c->diag, stmt->as.list.value->as.apply.function->pos.span,
               "%s is %s, so the function applied to its items has to give back %s, but this gives back %s.", name,
               checker_kind(c, list), checker_kind(c, item), checker_kind(c, types_item(c, value)));
    diag_note(c->diag, "To make a new list of what the function gives back, write it like \"double applied to "
                       "every number in scores\" instead.");
}
