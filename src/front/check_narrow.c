// Values that can be nothing, and narrowing: what the program has checked
// about a value where it uses it.
//
// A field whose kind is "a node or nothing" can't be used as a node until the
// program checks it: inside "if the node's next is not nothing:" (or a
// "while" loop like that, or after "if node is nothing, give back 0."), the
// name or field checked (its path, like "node's next") is the record it
// holds, until something sets it. Generated code still checks it while the
// program runs (another name for the same record could have changed it), so
// a program can never use nothing as a record. A value of a kind with
// variants checked to be one of them ("if s is a circle:") is narrowed the
// same way: its path is that variant there, so its fields can be used.

#include <string.h>
#include "front/check_internal.h"

static const char *path_of(Checker *c, const Expr *expr, bool display) {
    if (expr->kind == EXPR_NAME) return display ? expr->display : expr->as.name;
    if (expr->kind != EXPR_FIELD) return NULL;
    const char *record = path_of(c, expr->as.field.record, display);
    if (!record) return NULL;
    const Name *field = &expr->as.field.name;
    return arena_sprintf(c->arena, "%s's %s", record, display ? field->display : field->text);
}

const char *checker_path(Checker *c, const Expr *expr) {
    return path_of(c, expr, false);
}

void checker_report_maybe(Checker *c, const Expr *at, TypeVar var) {
    const char *path = path_of(c, at, true);
    if (path) {
        diag_error(c->diag, at->pos.span, "\"%s\" could be nothing, so check it first.", path);
        diag_note(c->diag, "Use it inside \"if %s is not nothing:\".", path);
        return;
    }
    diag_error(c->diag, at->pos.span, "This is %s, so check it first.", checker_kind(c, var));
    diag_note(c->diag, "Give it a name with \"let\", then use it inside \"if NAME is not nothing:\".");
}

static bool is_narrowed(const Checker *c, const char *path) {
    for (size_t i = 0; i < c->present.len; i++) {
        if (!c->present.items[i].killed && strcmp(c->present.items[i].path, path) == 0) return true;
    }
    return false;
}

TypeVar checker_narrow(Checker *c, const Expr *expr, TypeVar var) {
    Type type = types_of(c, var);
    if (expr == c->unnarrowed || !(type == TYPE_MAYBE || type_is_maybe(type))) return var;
    const char *path = checker_path(c, expr);
    if (!path || !is_narrowed(c, path)) return var;
    if (checker_reject_set_inside(c, expr, path)) return types_new(c, TYPE_ERROR, 0);
    ((Expr *)expr)->narrowed = true;  // codegen still checks it while the program runs
    if (type != TYPE_MAYBE) return types_new(c, type_present(type), expr->pos.line);
    TypeVar present = types_new(c, TYPE_UNKNOWN, 0);
    types_defer(c, DEFER_PRESENT, expr, var, present, present);
    return present;
}

void checker_add_narrowing(Checker *c, const char *path, size_t variant) {
    if (!path) return;
    Narrowing n = {path, false, variant};
    vec_push(c->arena, &c->present, n);
}

static void add_narrowing(Checker *c, const Expr *expr) {
    checker_add_narrowing(c, checker_path(c, expr), NO_VARIANT);
}

size_t checker_narrowed_variant(Checker *c, const Expr *expr) {
    if (expr->kind == EXPR_IT) {  // "its radius" inside "when it's a circle:"
        for (size_t i = c->loops.len; i > 0; i--) {
            const Loop *loop = &c->loops.items[i - 1];
            if (loop->has_it) return loop->subject ? loop->variant : NO_VARIANT;
        }
        return NO_VARIANT;
    }
    const char *path = checker_path(c, expr);
    for (size_t i = c->present.len; path && i > 0; i--) {
        const Narrowing *n = &c->present.items[i - 1];
        if (!n->killed && n->variant != NO_VARIANT && strcmp(n->path, path) == 0) return n->variant;
    }
    return NO_VARIANT;
}

void checker_narrow_condition(Checker *c, const Expr *condition, bool when) {
    if (condition->kind == EXPR_UNARY && condition->as.unary.op == UNARY_NOT) {
        checker_narrow_condition(c, condition->as.unary.operand, !when);
        return;
    }
    if (condition->kind == EXPR_KIND_TEST) {  // "s is a circle": s is a circle (and not nothing)
        if (when != condition->as.kind_test.negated) {
            checker_add_narrowing(c, checker_path(c, condition->as.kind_test.operand), condition->as.kind_test.variant);
        }
        return;
    }
    if (condition->kind != EXPR_BINARY) return;
    const Expr *left = condition->as.binary.left, *right = condition->as.binary.right;
    switch (condition->as.binary.op) {
    case BINARY_AND:
        if (!when) return;
        checker_narrow_condition(c, left, true);
        checker_narrow_condition(c, right, true);
        return;
    case BINARY_OR:
        if (when) return;
        checker_narrow_condition(c, left, false);
        checker_narrow_condition(c, right, false);
        return;
    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
        if ((condition->as.binary.op == BINARY_NOT_EQUAL) != when) return;
        if (right->kind == EXPR_NOTHING) add_narrowing(c, left);
        if (left->kind == EXPR_NOTHING) add_narrowing(c, right);
        return;
    default: return;
    }
}

void checker_kill_path(Checker *c, const Expr *target, const char *name) {
    const char *path = target ? checker_path(c, target) : name;
    if (!path) return;
    size_t len = strlen(path);
    for (size_t i = 0; i < c->present.len; i++) {
        const char *p = c->present.items[i].path;
        if (strncmp(p, path, len) == 0 && (p[len] == '\0' || strncmp(p + len, "'s ", 3) == 0)) {
            c->present.items[i].killed = true;
        }
    }
}

void checker_kill_set_in(Checker *c, const Block *block) {
    for (size_t i = 0; i < block->len; i++) {
        const Stmt *stmt = block->items[i];
        if (stmt->kind == STMT_SET) checker_kill_path(c, stmt->as.assign.field, stmt->as.assign.name.text);
        if (stmt->kind == STMT_LOOP) checker_kill_set_in(c, &stmt->as.loop.body);
        for (size_t b = 0; stmt->kind == STMT_IF && b < stmt->as.if_stmt.branches.len; b++) {
            checker_kill_set_in(c, &stmt->as.if_stmt.branches.items[b].body);
        }
        for (size_t b = 0; stmt->kind == STMT_MATCH && b < stmt->as.match.branches.len; b++) {
            checker_kill_set_in(c, &stmt->as.match.branches.items[b].body);
        }
    }
}

// --- Values that can be nothing ------------------------------------------------------------

void checker_note_maybe(Checker *c, TypeVar var, const Expr *at) {
    if (!type_is_maybe(types_of(c, var))) return;
    const char *path = checker_path(c, at);
    if (path) {
        diag_note(c->diag, "It could be nothing. Check it first, like \"if %s is not nothing:\".",
                  at->kind == EXPR_NAME ? at->display : path);
    } else {
        diag_note(c->diag, "It could be nothing. Give it a name with \"let\" and check it first with \"is not nothing\".");
    }
}

TypeVar checker_check_value(Checker *c, const Expr *expr) {
    if (expr->kind != EXPR_NOTHING) return checker_check_expr(c, expr);
    TypeVar var = types_new(c, TYPE_NOTHING, expr->pos.line);
    TypedExpr typed = {(Expr *)expr, var};
    vec_push(c->arena, &c->typed, typed);
    return var;
}

bool checker_fits(Checker *c, TypeVar target, TypeVar value, const Expr *expr, size_t line) {
    Type t = types_of(c, target), v = types_of(c, value);
    if (v == TYPE_NOTHING) {
        if (t == TYPE_UNKNOWN) return types_require(c, target, TYPE_MAYBE, line);
        return t == TYPE_MAYBE || type_is_maybe(t) || t == TYPE_ERROR;
    }
    // A dog where a dog or nothing goes, a number where a number or nothing goes.
    if (expr && type_can_be_nothing(v) && ((type_is_maybe(t) && type_present(t) == v) || t == TYPE_MAYBE)) {
        if (t == TYPE_MAYBE) types_require(c, target, v + TYPE_MAYBE, line);
        ((Expr *)expr)->lifted = true;  // codegen puts a number, text or yes/no in a box
        return true;
    }
    if (checker_iface_fits(c, target, value, expr)) return true;  // a dog where something that can speak goes
    return types_unify(c, target, value);
}

bool checker_check_nothing_test(Checker *c, const Expr *expr, TypeVar *result) {
    if (expr->kind != EXPR_BINARY || (expr->as.binary.op != BINARY_EQUAL && expr->as.binary.op != BINARY_NOT_EQUAL)) {
        return false;
    }
    const Expr *left = expr->as.binary.left, *right = expr->as.binary.right;
    if (left->kind != EXPR_NOTHING && right->kind != EXPR_NOTHING) return false;
    *result = types_new(c, TYPE_YESNO, expr->pos.line);
    const Expr *other = left->kind == EXPR_NOTHING ? right : left;
    checker_check_value(c, left->kind == EXPR_NOTHING ? left : right);
    if (other->kind == EXPR_NOTHING) {
        diag_error(c->diag, expr->pos.span, "I can't compare nothing with nothing.");
        diag_note(c->diag, "Compare something that can be nothing with it, like \"if the node's next is nothing\".");
        return true;
    }
    const Expr *saved = c->unnarrowed;
    c->unnarrowed = other;  // "x is nothing" asks about x as it is, even where it was checked before
    TypeVar var = checker_check_value(c, other);
    c->unnarrowed = saved;
    Type type = types_of(c, var);
    if (type == TYPE_UNKNOWN) {
        types_require(c, var, TYPE_MAYBE, expr->pos.line);
    } else if ((type_is_record(type) || type_is_sum(type)) && !type_is_maybe(type)) {
        const char *path = path_of(c, other, true);
        diag_error(c->diag, other->pos.span, "%s is %s, so it's never nothing.",
                   path ? arena_sprintf(c->arena, "\"%s\"", path) : "This", checker_kind(c, var));
        diag_note(c->diag, "Only a value that's \"%s or nothing\" can be nothing.", checker_kind(c, var));
    } else if (type != TYPE_MAYBE && !type_is_maybe(type) && type != TYPE_ERROR) {
        diag_error(c->diag, expr->pos.span, "I can't compare %s with nothing.", checker_kind(c, var));
        diag_note(c->diag, "Only a value whose kind ends in \"or nothing\" can be nothing, like a field or input that's "
                           "\"a number or nothing\".");
    }
    return true;
}

bool checker_settle_present(Checker *c, const Deferred *d) {
    Type maybe = types_of(c, d->left), present = types_of(c, d->right);
    if (maybe == TYPE_ERROR || present == TYPE_ERROR) return true;
    if (type_is_maybe(maybe)) return types_require(c, d->right, type_present(maybe), d->expr->pos.line) || true;
    if (type_can_be_nothing(present)) return types_require(c, d->left, present + TYPE_MAYBE, d->expr->pos.line) || true;
    if (present == TYPE_UNKNOWN) return false;
    diag_error(c->diag, d->expr->pos.span, "This can be nothing, but it's used as %s here, which can't be.",
               checker_kind(c, d->right));
    diag_note(c->diag, "Only a number, text, a yes/no value, a record or a kind with variants can be nothing. To say a "
                       "list or map has nothing in it, make it empty.");
    return true;
}


// "X, or D [if there isn't one]": X as written must be able to be nothing;
// the result is what it holds, and D has to be that kind too.
TypeVar checker_check_default(Checker *c, const Expr *expr) {
    const Expr *value = expr->as.lookup.map, *fallback = expr->as.lookup.fallback;
    size_t line = expr->pos.line;
    const Expr *saved = c->unnarrowed;
    c->unnarrowed = value;  // even where it was checked, ", or" asks about it as it is
    TypeVar var = checker_check_value(c, value);
    c->unnarrowed = saved;
    Type type = types_of(c, var);
    if (type != TYPE_ERROR && type != TYPE_UNKNOWN && type != TYPE_MAYBE && !type_is_maybe(type)) {
        diag_error(c->diag, value->pos.span, "This is %s, so it's never nothing.", checker_kind(c, var));
        diag_note(c->diag, "\", or\" gives another value when there isn't one. Leave it out here.");
        diag_info(c->diag, DIAG_NOT_ALLOWED, "something that can be nothing", NULL);
        checker_check_expr(c, fallback);
        return types_new(c, TYPE_ERROR, line);
    }
    if (type == TYPE_ERROR) {
        checker_check_expr(c, fallback);
        return var;
    }
    if (type == TYPE_UNKNOWN) types_require(c, var, TYPE_MAYBE, line);
    TypeVar present = types_new(c, TYPE_UNKNOWN, line);
    types_defer(c, DEFER_PRESENT, expr, var, present, present);
    TypeVar other = checker_check_value(c, fallback);
    if (!types_unify(c, present, other)) {
        diag_error(c->diag, fallback->pos.span, "This gives %s when there isn't one, but the value is %s.",
                   checker_kind(c, other), checker_kind(c, present));
        diag_note(c->diag, "The value after \", or\" has to be the same kind as the one it stands in for.");
    }
    return present;
}

// A "depends on" table: the subject is "it" in each condition (yes or no),
// and every line's value is the same kind, which the table is.
TypeVar checker_check_table(Checker *c, const Expr *expr) {
    const Expr *subject_expr = expr->as.table.subject;  // NULL: a choice ("4 if C, otherwise 0")
    if (subject_expr) {
        TypeVar subject = checker_check_expr(c, subject_expr);
        Loop loop = {true, subject, true, NO_VARIANT, false};
        vec_push(c->arena, &c->loops, loop);
    }
    TypeVar result = 0;
    for (size_t i = 0; i < expr->as.table.values.len; i++) {
        const Expr *condition = expr->as.table.conditions.items[i];
        if (condition) {
            checker_require(c, checker_check_expr(c, condition), TYPE_YESNO, condition,
                            subject_expr ? "A line's condition has to be yes or no"
                                         : "The condition after \"if\" has to be yes or no", NULL);
        }
        const Expr *value = expr->as.table.values.items[i];
        TypeVar var = checker_check_value(c, value);
        if (i == 0) {
            result = var;
        } else if (!types_unify(c, result, var)) {
            if (subject_expr) {
                diag_error(c->diag, value->pos.span, "All the values of a table have to be the same kind: the first is "
                                                     "%s, but this is %s.", checker_kind(c, result), checker_kind(c, var));
            } else {
                diag_error(c->diag, value->pos.span, "Both values of a choice have to be the same kind: the first is %s, "
                                                     "but this is %s.", checker_kind(c, result), checker_kind(c, var));
            }
        }
    }
    if (subject_expr) c->loops.len--;
    return result;
}
