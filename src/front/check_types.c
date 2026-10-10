// Type inference: type variables joined with union-find, the operators that
// wait for their sides' kinds, and the last step that reports anything left
// undecided and writes every kind into the program for the code generator.

#include <assert.h>
#include <string.h>
#include "front/check_internal.h"

// --- Variables -------------------------------------------------------------------

TypeVar types_new(Checker *c, Type type, size_t line) {
    TypeSlot slot = {c->types.len, type, line};
    vec_push(c->arena, &c->types, slot);
    return c->types.len - 1;
}

static TypeVar root(Checker *c, TypeVar var) {
    while (c->types.items[var].parent != var) {
        TypeVar parent = c->types.items[var].parent;
        c->types.items[var].parent = c->types.items[parent].parent;  // path halving
        var = parent;
    }
    return var;
}

Type types_of(Checker *c, TypeVar var) {
    return c->types.items[root(c, var)].type;
}

size_t types_line(Checker *c, TypeVar var) {
    return c->types.items[root(c, var)].line;
}

// TYPE_ERROR joins with anything without complaint, and spreads: a value
// that's already wrong shouldn't cause a second message.
bool types_unify(Checker *c, TypeVar a, TypeVar b) {
    TypeVar ra = root(c, a), rb = root(c, b);
    if (ra == rb) return true;
    TypeSlot *sa = &c->types.items[ra], *sb = &c->types.items[rb];
    if (sa->type != TYPE_UNKNOWN && sb->type != TYPE_UNKNOWN && sa->type != sb->type &&
        sa->type != TYPE_ERROR && sb->type != TYPE_ERROR) {
        return false;
    }
    if (sb->type == TYPE_ERROR || sa->type == TYPE_UNKNOWN) {
        sa->type = sb->type;
        sa->line = sb->line;
    }
    sb->parent = ra;
    return true;
}

bool types_require(Checker *c, TypeVar var, Type type, size_t line) {
    TypeSlot *slot = &c->types.items[root(c, var)];
    if (slot->type == TYPE_UNKNOWN || type == TYPE_ERROR) {
        slot->type = type;
        slot->line = line;
        return true;
    }
    return slot->type == type || slot->type == TYPE_ERROR;
}

// --- Operators that wait for their sides -------------------------------------------

static void report(Checker *c, const Expr *expr, const char *hint, const char *message) {
    diag_error(c->diag, expr->pos.span, "%s", message);
    if (hint) diag_note(c->diag, "%s", hint);
}

// "and": logical for two yes/no values, joining when text is on either side
// (the other side a number or text). Returns true once settled.
static bool settle_and(Checker *c, const Deferred *d) {
    Type left = types_of(c, d->left), right = types_of(c, d->right);
    size_t line = d->expr->pos.line;
    if (left == TYPE_ERROR || right == TYPE_ERROR) return types_require(c, d->result, TYPE_ERROR, line);
    if (left == TYPE_YESNO || right == TYPE_YESNO) {
        TypeVar other = left == TYPE_YESNO ? d->right : d->left;
        if (types_require(c, other, TYPE_YESNO, line)) return types_require(c, d->result, TYPE_YESNO, line);
        bool with_text = left == TYPE_TEXT || right == TYPE_TEXT;
        report(c, d->expr, with_text ? "To join them, turn the yes/no value into text first with \"as text\"." : NULL,
               arena_sprintf(c->arena, "I can't use \"and\" between %s and %s.", ast_type_name(left),
                             ast_type_name(right)));
        return types_require(c, d->result, TYPE_ERROR, line);
    }
    if (left == TYPE_NUMBER && right == TYPE_NUMBER) {
        report(c, d->expr, "To add numbers, use \"plus\".", "I can't use \"and\" between a number and a number.");
        return types_require(c, d->result, TYPE_ERROR, line);
    }
    if (left == TYPE_NUMBER) types_require(c, d->right, TYPE_TEXT, line);  // a number joins only with text
    if (right == TYPE_NUMBER) types_require(c, d->left, TYPE_TEXT, line);
    if (left == TYPE_UNKNOWN && right == TYPE_UNKNOWN) return false;
    types_require(c, d->result, TYPE_TEXT, line);
    // Text with a side not known yet: settled once that side turns out not to be yes/no.
    return types_of(c, d->left) != TYPE_UNKNOWN && types_of(c, d->right) != TYPE_UNKNOWN;
}

static bool settle(Checker *c, Deferred *d) {
    if (d->kind == DEFER_AND) return settle_and(c, d);
    Type type = types_of(c, d->left);
    if (type == TYPE_UNKNOWN) return false;
    if (type == TYPE_YESNO && d->kind == DEFER_ORDER) {
        report(c, d->expr, NULL, "I can't compare a yes/no value with a yes/no value.");
    } else if (type == TYPE_YESNO) {
        report(c, d->expr, NULL, "I can't turn a yes/no value into a number.");
    }
    return true;
}

void types_defer(Checker *c, DeferKind kind, const Expr *expr, TypeVar left, TypeVar right, TypeVar result) {
    Deferred d = {kind, expr, left, right, result, false};
    d.done = settle(c, &d);  // usually the kinds are known already
    if (!d.done) vec_push(c->arena, &c->deferred, d);
}

// Settling one operator can decide kinds another one waits for, so go round
// until nothing changes.
static void settle_all(Checker *c) {
    bool progress = true;
    while (progress) {
        progress = false;
        for (size_t i = 0; i < c->deferred.len; i++) {
            Deferred *d = &c->deferred.items[i];
            if (!d->done && settle(c, d)) d->done = progress = true;
        }
    }
}

// --- Kinds nothing decided ---------------------------------------------------------

// "to show with x (a number)", with the given input marked.
static const char *header_with_kind(const Checker *c, const Function *f, size_t marked) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_appendf(&sb, "to %s", f->name);
    const Stmt *def = f->definition;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        sb_append(&sb, i == 0 ? " with " : " and ");
        sb_append(&sb, def->as.function.params.items[i].text);
        if (i == marked) sb_append(&sb, " (a number)");
    }
    return sb.data;
}

static void report_undecided(Checker *c, const Function *f) {
    const Stmt *def = f->definition;
    for (size_t i = 0; i < f->params.len; i++) {
        if (types_of(c, f->params.items[i]) != TYPE_UNKNOWN) continue;
        const Name *param = &def->as.function.params.items[i];
        diag_error(c->diag, param->pos.span, "I can't tell what kind of value \"%s\" is.", param->text);
        diag_note(c->diag, "Nothing in the program decides it. Say which kind in the definition, like \"%s:\".",
                  header_with_kind(c, f, i));
        types_require(c, f->params.items[i], TYPE_ERROR, 0);  // and everything that depends on it
    }
    if (types_of(c, f->result) != TYPE_UNKNOWN) return;
    diag_error(c->diag, def->as.function.name.pos.span, "I can't tell what kind of value \"%s\" gives back.", f->name);
    diag_note(c->diag, "Say which kind in the definition, like \"%s, giving back a number:\".",
              checker_function_header(c, f));
    types_require(c, f->result, TYPE_ERROR, 0);
}

void types_finish(Checker *c, size_t first_error) {
    settle_all(c);
    // A kind left undecided is usually a side effect of another mistake, so
    // it's only reported once everything else is right.
    for (size_t i = 0; diag_count(c->diag) == first_error && i < c->functions.len; i++) {
        report_undecided(c, &c->functions.items[i]);
    }
    for (size_t i = 0; i < c->typed.len; i++) {
        c->typed.items[i].expr->type = types_of(c, c->typed.items[i].var);
    }
    for (size_t i = 0; i < c->functions.len; i++) {
        Function *f = &c->functions.items[i];
        Stmt *def = f->definition;
        def->as.function.param_types.len = 0;
        for (size_t p = 0; p < f->params.len; p++) {
            vec_push(c->arena, &def->as.function.param_types, types_of(c, f->params.items[p]));
        }
        def->as.function.result = types_of(c, f->result);
    }
}
