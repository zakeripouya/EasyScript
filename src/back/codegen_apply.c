// C code for the built-ins that take a function (EXPR_APPLY) and the
// sentences that change a list with them.
//
// Each use becomes a C function in the helper pool, like "every" and "only":
//
//   static EsList *es_map3(EsList *es_list) {           (a "to" function: called directly)
//       EsList *es_items = es_list;
//       EsList *es_result = es_list_new(ES_ITEMS_NUMBER, es_items->len);
//       for (size_t es_i = 0; es_i < es_items->len; es_i++) {
//           es_push_n(es_result, es_f_double(4, es_items->at.n[es_i]));
//       }
//       ...
//
// A function kept as a value is passed in (es_f) and called through its
// closure; it could change the list while it's being gone through, so then
// the helper goes through the list as it was (es_list_snapshot).

#include <string.h>
#include "back/codegen_internal.h"

static const char *const helper_names[] = {"map", "filter", "combine", "sortby"};

static bool is_static(const Expr *f) {
    return f->kind == EXPR_FUNCTION_REF;
}

// Whether calling it could change the list being gone through: only a
// function that uses names from around it could reach the list.
static bool could_change(const Expr *f) {
    return !is_static(f) && !(f->kind == EXPR_LAMBDA && f->as.lambda->as.function.captures.len == 0);
}

// "es_f_double(SITE, ARGS)" or "ES_CODE(es_f, TYPE)(SITE, es_f, ARGS)".
static const char *call(Codegen *g, const Expr *f, size_t site, const char *args) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    if (is_static(f)) {
        cg_append_function(&sb, "es_f_", f->as.ref.target);
        sb_appendf(&sb, "(%zu, %s)", site, args);
    } else {
        sb_appendf(&sb, "ES_CODE(es_f, %s)(%zu, es_f, %s)", cg_code_type(g, f->type), site, args);
    }
    return sb.data;
}

// The item at es_i, owned (functions take their inputs).
static const char *item(Codegen *g, Type list) {
    Type type = type_item(list);
    const char *at = arena_sprintf(g->arena, "es_items->at.%s[es_i]", cg_item_suffix(list));
    return type_counted(type) ? arena_sprintf(g->arena, "%s(%s)", cg_retain(type), at) : at;
}

static void emit_helper(Codegen *g, const Expr *expr, size_t id) {
    const Expr *f = expr->as.apply.function;
    Type list = expr->as.apply.list->type;
    ApplyKind kind = expr->as.apply.kind;
    size_t site = cg_site(g, expr->pos);
    Type result = kind == APPLY_COMBINE ? expr->type : TYPE_UNKNOWN;
    StrBuf *out = &g->helpers->code;
    sb_appendf(out, "static %s es_%s%zu(EsList *es_list", kind == APPLY_COMBINE ? cg_ctype(result) : "EsList *",
               helper_names[kind], id);
    if (!is_static(f)) sb_append(out, ", EsRecord *es_f");
    if (expr->as.apply.start) sb_appendf(out, ", %s%ses_acc", cg_ctype(result), type_counted(result) ? "" : " ");
    sb_append(out, ") {\n");
    // A copy for sorting, which only this code can reach; a snapshot when a closure could change the list.
    const char *items = kind == APPLY_SORT ? "es_list_copy(es_list)" : could_change(f) ? "es_list_snapshot(es_list)" : "es_list";
    sb_appendf(out, "    EsList *es_items = %s;\n", items);
    const char *one = item(g, list);
    switch (kind) {
    case APPLY_MAP:
        sb_appendf(out, "    EsList *es_result = es_list_new(%s, es_items->len);\n", cg_items(type_item(expr->type)));
        sb_append(out, "    for (size_t es_i = 0; es_i < es_items->len; es_i++) {\n");
        sb_appendf(out, "        es_push_%s(es_result, %s);\n    }\n", cg_item_suffix(expr->type), call(g, f, site, one));
        break;
    case APPLY_FILTER:
        sb_appendf(out, "    EsList *es_result = es_list_new(%s, 0);\n", cg_items(type_item(list)));
        sb_append(out, "    for (size_t es_i = 0; es_i < es_items->len; es_i++) {\n");
        sb_appendf(out, "        if (%s) es_push_%s(es_result, %s);\n    }\n", call(g, f, site, one), cg_item_suffix(list),
                   one);
        sb_append(out, "    es_list_shrink(es_result); /* exactly the items kept */\n");
        break;
    case APPLY_COMBINE:
        sb_append(out, "    size_t es_i = 0;\n");
        if (!expr->as.apply.start) {
            sb_appendf(out, "    if (es_items->len == 0) es_fail(%zu, \"Say what to start from, like \\\", starting "
                            "from 0\\\".\", \"There's nothing to combine: the list is empty.\");\n", site);
            sb_appendf(out, "    %s%ses_acc = %s;\n    es_i = 1;\n", cg_ctype(result), type_counted(result) ? "" : " ",
                       one);
        }
        sb_append(out, "    for (; es_i < es_items->len; es_i++) {\n");
        sb_appendf(out, "        es_acc = %s;\n    }\n", call(g, f, site, arena_sprintf(g->arena, "es_acc, %s", one)));
        break;
    case APPLY_SORT: {
        Type key = ast_func_kind(g->records, f->type)->result;
        sb_appendf(out, "    EsList *es_keys = es_list_new(%s, es_items->len);\n", cg_items(key));
        sb_append(out, "    for (size_t es_i = 0; es_i < es_items->len; es_i++) {\n");
        sb_appendf(out, "        es_push_%s(es_keys, %s);\n    }\n", cg_value_suffix(key), call(g, f, site, one));
        sb_appendf(out, "    es_sort_by_keys(es_items, es_keys, %s);\n", expr->as.apply.descending ? "true" : "false");
        break;
    }
    }
    if (kind != APPLY_SORT) sb_append(out, "    es_list_release(es_items);\n");
    if (!is_static(f)) sb_append(out, "    es_record_release(es_f);\n");
    sb_appendf(out, "    return %s;\n}\n\n", kind == APPLY_COMBINE ? "es_acc" : kind == APPLY_SORT ? "es_items" : "es_result");
}

// es_map3(LIST, F, START): the parts worked out in the order they're
// written (the function first in "F applied to every X in L").
void cg_emit_apply(Codegen *g, const Expr *expr, StrBuf *out) {
    static const char *const plans[] = {
        "\"applied to every\": 1 pass, making 1 new list of the same length.",
        "\"that pass\": 1 pass, making 1 new list of the items kept.",
        "\"combined using\": 1 pass, no list made.",
        "\"sorted by\": 1 pass working out each item's key, then a stable sort of a copy.",
    };
    cg_plan(g, expr->pos, "%s", plans[expr->as.apply.kind]);
    size_t id = ++g->helpers->count;
    emit_helper(g, expr, id);
    const Expr *f = expr->as.apply.function;
    bool function_first = expr->as.apply.kind == APPLY_MAP && f->pos.span.offset < expr->as.apply.list->pos.span.offset;
    Expr *parts[3] = {(Expr *)expr->as.apply.list, is_static(f) ? NULL : (Expr *)f, expr->as.apply.start};
    if (function_first) {
        parts[0] = parts[1];
        parts[1] = (Expr *)expr->as.apply.list;
    }
    const char *values[3] = {NULL, NULL, NULL};
    bool opened = cg_ordered(g, parts, 3, values, out);
    const char *list = function_first ? values[1] : values[0];
    if (expr->as.apply.list->kind == EXPR_NAME && !opened) list = cg_list_operand(g, expr->as.apply.list);
    sb_appendf(out, "es_%s%zu(%s", helper_names[expr->as.apply.kind], id, list);
    if (!is_static(f)) sb_appendf(out, ", %s", function_first ? values[0] : values[1]);
    if (expr->as.apply.start) sb_appendf(out, ", %s", values[2]);
    sb_append(out, opened ? "))" : ")");
}

// apply F to every X in L / keep only ... / sort L by F: L goes into its
// hidden name, then its items are replaced by the new ones worked out from it.
const char *cg_replace_stmt(Codegen *g, const Stmt *stmt) {
    const char *hidden = stmt->as.list.hidden->as.name;
    Type list = stmt->as.list.list->type;
    cg_declare_name(g, hidden, list);
    StrBuf var;
    sb_init(&var, g->arena);
    cg_append_var(g, &var, hidden, list);
    const char *value = cg_operand(g, stmt->as.list.value, false);
    return arena_sprintf(g->arena, "es_list_set(&%s, %s), es_list_replace(es_list_retain(%s), %s)", var.data,
                         cg_operand(g, stmt->as.list.list, false), var.data, value);
}
