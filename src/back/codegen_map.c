// C code for maps: map values, "has", the sentences that change maps,
// "for each K and V in M" loops, and fixed maps (constants).
//
// A map is an EsMap * (runtime/es_map.h), reference counted like lists. A
// key is passed as an EsValue, (EsValue){.t = KEY} or (EsValue){.n = KEY},
// because the map knows what its keys are; operations on values have one
// runtime function per kind of value, named with a suffix like the list
// ones: es_map_get_n (numbers), es_map_get_t (text), es_map_get_m (maps)...

#include "back/codegen_internal.h"

static Type key_type(const Codegen *g, Type map) {
    return ast_map_key(g->records, map);
}

static Type value_type(const Codegen *g, Type map) {
    return ast_map_value(g->records, map);
}

// "(EsValue){.t = KEY}"
static const char *key_code(const Codegen *g, Type map, const char *key) {
    return arena_sprintf(g->arena, "(EsValue){.%c = %s}", key_type(g, map) == TYPE_TEXT ? 't' : 'n', key);
}

// es_map_new(ES_ITEMS_TEXT, ES_ITEMS_NUMBER, ...: what the runtime calls the kinds of a map's keys and values.
static const char *kinds_of(const Codegen *g, Type key, Type value) {
    return arena_sprintf(g->arena, "%s, %s", cg_items(key), cg_items(value));
}

static const char *kinds_code(const Codegen *g, Type map) {
    return kinds_of(g, key_type(g, map), value_type(g, map));
}

// --- Values -------------------------------------------------------------------------

// es_map_from(KEYS, VALUES, 2, (EsValue[]){{.t = &es_s1}, {.n = 3.0}, ...}), the keys and
// values worked out in the order they're written.
static void emit_literal(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t n = expr->as.items.len;
    const char **values = arena_alloc(g->arena, n * sizeof(char *));
    bool opened = cg_ordered(g, expr->as.items.items, n, values, out);
    Type value = value_type(g, expr->type);
    sb_appendf(out, "es_map_from(%s, %zu, (EsValue[]){", kinds_code(g, expr->type), n / 2);
    for (size_t i = 0; i < n; i += 2) {
        sb_appendf(out, "%s{.%c = %s}, {.%s = %s}", i ? ", " : "", key_type(g, expr->type) == TYPE_TEXT ? 't' : 'n',
                   values[i], cg_value_suffix(value), values[i + 1]);
    }
    sb_append(out, opened ? "}))" : "})");
}

// es_map_get_n(SITE, MAP, KEY), or with ", or D":
// (es_map_lookup_n(MAP, KEY, &es_t1) ? es_t1 : D), D worked out only when the key is missing.
static void emit_lookup(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *map = expr->as.lookup.map;
    Expr *parts[] = {expr->as.lookup.key, (Expr *)map};
    const char *v[2];
    bool opened = cg_ordered(g, parts, 2, v, out);
    if (!opened && map->kind == EXPR_NAME) v[1] = cg_list_operand(g, map);  // a fixed map is used as it is
    const char *suffix = cg_value_suffix(expr->type);
    const char *key = key_code(g, map->type, v[0]);
    if (!expr->as.lookup.fallback) {
        sb_appendf(out, "es_map_get_%s(%zu, %s, %s)", suffix, cg_site(g, expr->pos), v[1], key);
    } else {
        size_t t = cg_new_temp(g, expr->type);
        sb_appendf(out, "(es_map_lookup_%s(%s, %s, &es_t%zu) ? es_t%zu : %s)", suffix, v[1], key, t, t,
                   cg_operand(g, expr->as.lookup.fallback, false));
    }
    if (opened) sb_append_char(out, ')');
}

void cg_emit_map_expr(Codegen *g, const Expr *expr, StrBuf *out) {
    switch (expr->kind) {
    case EXPR_MAP: emit_literal(g, expr, out); break;
    case EXPR_EMPTY_MAP: sb_appendf(out, "es_map_new(%s, 0)", kinds_code(g, expr->type)); break;
    default: emit_lookup(g, expr, out); break;
    }
}

// M has K: es_map_has(MAP, KEY)
void cg_emit_has(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *map = expr->as.binary.left;
    Expr *parts[] = {(Expr *)map, expr->as.binary.right};
    const char *v[2];
    bool opened = cg_ordered(g, parts, 2, v, out);
    if (!opened && map->kind == EXPR_NAME) v[0] = cg_list_operand(g, map);
    sb_appendf(out, "%ses_map_has(%s, %s)%s", expr->as.binary.op == BINARY_NOT_HAS ? "!" : "", v[0],
               key_code(g, map->type, v[1]), opened ? ")" : "");
}

// The number of entries, the keys, the values; whether it's empty; a copy.
void cg_emit_map_op(Codegen *g, const Expr *expr, StrBuf *out) {
    const char *map = cg_list_operand(g, expr->as.list_op.operand);
    switch (expr->as.list_op.op) {
    case LIST_ENTRIES: sb_appendf(out, "es_map_length(%s)", map); break;
    case LIST_KEYS: sb_appendf(out, "es_map_list(%s, true)", map); break;
    case LIST_VALUES: sb_appendf(out, "es_map_list(%s, false)", map); break;
    case LIST_IS_EMPTY: sb_appendf(out, "es_map_is_empty(%s)", map); break;
    case LIST_NOT_EMPTY: sb_appendf(out, "!es_map_is_empty(%s)", map); break;
    default: sb_appendf(out, "es_map_copy(%s)", map); break;  // LIST_COPY
    }
}

// --- Fixed maps -------------------------------------------------------------------

// A worked-out key or value as C.
static const char *fixed_value(Codegen *g, const ConstValue *v) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    if (v->kind == CONST_TEXT) {
        sb_appendf(&sb, "&es_s%zu", cg_static_text(g, v->text, v->len));
    } else if (v->kind == CONST_YESNO) {
        sb_append(&sb, v->yes ? "true" : "false");
    } else {
        cg_append_number(&sb, v->number);
    }
    return sb.data;
}

// static EsMap *es_k_prices; made (and marked immortal, so it's never
// counted, freed or changed) when main() starts:
//   es_k_prices = es_map_fix(es_map_from(ES_ITEMS_TEXT, ES_ITEMS_NUMBER, 2, (EsValue[]){...}));
void cg_emit_fixed_map(Codegen *g, const char *name, const ConstValue *v, StrBuf *out) {
    sb_append(out, "static EsMap *");
    cg_append_prefixed(out, "es_k_", name);
    sb_append(out, ";\n");
    StrBuf *init = g->inits;
    sb_append(init, "    ");
    cg_append_prefixed(init, "es_k_", name);
    const char *kinds = kinds_of(g, v->key_type, v->value_type);
    if (v->len == 0) {
        sb_appendf(init, " = es_map_fix(es_map_new(%s, 0));\n", kinds);
        return;
    }
    sb_appendf(init, " = es_map_fix(es_map_from(%s, %zu, (EsValue[]){", kinds, v->len);
    for (size_t i = 0; i < v->len; i++) {
        sb_appendf(init, "%s{.%c = %s}, {.%s = %s}", i ? ", " : "", v->key_type == TYPE_TEXT ? 't' : 'n',
                   fixed_value(g, &v->keys[i]), cg_value_suffix(v->value_type), fixed_value(g, &v->items[i]));
    }
    sb_append(init, "}));\n");
}

// --- "for each K and V in M" ---------------------------------------------------------

// One line setting a loop name from the entry: "es_vn_age = es_vh0_0each1->entries[es_i1].value.n;".
static void store_part(StrBuf *body, const char *name, Type type, const char *part) {
    if (type_counted(type)) {
        sb_appendf(body, "%s(&", cg_set(type));
        cg_append_var(body, name, type);
        sb_appendf(body, ", %s(%s));\n", cg_retain(type), part);
    } else {
        cg_append_var(body, name, type);
        sb_appendf(body, " = %s;\n", part);
    }
}

// The loop goes through the map as it was when the loop started (kept in a
// hidden variable, like a list's), skipping removed entries.
void cg_emit_each_map_head(Codegen *g, const Stmt *stmt, size_t id, StrBuf *body, size_t depth) {
    Type map = stmt->as.loop.list->type;
    const char *hidden = arena_sprintf(g->arena, "0each%zu", id);
    cg_declare_name(g, hidden, map);
    const char *value = cg_list_operand(g, stmt->as.loop.list);
    StrBuf entries;
    sb_init(&entries, g->arena);
    cg_append_var(&entries, hidden, map);
    sb_append_repeat(body, ' ', 4 * (depth + 1));
    sb_appendf(body, "es_map_set(&%s, es_map_snapshot(%s));\n", entries.data, value);
    sb_append_repeat(body, ' ', 4 * (depth + 1));
    sb_appendf(body, "for (size_t es_i%zu = 0; es_i%zu < %s->used; es_i%zu++) {\n", id, id, entries.data, id);
    const char *entry = arena_sprintf(g->arena, "%s->entries[es_i%zu]", entries.data, id);
    sb_append_repeat(body, ' ', 4 * (depth + 2));
    sb_appendf(body, "if (%s.hash == 0) continue; /* removed */\n", entry);
    Type key = key_type(g, map), val = value_type(g, map);
    sb_append_repeat(body, ' ', 4 * (depth + 2));
    const char *key_part = arena_sprintf(g->arena, "%s.key.%c", entry, key == TYPE_TEXT ? 't' : 'n');
    store_part(body, stmt->as.loop.var.text, key, key_part);
    sb_append_repeat(body, ' ', 4 * (depth + 2));
    const char *value_part = arena_sprintf(g->arena, "%s.value.%s", entry, cg_value_suffix(val));
    store_part(body, stmt->as.loop.value_name.text, val, value_part);
}

// --- Sentences ------------------------------------------------------------------------

// remove K from M: es_map_remove(SITE, MAP, KEY)
const char *cg_remove_key(Codegen *g, const Stmt *stmt) {
    StrBuf out;
    sb_init(&out, g->arena);
    Expr *parts[] = {stmt->as.list.value, stmt->as.list.list};
    const char *v[2];
    bool opened = cg_ordered(g, parts, 2, v, &out);
    sb_appendf(&out, "es_map_remove(%zu, %s, %s)%s", cg_site(g, stmt->as.list.value->pos), v[1],
               key_code(g, stmt->as.list.list->type, v[0]), opened ? ")" : "");
    return out.data;
}

// set the value for K in M to V: es_map_put_n(MAP, KEY, VALUE);
static const char *set_code(Codegen *g, const Stmt *stmt) {
    const Expr *target = stmt->as.assign.field;
    StrBuf out;
    sb_init(&out, g->arena);
    Expr *parts[] = {target->as.lookup.key, target->as.lookup.map, stmt->as.assign.value};
    const char *v[3];
    bool opened = cg_ordered(g, parts, 3, v, &out);
    sb_appendf(&out, "es_map_put_%s(%s, %s, %s)%s;", cg_value_suffix(target->type), v[1],
               key_code(g, target->as.lookup.map->type, v[0]), v[2], opened ? ")" : "");
    return out.data;
}

// add V to the value for K in M (and the other changes): one C block that
// works out the parts in the order they're written, then finds the value
// (after V, which could change the map) and changes it in place.
static const char *change_code(Codegen *g, const Stmt *stmt) {
    const Expr *target = stmt->as.change.field, *amount = stmt->as.change.amount;
    Type map = target->as.lookup.map->type, type = target->type;
    StrBuf out;
    sb_init(&out, g->arena);
    const char *amount_code = arena_sprintf(g->arena, "%s%ses_value = %s; ", cg_ctype(amount->type),
                                            type_counted(amount->type) ? "" : " ", cg_operand(g, amount, false));
    bool amount_first = amount->pos.span.offset < target->pos.span.offset;
    sb_appendf(&out, "{ %s", amount_first ? amount_code : "");
    sb_appendf(&out, "EsValue es_k = {.%c = %s}; ", key_type(g, map) == TYPE_TEXT ? 't' : 'n',
               cg_operand(g, target->as.lookup.key, false));
    sb_appendf(&out, "EsMap *es_m = %s; ", cg_list_operand(g, target->as.lookup.map));
    if (!amount_first) sb_append(&out, amount_code);
    sb_appendf(&out, "EsValue *es_p = es_map_at(%zu, es_m, es_k); ", cg_site(g, target->pos));
    if (stmt->as.change.append) {
        sb_appendf(&out, "es_add_%s(es_list_retain(es_p->l), es_value); ", cg_item_suffix(type));
    } else if (stmt->as.change.op == CHANGE_DIVIDE) {
        sb_appendf(&out, "es_p->n = es_div(%zu, es_p->n, es_value); ", cg_site(g, stmt->pos));
    } else {
        static const char *const ops[] = {"+", "-", "*"};
        sb_appendf(&out, "es_p->n = es_p->n %s es_value; ", ops[stmt->as.change.op]);
    }
    sb_append(&out, "es_map_release(es_m); }");
    return out.data;
}

const char *cg_map_stmt(Codegen *g, const Stmt *stmt) {
    return stmt->kind == STMT_SET ? set_code(g, stmt) : change_code(g, stmt);
}
