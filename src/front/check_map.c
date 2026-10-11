// Checking maps: a map has one kind of key (text or numbers) and one kind of
// value, so every map value has a type like "a map from text to numbers",
// and every sentence that looks up, sets or removes a key checks it's the
// right kind.

#include <stdlib.h>
#include <string.h>
#include "front/check_internal.h"
#include "front/consteval.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

// The checker writes the kind into the expression at the end.
static void record_kind(Checker *c, const Expr *expr, TypeVar var) {
    TypedExpr t = {(Expr *)expr, var};
    vec_push(c->arena, &c->typed, t);
}

// "\"ages\"" for a name, else "This map".
static const char *subject(Checker *c, const Expr *map) {
    return map->kind == EXPR_NAME ? arena_sprintf(c->arena, "\"%s\"", map->display) : "This map";
}

static const char *of_map(Checker *c, const Expr *map) {
    return map->kind == EXPR_NAME ? arena_sprintf(c->arena, "\"%s\"", map->display) : "this map";
}

bool checker_require_map(Checker *c, TypeVar var, const Expr *at, const char *message, const char *list_note,
                         TypeVar *key, TypeVar *value) {
    *key = types_new(c, TYPE_UNKNOWN, 0);
    *value = types_new(c, TYPE_UNKNOWN, 0);
    bool map = types_unify(c, var, types_new_map(c, *key, *value, at->pos.line));
    if (map && types_of(c, var) != TYPE_ERROR) return true;
    if (types_of(c, var) != TYPE_ERROR) {
        diag_error(c->diag, at->pos.span, "%s, but this is %s.", message, checker_kind(c, var));
        if (list_note && type_is_list(types_of(c, var))) diag_note(c->diag, "%s", list_note);
    }
    *key = *value = error_var(c);
    return false;
}

// A key used with a map has to be the kind of its keys, which are text or numbers.
static void require_key(Checker *c, const Expr *map, TypeVar key, const Expr *expr, TypeVar var) {
    if (!types_unify(c, key, var)) {
        const char *keys = types_of(c, key) == TYPE_TEXT ? "text" : "numbers";
        diag_error(c->diag, expr->pos.span, "The keys of %s are %s, but this is %s.", of_map(c, map), keys,
                   checker_kind(c, var));
        return;
    }
    types_defer_one_of(c, expr, var, var, KIND_TEXT | KIND_NUMBER, "A map's keys have to be text or numbers");
}

// A value put into a map has to be the kind it holds.
static void require_value(Checker *c, const Expr *map, TypeVar map_var, TypeVar value, const Expr *expr, TypeVar var) {
    if (types_unify(c, value, var)) return;
    diag_error(c->diag, expr->pos.span, "%s is %s, so it can't hold %s.", subject(c, map), checker_kind(c, map_var),
               checker_kind(c, var));
    diag_note(c->diag, "A map holds one kind of value. To keep %s too, make a separate map for it.", checker_kind(c, var));
}

// --- Values --------------------------------------------------------------------------

// A key written out ("coffee", 3) as it's shown in messages, or NULL for any other key.
static const char *literal_key(Checker *c, const Expr *key) {
    if (key->kind == EXPR_TEXT) return arena_sprintf(c->arena, "\"%.*s\"", (int)key->as.text.len, key->as.text.value);
    if (key->kind != EXPR_NUMBER) return NULL;
    // numbers that are the same to 12 significant digits are one key
    return const_key_text(c->arena, strtod(key->as.number.text, NULL));
}

// The same key written twice in one map ("a map of "tea" to 2, "tea" to 3").
static void report_duplicate_keys(Checker *c, const Expr *expr) {
    size_t n = expr->as.items.len / 2;
    for (size_t j = 1; j < n; j++) {
        const Expr *later = expr->as.items.items[2 * j];
        const char *shown = literal_key(c, later);
        for (size_t i = 0; shown && i < j; i++) {
            const Expr *earlier = expr->as.items.items[2 * i];
            const char *first = literal_key(c, earlier);
            if (!first || earlier->kind != later->kind || strcmp(first, shown) != 0) continue;
            diag_error(c->diag, later->pos.span, "The key %s is in this map twice.", shown);
            diag_note(c->diag, "Each key has one value, so give each key once.");
            break;
        }
    }
}

static TypeVar check_literal(Checker *c, const Expr *expr) {
    TypeVar first_key = 0, first_value = 0;
    for (size_t i = 0; i < expr->as.items.len; i += 2) {
        const Expr *key = expr->as.items.items[i], *value = expr->as.items.items[i + 1];
        TypeVar key_var = checker_check_expr(c, key), value_var = checker_check_expr(c, value);
        if (i == 0) {
            first_key = key_var, first_value = value_var;
            types_defer_one_of(c, key, key_var, key_var, KIND_TEXT | KIND_NUMBER,
                               "A map's keys have to be text or numbers");
            continue;
        }
        if (!types_unify(c, first_key, key_var)) {
            diag_error(c->diag, key->pos.span, "All the keys of a map have to be the same kind: the first is %s, but this "
                                               "is %s.", checker_kind(c, first_key), checker_kind(c, key_var));
        }
        if (!types_unify(c, first_value, value_var)) {
            diag_error(c->diag, value->pos.span, "All the values of a map have to be the same kind: the first is %s, but "
                                                 "this is %s.", checker_kind(c, first_value), checker_kind(c, value_var));
        }
    }
    report_duplicate_keys(c, expr);
    return types_new_map(c, first_key, first_value, expr->pos.line);
}

// the value for K in M [, or D]
static TypeVar check_lookup(Checker *c, const Expr *expr) {
    const Expr *map = expr->as.lookup.map, *fallback = expr->as.lookup.fallback;
    TypeVar key_var = checker_check_expr(c, expr->as.lookup.key);
    TypeVar map_var = checker_check_expr(c, map);
    TypeVar key, value;
    if (!checker_require_map(c, map_var, map, "I can only look up a value in a map",
                             "To take an item from a list, write \"item 2 of scores\".", &key, &value)) {
        if (fallback) checker_check_expr(c, fallback);
        return value;
    }
    require_key(c, map, key, expr->as.lookup.key, key_var);
    if (!fallback) return value;
    TypeVar fallback_var = checker_check_expr(c, fallback);
    if (!types_unify(c, value, fallback_var)) {
        diag_error(c->diag, fallback->pos.span, "%s is %s, so the value after \", or\" has to be %s, but this is %s.",
                   subject(c, map), checker_kind(c, map_var), checker_kind(c, value), checker_kind(c, fallback_var));
    }
    return value;
}

// M without K: a new map like M, without K.
static TypeVar check_without(Checker *c, const Expr *expr) {
    const Expr *map = expr->as.lookup.map;
    TypeVar map_var = checker_check_expr(c, map);
    TypeVar key_var = checker_check_expr(c, expr->as.lookup.key);
    TypeVar key, value;
    if (checker_require_map(c, map_var, map, "I can only leave a key out of a map",
                            "To leave out an item of a list, write \"scores without item 2\".", &key, &value)) {
        require_key(c, map, key, expr->as.lookup.key, key_var);
    }
    return map_var;
}

TypeVar checker_check_map_expr(Checker *c, const Expr *expr) {
    switch (expr->kind) {
    case EXPR_MAP: return check_literal(c, expr);
    case EXPR_EMPTY_MAP: return types_new(c, expr->as.list_type, expr->pos.line);
    case EXPR_MAP_GET: return check_lookup(c, expr);
    case EXPR_MAP_WITHOUT: return check_without(c, expr);
    default: return error_var(c);
    }
}

TypeVar checker_check_has(Checker *c, const Expr *expr, TypeVar left, TypeVar right) {
    TypeVar key, value;
    if (checker_require_map(c, left, expr->as.binary.left, "I can only check whether a map has a key",
                            "To check whether a list holds something, use \"contains\".", &key, &value)) {
        require_key(c, expr->as.binary.left, key, expr->as.binary.right, right);
    }
    return types_new(c, TYPE_YESNO, expr->pos.line);
}

TypeVar checker_check_map_op(Checker *c, const Expr *expr, TypeVar var) {
    const Expr *map = expr->as.list_op.operand;
    TypeVar key, value;
    switch (expr->as.list_op.op) {
    case LIST_ENTRIES:
        checker_require_map(c, var, map, "I can only count the entries of a map",
                            "To count the items of a list, write \"the number of items in scores\".", &key, &value);
        return types_new(c, TYPE_NUMBER, expr->pos.line);
    case LIST_KEYS:
        checker_require_map(c, var, map, "I can only take the keys of a map", NULL, &key, &value);
        return types_new_list(c, key, expr->pos.line);
    default:
        checker_require_map(c, var, map, "I can only take the values of a map", NULL, &key, &value);
        return types_new_list(c, value, expr->pos.line);
    }
}

// --- Sentences -----------------------------------------------------------------------

TypeVar checker_check_each(Checker *c, const Stmt *stmt, TypeVar *value) {
    const Expr *list = stmt->as.loop.list;
    TypeVar var = checker_check_expr(c, list), key;
    if (stmt->as.loop.pair) {
        checker_require_map(c, var, list, "A \"for each\" loop with two names needs a map to go through",
                            "A list has one item at a time, so name it once, like \"for each score in scores:\".", &key,
                            value);
        return key;
    }
    if (type_is_map(types_of(c, var))) {
        diag_error(c->diag, list->pos.span, "A map has a key and a value in each entry, so going through it takes two "
                                            "names.");
        diag_note(c->diag, "Name both, like \"for each name and age in ages:\". To go through only the keys, write "
                           "\"for each name in the keys of ages:\".");
        return error_var(c);
    }
    return checker_require_list(c, var, list, "A \"for each\" loop needs a list to go through");
}

void checker_check_remove_key(Checker *c, const Stmt *stmt) {
    const Expr *map = stmt->as.list.list;
    if (checker_reject_fixed(c, map)) return;
    TypeVar key_var = checker_check_expr(c, stmt->as.list.value);
    TypeVar key, value;
    if (checker_require_map(c, checker_check_expr(c, map), map, "I can only remove a key from a map",
                            "To remove an item from a list, write \"remove item 2 from scores\".", &key, &value)) {
        require_key(c, map, key, stmt->as.list.value, key_var);
    }
}

// The value in a map that a sentence changes ("the value for K in M"): its
// kind, as the map holds it. A fixed map can't change.
static TypeVar check_target(Checker *c, const Expr *target, TypeVar *map_var) {
    const Expr *map = target->as.lookup.map;
    *map_var = error_var(c);
    if (checker_reject_fixed(c, map)) return error_var(c);
    TypeVar key_var = checker_check_expr(c, target->as.lookup.key);
    *map_var = checker_check_expr(c, map);
    TypeVar key, value;
    if (checker_require_map(c, *map_var, map, "I can only set a value in a map",
                            "To change an item of a list, write \"set item 2 of scores to ...\".", &key, &value)) {
        require_key(c, map, key, target->as.lookup.key, key_var);
    }
    record_kind(c, target, value);
    return value;
}

void checker_check_map_set(Checker *c, const Expr *target, const Expr *value_expr, TypeVar value_var) {
    TypeVar map_var;
    TypeVar value = check_target(c, target, &map_var);
    if (types_of(c, value) != TYPE_ERROR) require_value(c, target->as.lookup.map, map_var, value, value_expr, value_var);
}

TypeVar checker_check_map_change(Checker *c, const Expr *target) {
    TypeVar map_var;
    return check_target(c, target, &map_var);
}
