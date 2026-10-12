// Type inference: type variables joined with union-find, the operators that
// wait for their sides' kinds, and the last step that reports anything left
// undecided and writes every kind into the program for the code generator.

#include <assert.h>
#include <string.h>
#include "front/check_internal.h"

// --- Variables -------------------------------------------------------------------

static TypeVar new_slot(Checker *c, Type type, size_t line, TypeVar item, TypeVar key) {
    TypeSlot slot = {c->types.len, type, line, item, key, NULL, 0};
    vec_push(c->arena, &c->types, slot);
    return c->types.len - 1;
}

// A variable for a part (a list's items, a map's keys or values), decided where the whole is if it's known.
static TypeVar new_part(Checker *c, Type type, size_t line) {
    return types_new(c, type, type == TYPE_UNKNOWN ? 0 : line);
}

TypeVar types_new(Checker *c, Type type, size_t line) {
    if (type_is_map(type)) {
        TypeVar key = new_part(c, ast_map_key(c->records, type), line);
        return new_slot(c, TYPE_MAP, line, new_part(c, ast_map_value(c->records, type), line), key);
    }
    const FuncKind *kind = type_is_func(type) ? ast_func_kind(c->records, type) : NULL;
    if (kind) {
        TypeVar *params = arena_alloc(c->arena, (kind->count + 1) * sizeof(TypeVar));
        for (size_t i = 0; i < kind->count; i++) params[i] = new_part(c, kind->params[i], line);
        return types_new_func(c, params, kind->count, new_part(c, kind->result, line), line);
    }
    if (!type_is_list(type)) return new_slot(c, type, line, 0, 0);
    return new_slot(c, TYPE_LIST, line, new_part(c, type_item(type), line), 0);
}

TypeVar types_new_func(Checker *c, const TypeVar *params, size_t count, TypeVar result, size_t line) {
    TypeVar *copy = arena_alloc(c->arena, (count + 1) * sizeof(TypeVar));
    for (size_t i = 0; i < count; i++) copy[i] = params[i];
    TypeVar func = new_slot(c, TYPE_FUNC, line, result, 0);
    c->types.items[func].params = copy;
    c->types.items[func].count = count;
    return func;
}

TypeVar types_new_list(Checker *c, TypeVar item, size_t line) {
    return new_slot(c, TYPE_LIST, line, item, 0);
}

TypeVar types_new_map(Checker *c, TypeVar key, TypeVar value, size_t line) {
    return new_slot(c, TYPE_MAP, line, value, key);
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

TypeVar types_key(Checker *c, TypeVar map) {
    return c->types.items[root(c, map)].key;
}

size_t types_func(Checker *c, TypeVar func, const TypeVar **params, TypeVar *result) {
    const TypeSlot *slot = &c->types.items[root(c, func)];
    *params = slot->params;
    *result = slot->item;
    return slot->count;
}

Type types_of(Checker *c, TypeVar var) {
    TypeSlot slot = c->types.items[root(c, var)];  // a copy: making a kind of map can't move it
    if (slot.type == TYPE_MAP) {
        Type key = types_of(c, slot.key), value = types_of(c, slot.item);
        if (key == TYPE_ERROR || value == TYPE_ERROR) return TYPE_ERROR;
        return ast_map_type(c->arena, c->records, key, value);
    }
    if (slot.type == TYPE_FUNC) {
        Type *params = arena_alloc(c->arena, (slot.count + 1) * sizeof(Type));
        for (size_t i = 0; i < slot.count; i++) {
            params[i] = types_of(c, slot.params[i]);
            if (params[i] == TYPE_ERROR) return TYPE_ERROR;
        }
        Type result = types_of(c, slot.item);
        if (result == TYPE_ERROR) return TYPE_ERROR;
        return ast_func_type(c->arena, c->records, params, slot.count, result);
    }
    if (slot.type != TYPE_LIST) return slot.type;
    Type item = types_of(c, slot.item);
    return item == TYPE_ERROR ? TYPE_ERROR : type_list_of(item);
}

size_t types_line(Checker *c, TypeVar var) {
    return c->types.items[root(c, var)].line;
}

// Whether `var` appears in `in` (so joining them would make a list or map
// that holds itself).
static bool occurs(Checker *c, TypeVar var, TypeVar in) {
    TypeVar r = root(c, in);
    if (r == var) return true;
    Type type = c->types.items[r].type;
    if (type == TYPE_MAP && occurs(c, var, c->types.items[r].key)) return true;
    if (type == TYPE_FUNC) {
        for (size_t i = 0; i < c->types.items[r].count; i++) {
            if (occurs(c, var, c->types.items[r].params[i])) return true;
        }
    }
    return (type == TYPE_LIST || type == TYPE_MAP || type == TYPE_FUNC) && occurs(c, var, c->types.items[r].item);
}

static bool compatible(Checker *c, TypeVar a, TypeVar b) {
    TypeVar ra = root(c, a), rb = root(c, b);
    if (ra == rb) return true;
    Type ta = c->types.items[ra].type, tb = c->types.items[rb].type;
    if (ta == TYPE_ERROR || tb == TYPE_ERROR) return true;
    if (ta == TYPE_UNKNOWN) return !occurs(c, ra, rb);
    if (tb == TYPE_UNKNOWN) return !occurs(c, rb, ra);
    if (ta == TYPE_LIST && tb == TYPE_LIST) return compatible(c, c->types.items[ra].item, c->types.items[rb].item);
    if (ta == TYPE_MAP && tb == TYPE_MAP) {
        return compatible(c, c->types.items[ra].key, c->types.items[rb].key) &&
               compatible(c, c->types.items[ra].item, c->types.items[rb].item);
    }
    if (ta == TYPE_FUNC && tb == TYPE_FUNC) {
        const TypeSlot *a_slot = &c->types.items[ra], *b_slot = &c->types.items[rb];
        if (a_slot->count != b_slot->count) return false;
        for (size_t i = 0; i < a_slot->count; i++) {
            if (!compatible(c, a_slot->params[i], b_slot->params[i])) return false;
        }
        return compatible(c, a_slot->item, b_slot->item);
    }
    // A record or nothing, kind not decided yet, goes with any record or nothing.
    if (ta == TYPE_MAYBE) return tb == TYPE_MAYBE || type_is_maybe(tb);
    if (tb == TYPE_MAYBE) return type_is_maybe(ta);
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
    } else if (tb == TYPE_ERROR || ta == TYPE_UNKNOWN || ta == TYPE_MAYBE) {
        c->types.items[ra].parent = rb;
    } else {
        if (ta == TYPE_LIST || ta == TYPE_MAP || ta == TYPE_FUNC) {
            join(c, c->types.items[ra].item, c->types.items[rb].item);
        }
        if (ta == TYPE_MAP) join(c, c->types.items[ra].key, c->types.items[rb].key);
        for (size_t i = 0; ta == TYPE_FUNC && i < c->types.items[ra].count; i++) {
            join(c, c->types.items[ra].params[i], c->types.items[rb].params[i]);
        }
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
static unsigned kind_bit(Type type) {
    if (type_is_list(type)) return KIND_LIST;
    if (type_is_map(type)) return KIND_MAP;
    if ((type_is_record(type) || type_is_sum(type)) && !type_is_maybe(type)) return KIND_RECORD;
    if (type_is_object(type) || type == TYPE_MAYBE) return KIND_OTHER;
    if (type == TYPE_DATE) return KIND_DATE;
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
               arena_sprintf(c->arena, "I can't join %s with %s.", checker_type_name(c, left),
                             checker_type_name(c, right)));
        return types_require(c, d->result, TYPE_ERROR, line);
    }
    types_unify(c, d->result, d->left);
    return true;
}

static bool settle(Checker *c, Deferred *d) {
    if (d->kind == DEFER_JOIN) return settle_join(c, d);
    if (d->kind == DEFER_ONE_OF) return settle_one_of(c, d);
    if (d->kind == DEFER_SEARCH) return checker_settle_search(c, d);
    if (d->kind == DEFER_FIELD) return checker_settle_field(c, d);
    if (d->kind == DEFER_PRESENT) return checker_settle_present(c, d);
    if (d->kind == DEFER_METHOD) return checker_settle_method(c, d);
    Type type = types_of(c, d->left);
    if (type == TYPE_UNKNOWN) return false;
    const char *name = checker_type_name(c, type);
    bool other = type == TYPE_YESNO || type_is_list(type) || type_is_object(type) || type_is_map(type) ||
                 type == TYPE_MAYBE;
    if (other && d->kind == DEFER_ORDER) {
        report(c, d->expr, NULL, arena_sprintf(c->arena, "I can't compare %s with %s.", name, name));
    } else if (other) {
        report(c, d->expr, NULL, arena_sprintf(c->arena, "I can't turn %s into a number.", name));
    }
    return true;
}

static void defer(Checker *c, Deferred d) {
    d.done = settle(c, &d);  // usually the kinds are known already
    if (!d.done) vec_push(c->arena, &c->deferred, d);
}

void types_defer(Checker *c, DeferKind kind, const Expr *expr, TypeVar left, TypeVar right, TypeVar result) {
    Deferred d = {kind, expr, left, right, result, false, 0, NULL, NULL};
    defer(c, d);
}

void types_defer_one_of(Checker *c, const Expr *expr, TypeVar var, TypeVar shown, unsigned allowed,
                        const char *message) {
    Deferred d = {DEFER_ONE_OF, expr, var, shown, var, false, allowed, message, NULL};
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

// Whether `var` is still waiting in a "contains" or "the position of",
// which works on text.
static bool searched(Checker *c, TypeVar var) {
    for (size_t i = 0; i < c->deferred.len; i++) {
        const Deferred *d = &c->deferred.items[i];
        if (!d->done && d->kind == DEFER_SEARCH && (root(c, d->left) == root(c, var) || root(c, d->right) == root(c, var))) {
            return true;
        }
    }
    return false;
}

// The kind of record whose field `var` is waiting to be looked up in, if any.
static const RecordKind *field_owner(Checker *c, TypeVar var) {
    for (size_t i = 0; i < c->deferred.len; i++) {
        const Deferred *d = &c->deferred.items[i];
        if (!d->done && d->kind == DEFER_FIELD && root(c, d->left) == root(c, var)) {
            return checker_field_owner(c, d->expr->as.field.name.text);
        }
    }
    return NULL;
}

// A kind with its undecided parts filled in, to suggest: numbers, and text
// for a map's keys ("a list of numbers", "a map from text to numbers").
static Type filled_in(Checker *c, Type type, Type unknown) {
    if (type_is_list(type)) return type_list_of(filled_in(c, type_item(type), TYPE_NUMBER));
    if (type_is_map(type)) {
        Type key = filled_in(c, ast_map_key(c->records, type), TYPE_TEXT);  // key first: kinds are added in order
        return ast_map_type(c->arena, c->records, key, filled_in(c, ast_map_value(c->records, type), TYPE_NUMBER));
    }
    return type == TYPE_UNKNOWN ? unknown : type;
}

// What to suggest writing for a kind nothing decided: "a number", "text"
// for what's searched, "a list of numbers" when it's known to be a list (a
// map likewise), or a kind of record that has the field it's asked for ("a
// dog", or "a dog or nothing" if it can be nothing).
static const char *suggested_kind(Checker *c, TypeVar var) {
    if (searched(c, var)) return "text";
    Type type = types_of(c, var);
    const RecordKind *owner = field_owner(c, var);
    if (!owner && type == TYPE_MAYBE && c->records->len > 0) owner = &c->records->items[0];
    if (owner) return arena_sprintf(c->arena, "a %s%s", owner->display, type == TYPE_MAYBE ? " or nothing" : "");
    return checker_type_name(c, filled_in(c, type, TYPE_NUMBER));
}

static bool undecided(Checker *c, TypeVar var) {
    return ast_type_undecided(c->records, types_of(c, var));
}

// "to show with x (a number)", with the given input marked.
static const char *header_with_kind(Checker *c, const Function *f, size_t marked) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_appendf(&sb, "to %s", f->display);
    const Stmt *def = f->definition;
    size_t first = def->as.function.method ? 1 : 0;  // a method: "to bark, for a dog"
    if (first) sb_appendf(&sb, ", for %s", checker_type_name(c, def->as.function.declared.items[0]));
    for (size_t i = first; i < def->as.function.params.len; i++) {
        if (i == first) sb_appendf(&sb, "%s %s ", first ? "," : "", def->as.function.intro ? def->as.function.intro : "with");
        if (i > first) sb_append(&sb, " and ");
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
    for (size_t i = 0; diag_count(c->diag) == first_error && i < c->deferred.len; i++) {
        if (!c->deferred.items[i].done && c->deferred.items[i].kind == DEFER_METHOD) {
            checker_report_unsettled_method(c, &c->deferred.items[i]);
        }
    }
    for (size_t i = 0; diag_count(c->diag) == first_error && i < c->functions.len; i++) {
        report_undecided(c, &c->functions.items[i]);
    }
    if (diag_count(c->diag) == first_error) checker_report_undecided_lambdas(c);
    if (diag_count(c->diag) == first_error) checker_report_undecided_fields(c);
    for (size_t i = 0; i < c->typed.len; i++) {
        c->typed.items[i].expr->type = types_of(c, c->typed.items[i].var);
    }
    for (size_t i = 0; i < c->slots.len; i++) {
        *c->slots.items[i].slot = types_of(c, c->slots.items[i].var);
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
    checker_write_lambdas(c);
}
