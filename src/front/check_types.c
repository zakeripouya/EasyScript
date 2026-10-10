// Type inference: type variables joined with union-find, the operators that
// wait for their sides' kinds, and the last step that reports anything left
// undecided and writes every kind into the program for the code generator.

#include <assert.h>
#include <string.h>
#include "front/check_internal.h"

// --- Variables -------------------------------------------------------------------

static TypeVar new_slot(Checker *c, Type type, size_t line, TypeVar item) {
    TypeSlot slot = {c->types.len, type, line, item};
    vec_push(c->arena, &c->types, slot);
    return c->types.len - 1;
}

TypeVar types_new(Checker *c, Type type, size_t line) {
    if (!type_is_list(type)) return new_slot(c, type, line, 0);
    TypeVar item = types_new(c, type_item(type), type_item(type) == TYPE_UNKNOWN ? 0 : line);
    return new_slot(c, TYPE_LIST, line, item);
}

TypeVar types_new_list(Checker *c, TypeVar item, size_t line) {
    return new_slot(c, TYPE_LIST, line, item);
}

static TypeVar root(Checker *c, TypeVar var) {
    while (c->types.items[var].parent != var) {
        TypeVar parent = c->types.items[var].parent;
        c->types.items[var].parent = c->types.items[parent].parent;  // path halving
        var = parent;
    }
    return var;
}

TypeVar types_item(Checker *c, TypeVar list) {
    return c->types.items[root(c, list)].item;
}

Type types_of(Checker *c, TypeVar var) {
    const TypeSlot *slot = &c->types.items[root(c, var)];
    if (slot->type != TYPE_LIST) return slot->type;
    Type item = types_of(c, slot->item);
    return item == TYPE_ERROR ? TYPE_ERROR : type_list_of(item);
}

size_t types_line(Checker *c, TypeVar var) {
    return c->types.items[root(c, var)].line;
}

// Whether `var` appears in `in` (so joining them would make a list that
// holds itself).
static bool occurs(Checker *c, TypeVar var, TypeVar in) {
    TypeVar r = root(c, in);
    if (r == var) return true;
    return c->types.items[r].type == TYPE_LIST && occurs(c, var, c->types.items[r].item);
}

static bool compatible(Checker *c, TypeVar a, TypeVar b) {
    TypeVar ra = root(c, a), rb = root(c, b);
    if (ra == rb) return true;
    Type ta = c->types.items[ra].type, tb = c->types.items[rb].type;
    if (ta == TYPE_ERROR || tb == TYPE_ERROR) return true;
    if (ta == TYPE_UNKNOWN) return !occurs(c, ra, rb);
    if (tb == TYPE_UNKNOWN) return !occurs(c, rb, ra);
    if (ta == TYPE_LIST && tb == TYPE_LIST) return compatible(c, c->types.items[ra].item, c->types.items[rb].item);
    return ta == tb;
}

// Joins two compatible variables. The one that knows more becomes the root;
// TYPE_ERROR wins over everything, so a value that's already wrong spreads
// silently instead of causing a second message.
static void join(Checker *c, TypeVar a, TypeVar b) {
    TypeVar ra = root(c, a), rb = root(c, b);
    if (ra == rb) return;
    Type ta = c->types.items[ra].type, tb = c->types.items[rb].type;
    if (ta == TYPE_ERROR || tb == TYPE_UNKNOWN) {
        c->types.items[rb].parent = ra;
    } else if (tb == TYPE_ERROR || ta == TYPE_UNKNOWN) {
        c->types.items[ra].parent = rb;
    } else {
        if (ta == TYPE_LIST) join(c, c->types.items[ra].item, c->types.items[rb].item);
        c->types.items[rb].parent = ra;
    }
}

bool types_unify(Checker *c, TypeVar a, TypeVar b) {
    if (!compatible(c, a, b)) return false;
    join(c, a, b);
    return true;
}

bool types_require(Checker *c, TypeVar var, Type type, size_t line) {
    if (type == TYPE_ERROR) {
        TypeSlot *slot = &c->types.items[root(c, var)];
        slot->type = TYPE_ERROR;
        slot->line = line;
        return true;
    }
    return types_unify(c, var, types_new(c, type, line));
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
    if (type_is_list(left) || type_is_list(right)) {  // a list joins text (as "72, 95 and 88"), nothing else
        if (left == TYPE_UNKNOWN || right == TYPE_UNKNOWN) return false;
        if (left == TYPE_TEXT || right == TYPE_TEXT) return types_require(c, d->result, TYPE_TEXT, line);
        report(c, d->expr, NULL, arena_sprintf(c->arena, "I can't use \"and\" between %s and %s.",
                                               ast_type_name(c->arena, left), ast_type_name(c->arena, right)));
        return types_require(c, d->result, TYPE_ERROR, line);
    }
    if (left == TYPE_YESNO || right == TYPE_YESNO) {
        TypeVar other = left == TYPE_YESNO ? d->right : d->left;
        if (types_require(c, other, TYPE_YESNO, line)) return types_require(c, d->result, TYPE_YESNO, line);
        bool with_text = left == TYPE_TEXT || right == TYPE_TEXT;
        report(c, d->expr, with_text ? "To join them, turn the yes/no value into text first with \"as text\"." : NULL,
               arena_sprintf(c->arena, "I can't use \"and\" between %s and %s.", ast_type_name(c->arena, left),
                             ast_type_name(c->arena, right)));
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

static unsigned kind_bit(Type type) {
    if (type_is_list(type)) return KIND_LIST;
    return type == TYPE_NUMBER ? KIND_NUMBER : type == TYPE_TEXT ? KIND_TEXT : type == TYPE_YESNO ? KIND_YESNO : 0;
}

static bool settle_one_of(Checker *c, const Deferred *d) {
    Type type = types_of(c, d->left);
    if (type == TYPE_UNKNOWN) return false;
    if (type != TYPE_ERROR && !(kind_bit(type) & d->allowed)) {
        report(c, d->expr, NULL, arena_sprintf(c->arena, "%s, but this is %s.", d->message, checker_kind(c, d->right)));
    }
    return true;
}

// "followed by": two lists of the same kind make a list; if either side is
// anything else, both are turned into text and joined.
static bool settle_join(Checker *c, const Deferred *d) {
    Type left = types_of(c, d->left), right = types_of(c, d->right);
    size_t line = d->expr->pos.line;
    if (left == TYPE_ERROR || right == TYPE_ERROR) return types_require(c, d->result, TYPE_ERROR, line);
    bool left_known = left != TYPE_UNKNOWN, right_known = right != TYPE_UNKNOWN;
    if ((left_known && !type_is_list(left)) || (right_known && !type_is_list(right))) {
        return types_require(c, d->result, TYPE_TEXT, line);
    }
    if (!left_known || !right_known) return false;  // a list, and a side that may turn out to be one
    if (!types_unify(c, d->left, d->right)) {
        report(c, d->expr, "Only lists of the same kind join into one list. To join them as text, turn one into text "
                           "first with \"as text\".",
               arena_sprintf(c->arena, "I can't join %s with %s.", ast_type_name(c->arena, left),
                             ast_type_name(c->arena, right)));
        return types_require(c, d->result, TYPE_ERROR, line);
    }
    types_unify(c, d->result, d->left);
    return true;
}

static bool settle(Checker *c, Deferred *d) {
    if (d->kind == DEFER_AND) return settle_and(c, d);
    if (d->kind == DEFER_JOIN) return settle_join(c, d);
    if (d->kind == DEFER_ONE_OF) return settle_one_of(c, d);
    Type type = types_of(c, d->left);
    if (type == TYPE_UNKNOWN) return false;
    const char *name = ast_type_name(c->arena, type);
    if ((type == TYPE_YESNO || type_is_list(type)) && d->kind == DEFER_ORDER) {
        report(c, d->expr, NULL, arena_sprintf(c->arena, "I can't compare %s with %s.", name, name));
    } else if (type == TYPE_YESNO || type_is_list(type)) {
        report(c, d->expr, NULL, arena_sprintf(c->arena, "I can't turn %s into a number.", name));
    }
    return true;
}

static void defer(Checker *c, Deferred d) {
    d.done = settle(c, &d);  // usually the kinds are known already
    if (!d.done) vec_push(c->arena, &c->deferred, d);
}

void types_defer(Checker *c, DeferKind kind, const Expr *expr, TypeVar left, TypeVar right, TypeVar result) {
    Deferred d = {kind, expr, left, right, result, false, 0, NULL};
    defer(c, d);
}

void types_defer_one_of(Checker *c, const Expr *expr, TypeVar var, TypeVar shown, unsigned allowed,
                        const char *message) {
    Deferred d = {DEFER_ONE_OF, expr, var, shown, var, false, allowed, message};
    defer(c, d);
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

// What to suggest writing for a kind nothing decided: "a number", or "a list
// of numbers" when it's known to be a list.
static const char *suggested_kind(Checker *c, TypeVar var) {
    Type type = types_of(c, var);
    return type_is_list(type) ? ast_type_name(c->arena, type - TYPE_UNKNOWN + TYPE_NUMBER) : "a number";
}

static bool undecided(Checker *c, TypeVar var) {
    return type_base(types_of(c, var)) == TYPE_UNKNOWN;
}

// "to show with x (a number)", with the given input marked.
static const char *header_with_kind(Checker *c, const Function *f, size_t marked) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_appendf(&sb, "to %s", f->display);
    const Stmt *def = f->definition;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        if (i == 0) sb_appendf(&sb, " %s ", def->as.function.intro ? def->as.function.intro : "with");
        if (i > 0) sb_append(&sb, " and ");
        sb_append(&sb, def->as.function.params.items[i].display);
        if (i == marked) sb_appendf(&sb, " (%s)", suggested_kind(c, f->params.items[i]));
    }
    return sb.data;
}

static void report_undecided(Checker *c, const Function *f) {
    const Stmt *def = f->definition;
    for (size_t i = 0; i < f->params.len; i++) {
        if (!undecided(c, f->params.items[i])) continue;
        const Name *param = &def->as.function.params.items[i];
        diag_error(c->diag, param->pos.span, "I can't tell what kind of value \"%s\" is.", param->display);
        diag_note(c->diag, "Nothing in the program decides it. Say which kind in the definition, like \"%s:\".",
                  header_with_kind(c, f, i));
        types_require(c, f->params.items[i], TYPE_ERROR, 0);  // and everything that depends on it
    }
    if (!undecided(c, f->result)) return;
    diag_error(c->diag, def->as.function.name.pos.span, "I can't tell what kind of value \"%s\" gives back.", f->display);
    diag_note(c->diag, "Say which kind in the definition, like \"%s, giving back %s:\".",
              checker_function_header(c, f), suggested_kind(c, f->result));
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
