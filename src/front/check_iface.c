// Interfaces: their definitions, whether a kind of record (or with
// variants) can be used as one (it has a method with the same name, inputs
// and result for every way), and calls of a way on a value of an interface,
// which pick the method while the program runs.

#include <string.h>
#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

static const Stmt *definition(const Checker *c, Type iface) {
    size_t index = type_iface_index(iface);
    return index < c->records->interfaces.len ? c->records->interfaces.items[index].definition : NULL;
}

static const Way *find_way(const Stmt *def, const char *name) {
    for (size_t i = 0; def && i < def->as.iface.ways.len; i++) {
        if (strcmp(def->as.iface.ways.items[i].name.text, name) == 0) return &def->as.iface.ways.items[i];
    }
    return NULL;
}

// "a way to greet with name (text), giving back text"
static const char *way_header(Checker *c, const Way *way) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_appendf(&sb, "a way to %s", way->name.display);
    for (size_t i = 0; i < way->params.len; i++) {
        sb_appendf(&sb, "%s%s (%s)", i ? " and " : way->intro ? arena_sprintf(c->arena, " %s ", way->intro) : " with ",
                   way->params.items[i].display, checker_type_name(c, way->declared.items[i]));
    }
    if (way->result != TYPE_NOTHING) sb_appendf(&sb, ", giving back %s", checker_type_name(c, way->result));
    return sb.data;
}

// "to greet, for a fish, with name (text), giving back text:": a method that would give the kind the way.
static const char *method_header(Checker *c, const Way *way, Type kind) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_appendf(&sb, "to %s, for %s", way->name.display, checker_type_name(c, kind));
    for (size_t i = 0; i < way->params.len; i++) {
        sb_appendf(&sb, "%s%s (%s)", i ? " and " : ", with ", way->params.items[i].display,
                   checker_type_name(c, way->declared.items[i]));
    }
    if (way->result != TYPE_NOTHING) sb_appendf(&sb, ", giving back %s", checker_type_name(c, way->result));
    return sb.data;
}

// --- Definitions --------------------------------------------------------------------------

void checker_collect_interfaces(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        const Stmt *stmt = program->items[i];
        if (stmt->kind != STMT_INTERFACE) continue;
        const Name *name = &stmt->as.iface.name;
        const RecordKind *kind = &c->records->interfaces.items[stmt->as.iface.index];
        if (kind->definition != stmt) {
            diag_error(c->diag, name->pos.span, "You already defined \"%s\" on line %zu.", name->display,
                       kind->definition->pos.line);
            diag_note(c->diag, "Each interface needs its own name.");
            continue;
        }
        for (size_t r = 0; r < c->records->len; r++) {
            if (strcmp(c->records->items[r].name, name->text) != 0) continue;
            diag_error(c->diag, name->pos.span, "\"%s\" is already the name of a kind of record, on line %zu.",
                       name->display, c->records->items[r].definition->pos.line);
            diag_note(c->diag, "Give the interface another name, like \"something that can %s\".",
                      stmt->as.iface.ways.len ? stmt->as.iface.ways.items[0].name.display : "speak");
        }
    }
}

void checker_check_interface(Checker *c, const Stmt *stmt) {
    if (c->function || c->blocks > 0) {
        diag_error(c->diag, stmt->as.iface.name.pos.span, "Interfaces can only be defined at the top level, not inside "
                   "%s.", c->function ? "a definition" : "an \"if\" or a loop");
        diag_note(c->diag, "Move it to the start of a line, outside every block.");
        return;
    }
    for (size_t i = 0; i < stmt->as.iface.ways.len; i++) {
        const Way *way = &stmt->as.iface.ways.items[i];
        for (size_t j = 0; j < i; j++) {
            if (strcmp(stmt->as.iface.ways.items[j].name.text, way->name.text) != 0) continue;
            diag_error(c->diag, way->name.pos.span, "\"%s\" already has a way to %s, on line %zu.",
                       stmt->as.iface.name.display, way->name.display, stmt->as.iface.ways.items[j].pos.line);
            diag_note(c->diag, "Each way of an interface needs its own name.");
        }
    }
}

// --- Fitting ---------------------------------------------------------------------------------

static const Function *method_for(const Checker *c, const char *name, Type kind) {
    for (size_t i = 0; i < c->functions.len; i++) {
        const Function *f = &c->functions.items[i];
        const Stmt *def = f->definition;
        if (def->as.function.method && def->as.function.declared.items[0] == kind && strcmp(f->name, name) == 0) {
            return f;
        }
    }
    return NULL;
}

static bool satisfies(Checker *c, Type kind, Type iface);

// The method has the way's inputs and result (deciding them, if they
// weren't). A way that gives back an interface can be a method that gives
// back any kind that fits it ("scale" giving back a square, for a shape).
static bool matches(Checker *c, const Function *f, const Way *way) {
    if (f->params.len != way->params.len + 1) return false;
    size_t line = way->pos.line;
    for (size_t i = 0; i < way->params.len; i++) {
        if (!types_unify(c, f->params.items[i + 1], types_new(c, way->declared.items[i], line))) return false;
    }
    Type got = types_of(c, f->result);
    if (type_is_iface(way->result) && !type_is_maybe(way->result) && (type_is_record(got) || type_is_sum(got)) &&
        !type_is_maybe(got)) {
        return satisfies(c, got, way->result);
    }
    return types_unify(c, f->result, types_new(c, way->result, line));
}

// Every way has a matching method. While a pair is being matched it's
// assumed to fit, so a way giving back the interface itself can be checked.
static bool satisfies(Checker *c, Type kind, Type iface) {
    for (size_t i = 0; i + 1 < c->assumed.len; i += 2) {
        if (c->assumed.items[i] == kind && c->assumed.items[i + 1] == iface) return true;
    }
    vec_push(c->arena, &c->assumed, kind);
    vec_push(c->arena, &c->assumed, iface);
    const Stmt *def = definition(c, iface);
    bool all = true;
    for (size_t i = 0; all && def && i < def->as.iface.ways.len; i++) {
        const Way *way = &def->as.iface.ways.items[i];
        const Function *f = method_for(c, way->name.text, kind);
        all = f && matches(c, f, way);
    }
    c->assumed.len -= 2;
    return all;
}

bool checker_satisfies(Checker *c, Type kind, Type iface, const Way **missing, const Function **wrong) {
    const Stmt *def = definition(c, iface);
    *missing = NULL;
    *wrong = NULL;
    if (satisfies(c, kind, iface)) return true;
    for (size_t i = 0; def && i < def->as.iface.ways.len; i++) {
        const Way *way = &def->as.iface.ways.items[i];
        const Function *f = method_for(c, way->name.text, kind);
        if (!f || !matches(c, f, way)) {
            *missing = way;
            *wrong = f;
            return false;
        }
    }
    return true;
}

bool checker_iface_fits(Checker *c, TypeVar target, TypeVar value, const Expr *expr) {
    Type t = types_of(c, target), v = types_of(c, value);
    if (!type_is_iface(t) || !(type_is_record(v) || type_is_sum(v)) || (type_is_maybe(v) && !type_is_maybe(t))) {
        return false;
    }
    const Way *missing;
    const Function *wrong;
    Type kind = type_present(v), iface = type_present(t);
    if (checker_satisfies(c, kind, iface, &missing, &wrong)) return true;
    if (!expr) return false;
    const char *as = checker_type_name(c, iface);
    if (!wrong) {
        diag_error(c->diag, expr->pos.span, "%s can't be used as %s: it has no way to %s.", checker_capitalized(c, kind),
                   as, missing->name.display);
        diag_note(c->diag, "Give it one, like \"%s:\".", method_header(c, missing, kind));
    } else {
        diag_error(c->diag, expr->pos.span, "%s can't be used as %s: its way to %s doesn't match.",
                   checker_capitalized(c, kind), as, missing->name.display);
        diag_note(c->diag, "It has to be \"%s\", but it's defined on line %zu as \"%s\".", way_header(c, missing),
                  wrong->definition->pos.line, checker_function_header(c, wrong));
    }
    return true;  // reported
}

// The interfaces every one of `kinds` (records or kinds with variants, none
// "or nothing") fits: how many, and the first.
size_t checker_common_interfaces(Checker *c, const Type *kinds, size_t n, Type *first) {
    size_t count = 0;
    *first = TYPE_UNKNOWN;
    for (size_t i = 0; i < c->records->interfaces.len; i++) {
        bool all = true;
        for (size_t k = 0; k < n && all; k++) {
            const Way *missing;
            const Function *wrong;
            all = checker_satisfies(c, kinds[k], type_iface(i), &missing, &wrong);
        }
        if (all && count++ == 0) *first = type_iface(i);
    }
    return count;
}

// --- Calling a way --------------------------------------------------------------------------

bool checker_dispatch(Checker *c, const Expr *expr, const TypeVar *args, bool as_value, TypeVar *result) {
    if (expr->as.call.args.len == 0) return false;
    Type type = types_of(c, args[0]);
    if (!type_is_iface(type)) return false;
    const Expr *self = expr->as.call.args.items[0];
    if (type_is_maybe(type)) checker_report_maybe(c, self, args[0]);
    const Way *way = find_way(definition(c, type), expr->as.call.name);
    if (!way && checker_find_function(c, expr->as.call.name)) return false;  // a function given it as an input
    *result = error_var(c);
    if (!way) {
        const Stmt *def = definition(c, type);
        diag_error(c->diag, expr->pos.span, "%s has no way to %s.", checker_capitalized(c, type_present(type)),
                   expr->display);
        StrBuf ways;
        sb_init(&ways, c->arena);
        for (size_t i = 0; def && i < def->as.iface.ways.len; i++) {
            sb_appendf(&ways, "%s\"%s\"", i == 0 ? "" : i + 1 == def->as.iface.ways.len ? " and " : ", ",
                       def->as.iface.ways.items[i].name.display);
        }
        diag_note(c->diag, "Only its ways can be used on it: %s.", ways.len ? ways.data : "it has none");
        return true;
    }
    size_t n = expr->as.call.args.len - 1;
    if (n != way->params.len) {
        diag_error(c->diag, expr->pos.span, "The way to %s takes %zu value%s after %s, but this gives it %zu.",
                   way->name.display, way->params.len, way->params.len == 1 ? "" : "s", self->display ? self->display
                   : "the value", n);
        diag_note(c->diag, "It's \"%s\".", way_header(c, way));
        return true;
    }
    for (size_t i = 0; i < n; i++) {
        const Expr *arg = expr->as.call.args.items[i + 1];
        if (checker_fits(c, types_new(c, way->declared.items[i], arg->pos.line), args[i + 1], arg, arg->pos.line)) continue;
        diag_error(c->diag, arg->pos.span, "The way to %s needs %s for \"%s\", but this is %s.", way->name.display,
                   checker_type_name(c, way->declared.items[i]), way->params.items[i].display,
                   checker_kind(c, args[i + 1]));
        diag_note(c->diag, "It's \"%s\".", way_header(c, way));
        return true;
    }
    if (as_value && way->result == TYPE_NOTHING) {
        diag_error(c->diag, expr->pos.span, "The way to %s doesn't give back a value, so it can't be used here.",
                   way->name.display);
        diag_note(c->diag, "It's \"%s\". Use it as a sentence of its own.", way_header(c, way));
        return true;
    }
    ((Expr *)expr)->as.call.dispatch = true;
    *result = types_new(c, way->result, expr->pos.line);
    return true;
}
