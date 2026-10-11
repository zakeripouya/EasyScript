// Functions as values: "the function double", functions made inside values
// ("a function taking x that gives back x times 2"), calling a function kept
// as a value ("call f with 3", "f with 3"), and the names such functions use
// from around them (captures).
//
// A function's kind is a type variable like any other: "a function taking"
// the kinds of its inputs "and giving back" its result (check_types.c), so
// inputs and results are inferred from how the function is made and how
// it's called.
//
// A function made inside another one sees every name made around it so far.
// Each one it uses is captured: kept in a cell (es_closure.h) that the code
// around it and the function share, so a change either makes is seen by both.

#include <string.h>
#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

const char *checker_capital(Checker *c, const char *text) {
    char *copy = arena_strdup(c->arena, text);
    if (copy[0] >= 'a' && copy[0] <= 'z') copy[0] = (char)(copy[0] - 'a' + 'A');
    return copy;
}

const char *checker_function_label(Checker *c, const Function *f) {
    if (f->definition->as.function.lambda) return f->display;
    return arena_sprintf(c->arena, "\"%s\"", f->display);
}

// --- Captures ------------------------------------------------------------------------

static void add_name(Arena *arena, NameList *names, const char *name) {
    for (size_t i = 0; i < names->len; i++) {
        if (strcmp(names->items[i], name) == 0) return;
    }
    vec_push(arena, names, name);
}

static void add_capture(Checker *c, Stmt *def, const Symbol *symbol) {
    for (size_t i = 0; i < def->as.function.captures.len; i++) {
        if (strcmp(def->as.function.captures.items[i].name, symbol->name) == 0) return;
    }
    Capture capture = {symbol->name, TYPE_UNKNOWN, symbol->var};
    vec_push(c->arena, &def->as.function.captures, capture);
}

void checker_use(Checker *c, const Symbol *symbol, bool setting) {
    size_t index = (size_t)(symbol - c->made.items);
    bool captured = false;
    for (LambdaScope *l = c->lambda; l && index < l->scope; l = l->outer) {
        add_capture(c, l->definition, symbol);
        captured = true;
    }
    if (!captured) return;
    Stmt *owner = (Stmt *)symbol->owner;
    add_name(c->arena, owner ? &owner->as.function.cells : &c->program->cells, symbol->name);
    if (setting && !symbol->set_inside) ((Symbol *)symbol)->set_inside = c->lambda->definition->pos.line;
}

bool checker_reject_set_inside(Checker *c, const Expr *expr, const char *path) {
    const Expr *root = expr;
    while (root->kind == EXPR_FIELD) root = root->as.field.record;
    if (root->kind != EXPR_NAME) return false;
    const Symbol *symbol = checker_find_made(c, root->as.name);
    if (!symbol || !symbol->set_inside) return false;
    const char *name = root->display;
    diag_error(c->diag, expr->pos.span, "\"%s\" was checked, but the function made on line %zu can set \"%s\" "
               "whenever it's called, so the check doesn't last.", path, symbol->set_inside, name);
    diag_note(c->diag, "Keep it in a name of its own first, like \"let current %s be %s\", and check that instead.",
              name, name);
    return true;
}

// --- The function double ----------------------------------------------------------------

TypeVar checker_check_function_ref(Checker *c, const Expr *expr) {
    const Function *f = checker_find_function(c, expr->as.ref.name);
    if (!f) return error_var(c);
    if (checker_shared_method(c, f->name)) {
        diag_error(c->diag, expr->pos.span, "Several kinds of record have a method called \"%s\", so I can't tell "
                   "which one this is.", f->display);
        diag_note(c->diag, "Make a function that calls it, like \"a function taking pet:\" with \"%s pet\" indented "
                           "below it; each call picks the method by the kind of record it's given.", f->display);
        return error_var(c);
    }
    ((Expr *)expr)->as.ref.target = f->definition;
    checker_resolved(c, expr, f->definition);
    return types_new_func(c, f->params.items, f->params.len, f->result, expr->pos.line);
}

// --- A function taking x ... ------------------------------------------------------------

// Its inputs and result: written down, or worked out like a "to" function's.
static Function *lambda_function(Checker *c, Stmt *def) {
    Function *f = arena_alloc(c->arena, sizeof(Function));
    f->name = def->as.function.name.text;
    f->display = def->as.function.name.display;
    f->definition = def;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        Type declared = def->as.function.declared.items[i];
        vec_push(c->arena, &f->params, types_new(c, declared, declared ? def->pos.line : 0));
    }
    Type result = def->as.function.declared_result;
    if (result == TYPE_UNKNOWN && !checker_gives_value(&def->as.function.body)) result = TYPE_NOTHING;
    f->result = types_new(c, result, result ? def->pos.line : 0);
    vec_push(c->arena, &c->lambdas, f);
    return f;
}

static void make_inputs(Checker *c, const Function *f) {
    const Stmt *def = f->definition;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        const Name *param = &def->as.function.params.items[i];
        const Symbol *existing = checker_find_made(c, param->text);
        if (existing && (size_t)(existing - c->made.items) >= c->lambda->scope) {
            diag_error(c->diag, param->pos.span, "This function already has an input called \"%s\".", param->display);
            diag_note(c->diag, "Give each input its own name.");
        } else if (existing) {
            diag_error(c->diag, param->pos.span, "You already made \"%s\" on line %zu.", existing->display,
                       existing->line);
            diag_note(c->diag, "The function can use \"%s\" as it is. Give its input another name, like \"new %s\".",
                      existing->display, param->display);
        } else {
            checker_make(c, param, f->params.items[i]);
        }
    }
}

// The body sees the names made around it so far, and its own; loops and
// checks around it don't count (it runs later, when it's called).
TypeVar checker_check_lambda(Checker *c, const Expr *expr) {
    Stmt *def = expr->as.lambda;
    for (size_t i = 0; i < c->loops.len; i++) {
        if (!c->loops.items[i].helper) continue;
        diag_error(c->diag, expr->pos.span, "A function can't be made inside \"every\" or \"only\".");
        diag_note(c->diag, "Make it before, like \"let check be a function taking x that gives back ...\", and use "
                           "that name inside.");
        return error_var(c);
    }
    Function *f = lambda_function(c, def);
    const Function *saved_function = c->function;
    Loop *loops = c->loops.items;
    size_t loops_len = c->loops.len, loops_cap = c->loops.cap;
    Narrowing *present = c->present.items;
    size_t present_len = c->present.len, present_cap = c->present.cap;
    size_t saved_blocks = c->blocks;
    LambdaScope scope = {def, c->made.len, c->lambda, false};
    for (size_t i = 0; i < loops_len; i++) scope.in_loop |= loops[i].has_it;
    c->loops.items = NULL, c->loops.len = c->loops.cap = 0;
    c->present.items = NULL, c->present.len = c->present.cap = 0;
    c->function = f;
    c->lambda = &scope;
    c->blocks = 0;
    make_inputs(c, f);
    checker_check_statements(c, &def->as.function.body);
    c->made.len = scope.scope;
    c->lambda = scope.outer;
    c->function = saved_function;
    c->loops.items = loops, c->loops.len = loops_len, c->loops.cap = loops_cap;
    c->present.items = present, c->present.len = present_len, c->present.cap = present_cap;
    c->blocks = saved_blocks;
    return types_new_func(c, f->params.items, f->params.len, f->result, expr->pos.line);
}

// --- Calling a function value -------------------------------------------------------------

static const char *callee_label(Checker *c, const Expr *callee) {
    if (callee->kind == EXPR_NAME) return arena_sprintf(c->arena, "\"%s\"", callee->display);
    return "This function";
}

static const char *ordinal(Checker *c, size_t i) {
    static const char *const words[] = {"first", "second", "third", "fourth", "fifth",
                                        "sixth", "seventh", "eighth", "ninth", "tenth"};
    return i < 10 ? words[i] : arena_sprintf(c->arena, "input number %zu", i + 1);
}

static TypeVar report_not_function(Checker *c, const Expr *callee, TypeVar var) {
    if (callee->kind == EXPR_NAME) {
        diag_error(c->diag, callee->pos.span, "\"%s\" is %s, not a function, so it can't be called.", callee->display,
                   checker_kind(c, var));
    } else {
        diag_error(c->diag, callee->pos.span, "This is %s, not a function, so it can't be called.", checker_kind(c, var));
    }
    diag_note(c->diag, "Only a function can be called. To keep one in a name, write something like \"let double be "
                       "a function taking x that gives back x times 2\".");
    return error_var(c);
}

static TypeVar report_count(Checker *c, const Expr *expr, TypeVar func, size_t wanted) {
    const Expr *callee = expr->as.call.callee;
    size_t given = expr->as.call.args.len;
    if (wanted == 0) {
        diag_error(c->diag, expr->pos.span, "%s doesn't take any values, but this gives it %zu.",
                   callee_label(c, callee), given);
    } else {
        diag_error(c->diag, expr->pos.span, "%s takes %zu value%s, but this gives it %zu.", callee_label(c, callee),
                   wanted, wanted == 1 ? "" : "s", given);
    }
    diag_note(c->diag, "It's %s.", checker_kind(c, func));
    return error_var(c);
}

// The inputs (checked already) against the function's, then what it gives back.
static TypeVar finish_value_call(Checker *c, const Expr *expr, TypeVar func, const TypeVar *args, bool as_value) {
    const TypeVar *params;
    TypeVar result;
    size_t n = types_func(c, func, &params, &result);
    if (n != expr->as.call.args.len) return report_count(c, expr, func, n);
    for (size_t i = 0; i < n; i++) {
        const Expr *arg = expr->as.call.args.items[i];
        if (checker_fits(c, params[i], args[i], arg, arg->pos.line)) continue;
        diag_error(c->diag, arg->pos.span, "%s needs %s for its %s input, but this is %s.",
                   callee_label(c, expr->as.call.callee), checker_kind(c, params[i]), ordinal(c, i),
                   checker_kind(c, args[i]));
        diag_note(c->diag, "It's %s.", checker_kind(c, func));
        return error_var(c);
    }
    Type type = types_of(c, result);
    if (as_value && type == TYPE_NOTHING) {
        diag_error(c->diag, expr->pos.span, "%s doesn't give back a value, so it can't be used here.",
                   callee_label(c, expr->as.call.callee));
        diag_note(c->diag, "It's %s. Call it as a sentence of its own, like \"call %s with ...\".",
                  checker_kind(c, func), expr->as.call.callee->kind == EXPR_NAME ? expr->as.call.callee->display : "f");
        return error_var(c);
    }
    if (as_value && type == TYPE_UNKNOWN) {
        types_defer_one_of(c, expr, result, result, KIND_NUMBER | KIND_TEXT | KIND_YESNO | KIND_LIST | KIND_RECORD |
                           KIND_MAP | KIND_OTHER | KIND_DATE, "A function called here has to give back a value");
    }
    return result;
}

TypeVar checker_check_value_call(Checker *c, const Expr *expr, bool as_value) {
    const Expr *callee = expr->as.call.callee;
    TypeVar func = checker_check_expr(c, callee);
    size_t n = expr->as.call.args.len;
    TypeVar *args = arena_alloc(c->arena, (n + 1) * sizeof(TypeVar));
    for (size_t i = 0; i < n; i++) args[i] = checker_check_value(c, expr->as.call.args.items[i]);
    Type type = types_of(c, func);
    if (type == TYPE_ERROR) return error_var(c);
    if (type == TYPE_UNKNOWN) {  // "f with 3" where nothing has decided f yet: it takes one input
        TypeVar *params = arena_alloc(c->arena, (n + 1) * sizeof(TypeVar));
        for (size_t i = 0; i < n; i++) params[i] = types_new(c, TYPE_UNKNOWN, 0);
        types_unify(c, func, types_new_func(c, params, n, types_new(c, TYPE_UNKNOWN, 0), expr->pos.line));
    } else if (!type_is_func(type)) {
        return report_not_function(c, callee, func);
    }
    return finish_value_call(c, expr, func, args, as_value);
}

// --- At the end ----------------------------------------------------------------------------

// "a function taking x (a number)": how to write down the kind of input i.
static const char *lambda_header(Checker *c, const Function *f, size_t marked) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_append(&sb, "a function taking ");
    const Stmt *def = f->definition;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        sb_appendf(&sb, "%s%s", i ? " and " : "", def->as.function.params.items[i].display);
        if (i == marked) sb_append(&sb, " (a number)");
    }
    return sb.data;
}

// Inputs first: a result is usually undecided because an input is (which
// then counts as a mistake already made, so the result isn't reported too).
void checker_report_undecided_lambdas(Checker *c) {
    size_t before = diag_count(c->diag);
    for (size_t i = 0; i < c->lambdas.len; i++) {
        const Function *f = c->lambdas.items[i];
        const Stmt *def = f->definition;
        for (size_t p = 0; p < f->params.len; p++) {
            if (!ast_type_undecided(c->records, types_of(c, f->params.items[p]))) continue;
            const Name *param = &def->as.function.params.items[p];
            diag_error(c->diag, param->pos.span, "I can't tell what kind of value \"%s\" is.", param->display);
            diag_note(c->diag, "Nothing in the program decides it. Say which kind where the function is made, like "
                               "\"%s\".", lambda_header(c, f, p));
            types_require(c, f->params.items[p], TYPE_ERROR, 0);
        }
    }
    for (size_t i = 0; i < c->lambdas.len; i++) {
        const Function *f = c->lambdas.items[i];
        if (!ast_type_undecided(c->records, types_of(c, f->result))) continue;
        if (diag_count(c->diag) == before) {
            diag_error(c->diag, f->definition->pos.span, "I can't tell what kind of value \"%s\" gives back.",
                       f->display);
            diag_note(c->diag, "Say which kind where it's made, like \"a function taking x, giving back a number:\".");
        }
        types_require(c, f->result, TYPE_ERROR, 0);
    }
}

void checker_write_lambdas(Checker *c) {
    for (size_t i = 0; i < c->lambdas.len; i++) {
        const Function *f = c->lambdas.items[i];
        Stmt *def = f->definition;
        def->as.function.param_types.len = 0;
        for (size_t p = 0; p < f->params.len; p++) {
            vec_push(c->arena, &def->as.function.param_types, types_of(c, f->params.items[p]));
        }
        def->as.function.result = types_of(c, f->result);
        for (size_t k = 0; k < def->as.function.captures.len; k++) {
            Capture *capture = &def->as.function.captures.items[k];
            capture->type = types_of(c, capture->var);
        }
    }
}
