// C code for lists: list values, the sentences that change lists, "for each"
// loops, and the helper functions "every" and "only" become.
//
// A list is an EsList * (runtime/es_list.h), reference counted like text: a
// list expression produces an owned list, and the runtime's list functions
// consume the lists (and text) they're given. Operations on items have one
// runtime function per kind of item, named with a suffix: es_get_n (numbers),
// es_get_b (yes/no), es_get_t (text), es_get_l (lists).
//
// "every NAME in L ..." and "only L that ..." work something out for each
// item, so each becomes a small C function that loops over the list:
//
//   static EsList *es_every1(EsList *es_list, double es_vn_bonus) {...}
//
// Its parameters are the variables the expression reads (C has no closures),
// passed without retaining: the caller holds them while it runs.

#include <assert.h>
#include <string.h>
#include "back/codegen_internal.h"

const char *cg_item_suffix(Type list) {
    Type item = type_item(list);
    return type_is_list(item) ? "l" : item == TYPE_TEXT ? "t" : item == TYPE_YESNO ? "b" : "n";
}

// ES_ITEMS_NUMBER, ...: what the runtime calls the kind of a list's items.
static const char *items_kind(Type list) {
    Type item = type_item(list);
    return type_is_list(item) ? "ES_ITEMS_LIST" : item == TYPE_TEXT ? "ES_ITEMS_TEXT"
                                              : item == TYPE_YESNO ? "ES_ITEMS_YESNO" : "ES_ITEMS_NUMBER";
}

static const char *where_code(ItemWhere where) {
    return where == ITEM_FIRST ? "ES_FIRST" : where == ITEM_LAST ? "ES_LAST" : "ES_AT";
}

// --- Values -------------------------------------------------------------------------

// es_list_from(ES_ITEMS_NUMBER, 3, (double[]){72.0, 95.0, 88.0}). The items
// are worked out left to right (through temporaries if two have effects);
// the list takes the text and lists among them.
static void emit_literal(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t n = expr->as.items.len;
    const char **values = arena_alloc(g->arena, n * sizeof(char *));
    bool opened = cg_ordered(g, expr->as.items.items, n, values, out);
    sb_appendf(out, "es_list_from(%s, %zu, (%s[]){", items_kind(expr->type), n, cg_ctype(type_item(expr->type)));
    for (size_t i = 0; i < n; i++) sb_appendf(out, "%s%s", i ? ", " : "", values[i]);
    sb_append(out, opened ? "}))" : "})");
}

// es_get_n(SITE, LIST, ES_AT, POSITION): the position is written first, so
// it's worked out first.
static void emit_item(Codegen *g, const Expr *expr, StrBuf *out) {
    Expr *parts[] = {expr->as.item.position, expr->as.item.list};
    const char *values[2] = {"0", NULL};
    bool opened = cg_ordered(g, parts, 2, values, out);
    sb_appendf(out, "es_get_%s(%zu, %s, %s, %s)%s", cg_item_suffix(expr->as.item.list->type), cg_site(g, expr->pos),
               values[1], where_code(expr->as.item.where), values[0], opened ? ")" : "");
}

static void emit_list_op(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *operand = expr->as.list_op.operand;
    const char *code = cg_operand(g, operand, false);
    switch (expr->as.list_op.op) {
    case LIST_SUM: sb_appendf(out, "es_list_sum(%s)", code); break;
    case LIST_BIGGEST:
    case LIST_SMALLEST:
        sb_appendf(out, "es_extreme_%s(%zu, %s, %d)", cg_item_suffix(operand->type), cg_site(g, expr->pos), code,
                   expr->as.list_op.op == LIST_BIGGEST);
        break;
    case LIST_REVERSED: sb_appendf(out, "es_list_reversed(%s)", code); break;
    case LIST_COPY: sb_appendf(out, "es_list_copy(%s)", code); break;
    case LIST_IS_EMPTY:
    case LIST_NOT_EMPTY:
        sb_appendf(out, "%s%s(%s)", expr->as.list_op.op == LIST_NOT_EMPTY ? "!" : "",
                   type_is_list(operand->type) ? "es_list_is_empty" : "es_text_is_empty", code);
        break;
    }
}

// Slices, their parts worked out in the order they're written:
// es_slice_range(SITE, L, FROM, TO, to_end), es_slice_end(SITE, L, COUNT,
// last), es_slice_without(SITE, L, last), es_slice_every(L, STEP, FIRST).
static void emit_slice(Codegen *g, const Expr *expr, StrBuf *out) {
    SliceKind kind = expr->as.slice.kind;
    Expr *parts[] = {expr->as.slice.from, expr->as.slice.to, expr->as.slice.list};
    const char *v[3] = {"0", "0", NULL};
    bool opened = cg_ordered(g, parts, 3, v, out);
    size_t site = kind == SLICE_EVERY ? 0 : cg_site(g, expr->pos);  // SLICE_EVERY can't fail
    switch (kind) {
    case SLICE_RANGE:
        sb_appendf(out, "es_slice_range(%zu, %s, %s, %s, %d)", site, v[2], v[0], v[1], expr->as.slice.to == NULL);
        break;
    case SLICE_FIRST:
    case SLICE_LAST: sb_appendf(out, "es_slice_end(%zu, %s, %s, %d)", site, v[2], v[0], kind == SLICE_LAST); break;
    case SLICE_WITHOUT_FIRST:
    case SLICE_WITHOUT_LAST: sb_appendf(out, "es_slice_without(%zu, %s, %d)", site, v[2], kind == SLICE_WITHOUT_LAST); break;
    case SLICE_EVERY:
        sb_appendf(out, "es_slice_every(%s, %zu, %zu)", v[2], expr->as.slice.step, expr->as.slice.start);
        break;
    }
    if (opened) sb_append_char(out, ')');
}

void cg_emit_contains(Codegen *g, const Expr *expr, StrBuf *out) {
    Expr *parts[] = {expr->as.binary.left, expr->as.binary.right};
    const char *values[2];
    bool opened = cg_ordered(g, parts, 2, values, out);
    sb_appendf(out, "%ses_contains_%s(%s, %s)%s", expr->as.binary.op == BINARY_NOT_CONTAINS ? "!" : "",
               cg_item_suffix(expr->as.binary.left->type), values[0], values[1], opened ? ")" : "");
}

// --- "every" and "only" ----------------------------------------------------------------

typedef Vec(const char *) Names;

static bool among(const Names *names, const char *name) {
    for (size_t i = 0; i < names->len; i++) {
        if (strcmp(names->items[i], name) == 0) return true;
    }
    return false;
}

static void collect(Codegen *g, const Expr *expr, Names *bound, Codegen *helper);

static void collect_all(Codegen *g, Expr *const *exprs, size_t n, Names *bound, Codegen *helper) {
    for (size_t i = 0; i < n; i++) {
        if (exprs[i]) collect(g, exprs[i], bound, helper);
    }
}

// The variables an expression reads, except the names bound inside it
// ("every" items): they become the helper's parameters.
static void collect(Codegen *g, const Expr *expr, Names *bound, Codegen *helper) {
    switch (expr->kind) {
    case EXPR_NAME: {
        const char *name = expr->as.name;
        if (cg_is_constant(g, name) || cg_find_function(g, name) || among(bound, name)) return;
        for (size_t i = 0; i < helper->captured.len; i++) {
            if (strcmp(helper->captured.items[i].name, name) == 0) return;
        }
        VarCode var = {name, expr->type};
        vec_push(g->arena, &helper->captured, var);
        return;
    }
    case EXPR_CALL: collect_all(g, expr->as.call.args.items, expr->as.call.args.len, bound, helper); return;
    case EXPR_LENGTH:
    case EXPR_FILE_CONTENTS: collect(g, expr->as.operand, bound, helper); return;
    case EXPR_UNARY: collect(g, expr->as.unary.operand, bound, helper); return;
    case EXPR_BINARY: {
        Expr *sides[] = {expr->as.binary.left, expr->as.binary.right};
        collect_all(g, sides, 2, bound, helper);
        return;
    }
    case EXPR_CONVERT: collect(g, expr->as.convert.operand, bound, helper); return;
    case EXPR_LIST: collect_all(g, expr->as.items.items, expr->as.items.len, bound, helper); return;
    case EXPR_ITEM: {
        Expr *parts[] = {expr->as.item.position, expr->as.item.list};
        collect_all(g, parts, 2, bound, helper);
        return;
    }
    case EXPR_LIST_OP: collect(g, expr->as.list_op.operand, bound, helper); return;
    case EXPR_EVERY:
        collect(g, expr->as.every.list, bound, helper);
        vec_push(g->arena, bound, expr->as.every.var.text);
        collect(g, expr->as.every.body, bound, helper);
        bound->len--;
        return;
    case EXPR_ONLY: {
        Expr *parts[] = {expr->as.only.list, expr->as.only.condition};
        collect_all(g, parts, 2, bound, helper);
        return;
    }
    case EXPR_SLICE: {
        Expr *parts[] = {expr->as.slice.from, expr->as.slice.to, expr->as.slice.list};
        collect_all(g, parts, 3, bound, helper);
        return;
    }
    default: return;
    }
}

// Whether working out the expression can call a function (which could change
// the list being gone through).
static bool calls_function(const Codegen *g, const Expr *expr) {
    switch (expr->kind) {
    case EXPR_CALL: return true;
    case EXPR_NAME: return cg_find_function(g, expr->as.name) != NULL;
    case EXPR_LENGTH:
    case EXPR_FILE_CONTENTS: return calls_function(g, expr->as.operand);
    case EXPR_UNARY: return calls_function(g, expr->as.unary.operand);
    case EXPR_BINARY: return calls_function(g, expr->as.binary.left) || calls_function(g, expr->as.binary.right);
    case EXPR_CONVERT: return calls_function(g, expr->as.convert.operand);
    case EXPR_LIST:
        for (size_t i = 0; i < expr->as.items.len; i++) {
            if (calls_function(g, expr->as.items.items[i])) return true;
        }
        return false;
    case EXPR_ITEM:
        return (expr->as.item.position && calls_function(g, expr->as.item.position)) ||
               calls_function(g, expr->as.item.list);
    case EXPR_LIST_OP: return calls_function(g, expr->as.list_op.operand);
    case EXPR_EVERY: return calls_function(g, expr->as.every.list) || calls_function(g, expr->as.every.body);
    case EXPR_ONLY: return calls_function(g, expr->as.only.list) || calls_function(g, expr->as.only.condition);
    case EXPR_SLICE:
        return (expr->as.slice.from && calls_function(g, expr->as.slice.from)) ||
               (expr->as.slice.to && calls_function(g, expr->as.slice.to)) || calls_function(g, expr->as.slice.list);
    default: return false;
    }
}

// One line of the helper: "es_vn_score = es_items->at.n[es_i];" or, for text
// and lists, "es_set(&es_vt_name, es_retain(es_items->at.t[es_i]));".
static void store_item(StrBuf *out, const char *name, Type type, const char *suffix) {
    sb_append(out, "        ");
    if (type_counted(type)) {
        sb_appendf(out, "%s(&", cg_set(type));
        cg_append_var(out, name, type);
        sb_appendf(out, ", %s(es_items->at.%s[es_i]));\n", cg_retain(type), suffix);
    } else {
        cg_append_var(out, name, type);
        sb_appendf(out, " = es_items->at.%s[es_i];\n", suffix);
    }
}

// The C function for one "every" or "only", added to the helper pool; its
// parameters are in helper->captured.
static void emit_helper(Codegen *g, const Expr *expr, Codegen *helper, size_t id) {
    bool every = expr->kind == EXPR_EVERY;
    const Expr *list = every ? expr->as.every.list : expr->as.only.list;
    const Expr *body = every ? expr->as.every.body : expr->as.only.condition;
    Type item_type = type_item(list->type);
    const char *item = every ? expr->as.every.var.text : "0item";  // "only" has no name: "it" is the item
    LoopCode loop = {NULL, 0, 0, item, item_type};
    vec_push(g->arena, &helper->loops, loop);
    VarCode var = {item, item_type};
    vec_push(g->arena, &helper->vars, var);
    const char *value = cg_operand(helper, body, false);

    StrBuf *out = &g->helpers->code;
    sb_appendf(out, "static EsList *es_%s%zu(EsList *es_list", every ? "every" : "only", id);
    for (size_t i = 0; i < helper->captured.len; i++) {
        Type type = helper->captured.items[i].type;
        sb_appendf(out, ", %s%s", cg_ctype(type), type_counted(type) ? "" : " ");
        cg_append_var(out, helper->captured.items[i].name, type);
    }
    sb_append(out, ") {\n");
    cg_emit_declarations(helper, out);
    // A call could change the list while it's being gone through; then go
    // through it as it was.
    sb_appendf(out, "    EsList *es_items = %s;\n", calls_function(g, body) ? "es_list_snapshot(es_list)" : "es_list");
    sb_appendf(out, "    EsList *es_result = es_list_new(%s, %s);\n", items_kind(expr->type), every ? "es_items->len" : "0");
    sb_append(out, "    for (size_t es_i = 0; es_i < es_items->len; es_i++) {\n");
    const char *suffix = cg_item_suffix(list->type);
    store_item(out, item, item_type, suffix);
    if (every) {
        sb_appendf(out, "        es_push_%s(es_result, %s);\n", cg_item_suffix(expr->type), value);
    } else {
        sb_appendf(out, "        if (%s) es_push_%s(es_result, ", value, suffix);
        cg_emit_var(out, item, item_type);
        sb_append(out, ");\n");
    }
    sb_append(out, "    }\n");
    if (type_counted(item_type)) {
        sb_appendf(out, "    %s(&", cg_drop(item_type));
        cg_append_var(out, item, item_type);
        sb_append(out, ");\n");
    }
    sb_append(out, "    es_list_release(es_items);\n    return es_result;\n}\n\n");
}

// es_every1(LIST, captured variables...)
static void emit_helper_call(Codegen *g, const Expr *expr, StrBuf *out) {
    Codegen helper = {0};
    helper.arena = g->arena;
    helper.texts = g->texts;
    helper.helpers = g->helpers;
    helper.sites = g->sites;
    helper.functions = g->functions;
    helper.constants = g->constants;
    Names bound = {0};
    if (expr->kind == EXPR_EVERY) {
        vec_push(g->arena, &bound, expr->as.every.var.text);
        collect(g, expr->as.every.body, &bound, &helper);
    } else {
        collect(g, expr->as.only.condition, &bound, &helper);
    }
    const Expr *list = expr->kind == EXPR_EVERY ? expr->as.every.list : expr->as.only.list;
    const char *list_code = cg_operand(g, list, false);
    size_t id = ++g->helpers->count;
    emit_helper(g, expr, &helper, id);
    sb_appendf(out, "es_%s%zu(%s", expr->kind == EXPR_EVERY ? "every" : "only", id, list_code);
    for (size_t i = 0; i < helper.captured.len; i++) {
        sb_append(out, ", ");
        cg_append_var(out, helper.captured.items[i].name, helper.captured.items[i].type);
    }
    sb_append_char(out, ')');
}

void cg_emit_list_expr(Codegen *g, const Expr *expr, StrBuf *out) {
    switch (expr->kind) {
    case EXPR_LIST: emit_literal(g, expr, out); break;
    case EXPR_EMPTY_LIST: sb_appendf(out, "es_list_new(%s, 0)", items_kind(expr->type)); break;
    case EXPR_ITEM: emit_item(g, expr, out); break;
    case EXPR_LIST_OP: emit_list_op(g, expr, out); break;
    case EXPR_EVERY:
    case EXPR_ONLY: emit_helper_call(g, expr, out); break;
    case EXPR_SLICE: emit_slice(g, expr, out); break;
    default: assert(!"not a list expression"); break;
    }
}

// --- "for each" ---------------------------------------------------------------------

// The loop goes through the list as it was when the loop started, kept in a
// hidden variable ("0each3", which no program can name) so that leaving the
// loop early releases it like any other.
void cg_emit_each_head(Codegen *g, const Stmt *stmt, size_t id, StrBuf *body, size_t depth) {
    Type list = stmt->as.loop.list->type;
    const char *hidden = arena_sprintf(g->arena, "0each%zu", id);
    cg_declare_name(g, hidden, list);
    const char *value = cg_operand(g, stmt->as.loop.list, false);
    StrBuf items;
    sb_init(&items, g->arena);
    cg_append_var(&items, hidden, list);
    sb_append_repeat(body, ' ', 4 * (depth + 1));
    sb_appendf(body, "%s(&%s, es_list_snapshot(%s));\n", cg_set(list), items.data, value);
    sb_append_repeat(body, ' ', 4 * (depth + 1));
    sb_appendf(body, "for (size_t es_i%zu = 0; es_i%zu < %s->len; es_i%zu++) {\n", id, id, items.data, id);
    sb_append_repeat(body, ' ', 4 * (depth + 2));
    Type item = type_item(list);
    const char *name = stmt->as.loop.var.text;
    if (type_counted(item)) {
        sb_appendf(body, "%s(&", cg_set(item));
        cg_append_var(body, name, item);
        sb_appendf(body, ", %s(%s->at.%s[es_i%zu]));\n", cg_retain(item), items.data, cg_item_suffix(list), id);
    } else {
        cg_append_var(body, name, item);
        sb_appendf(body, " = %s->at.%s[es_i%zu];\n", items.data, cg_item_suffix(list), id);
    }
}

// --- Sentences ------------------------------------------------------------------------

// A list sentence as one C expression statement, its parts worked out in the
// order they're written.
const char *cg_list_stmt(Codegen *g, const Stmt *stmt) {
    StrBuf out;
    sb_init(&out, g->arena);
    Type list = stmt->as.list.list->type;
    const char *suffix = cg_item_suffix(list);
    // The site: the position when there is one (errors are about it), else the sentence.
    size_t site = stmt->as.list.op == LIST_SORT || stmt->as.list.op == LIST_ADD_END
                      ? 0
                      : cg_site(g, stmt->as.list.position ? stmt->as.list.position->pos : stmt->pos);
    const char *where = where_code(stmt->as.list.where);
    const char *v[3] = {"0", "0", "0"};
    bool opened = false;
    switch (stmt->as.list.op) {
    case LIST_ADD_END: {  // add V to the end of L
        Expr *parts[] = {stmt->as.list.value, stmt->as.list.list};
        opened = cg_ordered(g, parts, 2, v, &out);
        sb_appendf(&out, "es_add_%s(%s, %s)", suffix, v[1], v[0]);
        break;
    }
    case LIST_INSERT: {  // insert V at position P of L
        Expr *parts[] = {stmt->as.list.value, stmt->as.list.position, stmt->as.list.list};
        opened = cg_ordered(g, parts, 3, v, &out);
        sb_appendf(&out, "es_insert_%s(%zu, %s, %s, %s)", suffix, site, v[2], v[1], v[0]);
        break;
    }
    case LIST_REMOVE: {  // remove item P from L
        Expr *parts[] = {stmt->as.list.position, stmt->as.list.list};
        opened = cg_ordered(g, parts, 2, v, &out);
        sb_appendf(&out, "es_remove(%zu, %s, %s, %s)", site, v[1], where, v[0]);
        break;
    }
    case LIST_SET_ITEM: {  // set item P of L to V
        Expr *parts[] = {stmt->as.list.position, stmt->as.list.list, stmt->as.list.value};
        opened = cg_ordered(g, parts, 3, v, &out);
        sb_appendf(&out, "es_put_%s(%zu, %s, %s, %s, %s)", suffix, site, v[1], where, v[0], v[2]);
        break;
    }
    case LIST_SORT: {
        static const char *const how[] = {"ES_SORT_UP", "ES_SORT_DOWN", "ES_SORT_LENGTH_UP", "ES_SORT_LENGTH_DOWN"};
        sb_appendf(&out, "es_sort(%s, %s)", cg_operand(g, stmt->as.list.list, false),
                   how[stmt->as.list.by_length * 2 + stmt->as.list.descending]);
        break;
    }
    }
    if (opened) sb_append_char(&out, ')');
    return out.data;
}
