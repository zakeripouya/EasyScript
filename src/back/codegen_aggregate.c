// Questions about a list (EXPR_AGGREGATE) each become one C function
// (es_aggN) that goes through the list once: how many, any, every, no, the
// total, average and median, the top N, the most common, the one with the
// highest, and "per" (a map from each group to its result). A filter the list
// was written with ("the orders whose amount is over 100") is tested in the
// same pass, so no list of the kept items is made, unless its test calls a
// definition (then the filter keeps its own pass, so calls happen in the same
// order as written).

#include <string.h>
#include "back/codegen_internal.h"

static const char *const item_name = "0item";  // the item ("it"), a name no program can make

typedef struct {
    const Expr *expr;
    AggregateKind kind;
    const Expr *list;         // what's gone through, inside the filters taken into the pass
    Vec(const Expr *) tests;  // those filters' tests, the outermost first
    Type item;                // the item's kind
    const char *suffix;       // the runtime's suffix for the items: n, t, ...
    size_t site;
} Question;

// The filters around the list that can be tested in the same pass.
static void find_list(Codegen *g, Question *q) {
    const Expr *list = q->expr->as.aggregate.list;
    while (list->kind == EXPR_ONLY && !cg_calls_function(g, list->as.only.condition)) {
        vec_push(g->arena, &q->tests, list->as.only.condition);
        list = list->as.only.list;
    }
    q->list = list;
    q->item = type_item(list->type);
    q->suffix = cg_item_suffix(list->type);
}

static const char *item_read(Codegen *g, const Question *q, bool owned) {
    const char *read = arena_sprintf(g->arena, "es_items->at.%s[es_i]", q->suffix);
    return owned && type_counted(q->item) ? arena_sprintf(g->arena, "%s(%s)", cg_retain(q->item), read) : read;
}

// The key worked out for each item, as a number: its field, the item itself,
// or how many items a list has ("with the most orders").
static const char *key_code(Codegen *helper, const Question *q) {
    const Expr *key = q->expr->as.aggregate.key;
    if (!key) return item_read(helper, q, false);
    if (type_is_list(key->type)) return arena_sprintf(helper->arena, "es_list_length(%s)", cg_list_operand(helper, key));
    return cg_operand(helper, key, false);
}

// The group's key as the map takes it: (EsValue){.t = ...} or {.n = ...}.
static const char *group_code(Codegen *helper, const Question *q) {
    const Expr *group = q->expr->as.aggregate.group;
    return arena_sprintf(helper->arena, "(EsValue){.%c = %s}", group->type == TYPE_TEXT ? 't' : 'n',
                         cg_operand(helper, group, false));
}

static const char *result_ctype(const Question *q) {
    if (q->expr->as.aggregate.group) return "EsMap *";
    return cg_ctype(q->expr->type);
}

// What's set up before the items are gone through.
static void emit_start(Codegen *g, const Question *q, StrBuf *out) {
    const Expr *group = q->expr->as.aggregate.group;
    bool lowest = q->expr->as.aggregate.lowest;
    if (group) {
        const char *keys = cg_items(group->type);
        const char *values = q->kind == AGG_MEDIAN ? "ES_ITEMS_LIST" : "ES_ITEMS_NUMBER";
        sb_appendf(out, "    EsMap *es_result = es_map_new(%s, %s, 0);\n", keys, values);
        if (q->kind == AGG_AVERAGE) sb_appendf(out, "    EsMap *es_counts = es_map_new(%s, ES_ITEMS_NUMBER, 0);\n", keys);
        return;
    }
    switch (q->kind) {
    case AGG_COUNT:
    case AGG_TOTAL: sb_append(out, "    double es_result = 0;\n"); break;
    case AGG_ANY: sb_append(out, "    bool es_result = false;\n"); break;
    case AGG_ALL:
    case AGG_NONE: sb_append(out, "    bool es_result = true;\n"); break;
    case AGG_AVERAGE: sb_append(out, "    double es_total = 0, es_count = 0;\n"); break;
    case AGG_MEDIAN: sb_append(out, "    EsList *es_keys = es_list_new(ES_ITEMS_NUMBER, 0);\n"); break;
    case AGG_TOP: break;  // es_top, made before the list is looked at
    case AGG_MOST_COMMON:
        sb_appendf(out, "    EsMap *es_counts = es_map_new(%s, ES_ITEMS_NUMBER, 0);\n", cg_items(q->item));
        break;
    case AGG_BEST: sb_append(out, "    size_t es_best = SIZE_MAX;\n    double es_key = 0;\n"); break;
    }
    (void)g;
    (void)lowest;
}

// What's done with each item that passes the filters.
static void emit_step(Codegen *helper, const Question *q, StrBuf *out) {
    const char *test = q->expr->as.aggregate.condition ? cg_operand(helper, q->expr->as.aggregate.condition, false) : NULL;
    if (q->expr->as.aggregate.group) {
        const char *key = q->kind == AGG_COUNT ? "1" : key_code(helper, q);
        const char *group = group_code(helper, q);
        sb_appendf(out, "        double es_k = %s;\n", key);
        if (q->kind == AGG_MEDIAN) {
            sb_appendf(out, "        bool es_added;\n        EsValue *es_v = es_map_slot(es_result, %s, &es_added);\n", group);
            sb_append(out, "        if (es_added) es_v->l = es_list_new(ES_ITEMS_NUMBER, 0);\n");
            sb_append(out, "        es_push_n(es_v->l, es_k);\n");
            return;
        }
        sb_appendf(out, "        es_map_slot(es_result, %s, NULL)->n += es_k;\n", group);
        if (q->kind == AGG_AVERAGE) {
            sb_appendf(out, "        es_map_slot(es_counts, %s, NULL)->n += 1;\n", group_code(helper, q));
        }
        return;
    }
    switch (q->kind) {
    case AGG_COUNT:
        if (test) sb_appendf(out, "        if (%s) es_result += 1;\n", test);
        else sb_append(out, "        es_result += 1;\n");
        break;
    case AGG_ANY: sb_appendf(out, "        if (%s) {\n            es_result = true;\n            break;\n        }\n", test); break;
    case AGG_ALL: sb_appendf(out, "        if (!(%s)) {\n            es_result = false;\n            break;\n        }\n", test); break;
    case AGG_NONE: sb_appendf(out, "        if (%s) {\n            es_result = false;\n            break;\n        }\n", test); break;
    case AGG_TOTAL: sb_appendf(out, "        es_result += %s;\n", key_code(helper, q)); break;
    case AGG_AVERAGE: sb_appendf(out, "        es_total += %s;\n        es_count += 1;\n", key_code(helper, q)); break;
    case AGG_MEDIAN: sb_appendf(out, "        es_push_n(es_keys, %s);\n", key_code(helper, q)); break;
    case AGG_TOP: sb_appendf(out, "        es_top_offer(&es_top, %s, es_i);\n", key_code(helper, q)); break;
    case AGG_MOST_COMMON:
        sb_appendf(out, "        es_map_slot(es_counts, (EsValue){.%c = %s}, NULL)->n += 1;\n",
                   q->item == TYPE_TEXT ? 't' : 'n', item_read(helper, q, true));
        break;
    case AGG_BEST:
        sb_appendf(out, "        double es_k = %s;\n", key_code(helper, q));
        sb_appendf(out, "        if (es_best == SIZE_MAX || es_k %s es_key || (isnan(es_key) && !isnan(es_k))) {\n",
                   q->expr->as.aggregate.lowest ? "<" : ">");
        sb_append(out, "            es_best = es_i;\n            es_key = es_k;\n        }\n");
        break;
    }
}

// What's given back once every item is gone through (es_items is still held).
static void emit_finish(Codegen *g, const Question *q, StrBuf *out) {
    const char *release = "    es_list_release(es_items);\n";
    if (q->expr->as.aggregate.group) {
        sb_append(out, release);
        if (q->kind == AGG_AVERAGE) sb_append(out, "    return es_map_divide(es_result, es_counts);\n");
        else if (q->kind == AGG_MEDIAN) sb_appendf(out, "    return es_map_medians(%zu, es_result);\n", q->site);
        else sb_append(out, "    return es_result;\n");
        return;
    }
    switch (q->kind) {
    case AGG_AVERAGE: sb_appendf(out, "%s    return es_average(%zu, es_total, es_count);\n", release, q->site); break;
    case AGG_MEDIAN: sb_appendf(out, "%s    return es_median(%zu, es_keys);\n", release, q->site); break;
    case AGG_TOP: sb_appendf(out, "    EsList *es_result = es_top_take(&es_top, es_items);\n%s    return es_result;\n", release); break;
    case AGG_MOST_COMMON: sb_appendf(out, "%s    return es_most_common(&es_top, es_counts);\n", release); break;
    case AGG_BEST: {
        sb_appendf(out, "    if (es_best == SIZE_MAX) {\n    %s        es_no_choice(%zu);\n    }\n", release, q->site);
        const char *read = arena_sprintf(g->arena, "es_items->at.%s[es_best]", q->suffix);
        if (type_counted(q->item)) read = arena_sprintf(g->arena, "%s(%s)", cg_retain(q->item), read);
        sb_appendf(out, "    %s%ses_result = %s;\n%s    return es_result;\n", cg_ctype(q->item),
                   type_counted(q->item) ? "" : " ", read, release);
        break;
    }
    default: sb_appendf(out, "%s    return es_result;\n", release); break;
    }
}

static void plan(Codegen *g, const Question *q, bool snapshot) {
    static const char *const names[] = {"how many", "any", "every", "no", "the total", "the average", "the median",
                                        "the top", "the most common", "with the highest"};
    static const char *const how[] = {
        "counted in 1 pass, no list made",
        "1 pass, stopping at the first item that passes",
        "1 pass, stopping at the first item that fails",
        "1 pass, stopping at the first item that passes",
        "added up in 1 pass, no list made",
        "added up and counted in 1 pass, no list made",
        "1 pass keeping the numbers in 1 list, sorted to find the middle",
        "1 pass keeping the best ones in a heap, no sort of the whole list",
        "1 pass counting in a hash map, then the most common kept in a heap",
        "1 pass keeping the best so far, no list made",
    };
    static const char *const grouped[] = {
        "counted per group in 1 pass, in a hash map", NULL, NULL, NULL,
        "added up per group in 1 pass, in a hash map",
        "added up and counted per group in 1 pass, in 2 hash maps",
        "1 pass keeping each group's numbers in a list, each sorted to find its middle",
    };
    const char *name = names[q->kind];
    if (q->kind == AGG_TOP && q->expr->as.aggregate.lowest) name = "the lowest";
    if (q->kind == AGG_BEST && q->expr->as.aggregate.lowest) name = "with the lowest";
    StrBuf line;
    sb_init(&line, g->arena);
    bool group = q->expr->as.aggregate.group != NULL;
    if (group && q->kind == AGG_COUNT) name = "the number of";
    sb_appendf(&line, "\"%s%s\": %s", name, group ? " ... per" : "", group ? grouped[q->kind] : how[q->kind]);
    if (q->tests.len) sb_appendf(&line, ", %zu filter%s tested in the same pass", q->tests.len, q->tests.len > 1 ? "s" : "");
    if (snapshot) sb_append(&line, " (through a copy of the list, since it calls a definition)");
    cg_plan(g, q->expr->pos, "%s.", line.data);
}

// The C function for one question, added to the helper pool.
static void emit_helper(Codegen *g, Question *q, Codegen *helper, size_t id) {
    const Expr *agg = q->expr;
    LoopCode loop = {NULL, 0, 0, item_name, q->item};
    vec_push(g->arena, &helper->loops, loop);
    VarCode var = {item_name, q->item};
    vec_push(g->arena, &helper->vars, var);
    StrBuf step;  // worked out first: it decides the temporaries the helper declares
    sb_init(&step, g->arena);
    for (size_t i = q->tests.len; i-- > 0;) {  // the innermost filter first, as written
        sb_appendf(&step, "        if (!(%s)) continue;\n", cg_operand(helper, q->tests.items[i], false));
    }
    emit_step(helper, q, &step);
    bool calls = (agg->as.aggregate.condition && cg_calls_function(g, agg->as.aggregate.condition)) ||
                 (agg->as.aggregate.key && cg_calls_function(g, agg->as.aggregate.key)) ||
                 (agg->as.aggregate.group && cg_calls_function(g, agg->as.aggregate.group));
    plan(g, q, calls);

    StrBuf *out = &g->helpers->code;
    sb_appendf(out, "static %s%ses_agg%zu(EsList *es_list", result_ctype(q), strchr(result_ctype(q), '*') ? "" : " ", id);
    if (agg->as.aggregate.count) sb_append(out, ", double es_take");
    cg_helper_params(out, helper);
    sb_append(out, ") {\n");
    cg_emit_declarations(helper, out);
    if (q->kind == AGG_TOP || q->kind == AGG_MOST_COMMON) {  // how many to take is checked first
        sb_appendf(out, "    EsTop es_top = es_top_start(%zu, es_take, es_list->len, %s);\n", q->site,
                   agg->as.aggregate.lowest ? "true" : "false");
    }
    // A call could change the list while it's being gone through; then go through it as it was.
    sb_appendf(out, "    EsList *es_items = %s;\n", calls ? "es_list_snapshot(es_list)" : "es_list");
    emit_start(g, q, out);
    sb_append(out, "    for (size_t es_i = 0; es_i < es_items->len; es_i++) {\n");
    cg_store_item(out, item_name, q->item, q->suffix);
    sb_append_n(out, step.data, step.len);
    sb_append(out, "    }\n");
    cg_drop_helper_names(helper, out);
    emit_finish(g, q, out);
    sb_append(out, "}\n\n");
}

void cg_emit_aggregate(Codegen *g, const Expr *expr, StrBuf *out) {
    Question q = {expr, expr->as.aggregate.kind, NULL, {0}, TYPE_UNKNOWN, NULL, cg_site(g, expr->pos)};
    find_list(g, &q);
    Codegen helper;
    cg_helper_init(g, &helper);
    for (size_t i = 0; i < q.tests.len; i++) cg_helper_collect(g, q.tests.items[i], &helper);
    cg_helper_collect(g, expr->as.aggregate.condition, &helper);
    cg_helper_collect(g, expr->as.aggregate.key, &helper);
    cg_helper_collect(g, expr->as.aggregate.group, &helper);
    const Expr *count = expr->as.aggregate.count;
    const char *count_code = count ? cg_operand(g, count, false) : NULL;
    const char *list_code = cg_list_operand(g, q.list);
    const char *close = "";
    if (count && cg_has_effects(g, count) && cg_has_effects(g, q.list)) {  // how many to take is written first
        size_t t = cg_new_temp(g, TYPE_NUMBER);
        sb_appendf(out, "(es_t%zu = %s, ", t, count_code);
        count_code = arena_sprintf(g->arena, "es_t%zu", t);
        close = ")";
    }
    size_t id = ++g->helpers->count;
    emit_helper(g, &q, &helper, id);
    sb_appendf(out, "es_agg%zu(%s", id, list_code);
    if (count_code) sb_appendf(out, ", %s", count_code);
    cg_helper_args(g, out, &helper);
    sb_appendf(out, ")%s", close);
}
