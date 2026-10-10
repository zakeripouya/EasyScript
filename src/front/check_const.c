// The checker's part for constants: collecting them, working out their
// values (through consteval.c), and suggesting them for misspelled names.

#include <string.h>
#include "front/check_internal.h"
#include "front/consteval.h"

// The constant closest to `name` by spelling, if one is close enough.
const Constant *checker_closest_constant(const Checker *c, const char *name) {
    size_t limit = strlen(name) <= 3 ? 1 : 2;
    const Constant *best = NULL;
    size_t best_distance = limit + 1;
    for (size_t i = 0; i < c->constants.len; i++) {
        size_t d = edit_distance(c->arena, name, c->constants.items[i].name);
        if (d < best_distance) {
            best_distance = d;
            best = &c->constants.items[i];
        }
    }
    return best;
}

// --- Constants ---------------------------------------------------------------

static const char *const keep_note =
    "A constant is fixed before the program starts, so it can only use numbers, text, yes or no, and constants made "
    "above it. If the value is only known while the program runs, use \"let\" instead of \"keep\".";

// A name inside a constant's value: only a constant made above it will do.
static bool lookup_constant(void *context, const Expr *expr, ConstValue *out) {
    Checker *c = context;
    const char *name = expr->as.name;
    const Constant *k = checker_find_constant(c, name);
    if (k && k->index < c->constant->index) {
        *out = k->definition->as.constant.folded;
        return k->ok;  // if it failed, its own error was already reported
    }
    if (k == c->constant) {
        diag_error(c->diag, expr->pos.span, "A constant can't use itself.");
        diag_note(c->diag, "Give it a value made from numbers, text, yes or no, and constants made above it.");
    } else if (k) {
        diag_error(c->diag, expr->pos.span, "\"%s\" is a constant made later, on line %zu.", name, k->definition->pos.line);
        diag_note(c->diag, "A constant can only use constants made above it. Move \"keep %s ...\" higher up.", name);
    } else if (checker_find_function(c, name)) {
        diag_error(c->diag, expr->pos.span, "A constant can't call a function, because functions only run while the "
                                            "program runs.");
        diag_note(c->diag, "%s", keep_note);
    } else if (checker_find_later(c, name)) {
        diag_error(c->diag, expr->pos.span, "\"%s\" is a variable, so its value isn't known before the program runs.",
                   name);
        diag_note(c->diag, "%s", keep_note);
    } else {
        checker_report_unknown(c, name, expr->pos);
    }
    return false;
}

void checker_check_constant(Checker *c, const Stmt *stmt) {
    if (c->function || c->blocks > 0) {
        diag_error(c->diag, stmt->as.constant.name.pos.span, "Constants can only be made at the top level, not inside %s.",
                   c->function ? "a function" : "an \"if\" or a loop");
        diag_note(c->diag, "Move \"keep %s ...\" to the start of a line, outside every block.",
                  stmt->as.constant.name.text);
        return;
    }
    Constant *k = (Constant *)checker_find_constant(c, stmt->as.constant.name.text);
    if (!k || k->definition != stmt) return;  // a duplicate, already reported
    c->constant = k;
    k->ok = const_eval(c->arena, c->diag, stmt->as.constant.value, lookup_constant, c, &k->definition->as.constant.folded);
    c->constant = NULL;
}

// Every top-level constant, so it can be used anywhere (even above it).
void checker_collect_constants(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        Stmt *stmt = program->items[i];
        if (stmt->kind != STMT_CONSTANT) continue;
        const Name *name = &stmt->as.constant.name;
        const Constant *existing = checker_find_constant(c, name->text);
        const Function *function = checker_find_function(c, name->text);
        if (existing) {
            diag_error(c->diag, name->pos.span, "You already made the constant \"%s\" on line %zu.", name->text,
                       existing->definition->pos.line);
            diag_note(c->diag, "Each constant needs its own name.");
            continue;
        }
        if (function) {
            diag_error(c->diag, name->pos.span, "\"%s\" is the name of a function, defined on line %zu.", name->text,
                       function->definition->pos.line);
            diag_note(c->diag, "Give the constant a different name.");
            continue;
        }
        Constant k = {name->text, stmt, c->constants.len, false};
        vec_push(c->arena, &c->constants, k);
    }
}
