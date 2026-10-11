// C code for calling a way of an interface ("speak pet", where pet is
// something that can speak): which method runs depends on the kind of the
// value, so each way that's called gets a C function that looks at the
// value's EsRecordType and calls the method for that kind:
//
//   static void es_way0_speak(int es_call, EsRecord *es_r) {
//       const EsRecordType *es_t = es_r->type;
//       if (es_t == &es_rt0) { es_f_speak_0r0(es_call, es_r); return; }
//       es_f_speak_0r1(es_call, es_r);
//   }
//
// The checker made sure only kinds with the way can be there, so the last
// kind needs no test.

#include <string.h>
#include "back/codegen_internal.h"

static const Way *find_way(const Codegen *g, Type iface, const char *name) {
    const Stmt *def = g->records->interfaces.items[type_iface_index(iface)].definition;
    for (size_t i = 0; i < def->as.iface.ways.len; i++) {
        if (strcmp(def->as.iface.ways.items[i].name.text, name) == 0) return &def->as.iface.ways.items[i];
    }
    return NULL;
}

// A method that gives its kind the way: the same name, inputs and result.
static bool gives_way(const Stmt *fn, const Way *way) {
    if (!fn->as.function.method || strcmp(fn->as.function.name.text, way->name.text) != 0) return false;
    Type result = fn->as.function.result;  // a kind that fits the interface the way gives back counts (the checker said so)
    bool fits = type_is_iface(way->result) && (type_is_record(result) || type_is_sum(result));
    if (fn->as.function.param_types.len != way->params.len + 1 || (result != way->result && !fits)) return false;
    for (size_t i = 0; i < way->params.len; i++) {
        if (fn->as.function.param_types.items[i + 1] != way->declared.items[i]) return false;
    }
    return true;
}

// "es_t == &es_rt0", or for a kind with variants each variant's type.
static void append_test(const Codegen *g, StrBuf *out, Type kind) {
    if (type_is_record(kind)) {
        sb_appendf(out, "es_t == &es_rt%zu", type_record_index(kind));
        return;
    }
    size_t sum = type_sum_index(kind), n = 0;
    for (size_t v = 0; v < g->records->variants.len; v++) {
        if (g->records->variants.items[v].sum != sum) continue;
        sb_appendf(out, "%ses_t == &es_wt%zu", n++ ? " || " : "", v);
    }
    if (n == 0) sb_append(out, "0");
}

static void append_call(StrBuf *out, const Stmt *fn, const Way *way) {
    cg_append_function(out, "es_f_", fn);
    sb_append(out, "(es_call, es_r");
    for (size_t i = 0; i < way->params.len; i++) sb_appendf(out, ", es_a%zu", i);
    sb_append_char(out, ')');
}

static void emit_dispatcher(Codegen *g, const Way *way, const char *c_name) {
    StrBuf *out = &g->helpers->code;
    bool nothing = way->result == TYPE_NOTHING;
    sb_appendf(out, "static %s %s(int es_call, EsRecord *es_r", cg_ctype(way->result), c_name);
    for (size_t i = 0; i < way->params.len; i++) {
        Type type = way->declared.items[i];
        sb_appendf(out, ", %s%ses_a%zu", cg_ctype(type), type_counted(type) ? "" : " ", i);
    }
    sb_append(out, ") {\n    const EsRecordType *es_t = es_r->type;\n    (void)es_t;\n");
    Vec(const Stmt *) methods = {0};
    for (size_t i = 0; i < g->functions.len; i++) {
        if (gives_way(g->functions.items[i], way)) vec_push(g->arena, &methods, g->functions.items[i]);
    }
    for (size_t i = 0; i < methods.len; i++) {
        const Stmt *fn = methods.items[i];
        bool last = i + 1 == methods.len;
        sb_append(out, "    ");
        if (!last) {
            sb_append(out, "if (");
            append_test(g, out, fn->as.function.declared.items[0]);
            sb_append(out, nothing ? ") {\n        " : ") ");
        }
        if (!nothing) sb_append(out, "return ");
        append_call(out, fn, way);
        sb_append(out, nothing && !last ? ";\n        return;\n    }\n" : ";\n");
    }
    if (methods.len == 0) {  // no kind has the way, so no value can get here
        sb_append(out, "    (void)es_call;\n    es_record_release(es_r);\n");
        for (size_t i = 0; i < way->params.len; i++) {
            Type type = way->declared.items[i];
            if (type_counted(type)) sb_appendf(out, "    %s(es_a%zu);\n", cg_release(type), i);
            else sb_appendf(out, "    (void)es_a%zu;\n", i);
        }
        if (!nothing) sb_appendf(out, "    return %s;\n", cg_empty(way->result));
    }
    sb_append(out, "}\n\n");
}

Type cg_dispatch_result(const Codegen *g, const Expr *expr) {
    return find_way(g, type_present(expr->as.call.args.items[0]->type), expr->as.call.name)->result;
}

// es_way0_speak(SITE, PET, ...): the dispatcher, made the first time the way is called.
void cg_emit_dispatch(Codegen *g, const Expr *expr, StrBuf *out) {
    Type iface = type_present(expr->as.call.args.items[0]->type);
    const Way *way = find_way(g, iface, expr->as.call.name);
    StrBuf name;
    sb_init(&name, g->arena);
    sb_appendf(&name, "es_way%zu", type_iface_index(iface));
    cg_append_prefixed(&name, "_", way->name.text);
    bool known = false;
    for (size_t i = 0; i < g->closures->dispatchers.len; i++) known |= strcmp(g->closures->dispatchers.items[i], name.data) == 0;
    if (!known) {
        vec_push(g->arena, &g->closures->dispatchers, name.data);
        emit_dispatcher(g, way, name.data);
    }
    size_t n = expr->as.call.args.len;
    const char **values = arena_alloc(g->arena, (n + 1) * sizeof(char *));
    bool opened = cg_ordered(g, expr->as.call.args.items, n, values, out);
    sb_appendf(out, "%s(%zu", name.data, cg_site(g, expr->pos));
    for (size_t i = 0; i < n; i++) sb_appendf(out, ", %s", values[i]);
    sb_append(out, opened ? "))" : ")");
}
