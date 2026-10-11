// C code for functions as values (runtime/es_closure.h): closures, the
// cells of the names they share with the code around them, and calls
// through a function value.
//
// A function made inside a value becomes a C function like any other
// (es_f_0l1, taking the closure as es_self) plus, when it uses names from
// around it, a struct for its closure holding their cells:
//
//   typedef struct { EsClosure base; EsRecord *c0; } es_L0l1;
//   static const EsRecordType es_lt0l1 = {"the function made on line 3", 1, ...};
//   static EsRecord *es_mk0l1(EsRecord *c0) { ... }   (makes the closure)
//
// One that uses nothing from around it is a static immortal closure
// (es_lv0l1), so making it costs nothing. A function defined with "to" and
// used as a value gets a wrapper with the closure's signature (es_w_double)
// and a static closure (es_fv_double).
//
// A name such a function uses is kept in a cell (es_cvn_count) by the code
// that makes it, and read as ES_CELL(es_cvn_count, double) everywhere.

#include <string.h>
#include "back/codegen_internal.h"

// --- Cells ----------------------------------------------------------------------------

static bool among(const char *const *names, size_t len, const char *name) {
    for (size_t i = 0; i < len; i++) {
        if (strcmp(names[i], name) == 0) return true;
    }
    return false;
}

bool cg_is_cell(const Codegen *g, const char *name) {
    return among(g->cells.items, g->cells.len, name) || among(g->borrowed.items, g->borrowed.len, name);
}

void cg_append_cell(StrBuf *out, const char *name, Type type) {
    StrBuf raw;
    sb_init(&raw, out->arena);
    cg_append_raw_var(&raw, name, type);
    sb_appendf(out, "es_c%s", raw.data + 3);  // es_vn_count -> es_cvn_count
}

void cg_append_var(const Codegen *g, StrBuf *out, const char *name, Type type) {
    if (!cg_is_cell(g, name)) {
        cg_append_raw_var(out, name, type);
        return;
    }
    sb_append(out, "ES_CELL(");
    cg_append_cell(out, name, type);
    sb_appendf(out, ", %s)", cg_ctype(type));
}

void cg_append_assign(const Codegen *g, StrBuf *out, const char *name, Type type, const char *value, bool make) {
    if (make && cg_is_cell(g, name)) {
        sb_append(out, "es_record_set(&");
        cg_append_cell(out, name, type);
        sb_appendf(out, ", es_cell_%s(%s));\n", cg_value_suffix(type), value);
    } else if (type_counted(type)) {
        sb_appendf(out, "%s(&", cg_set(type));
        cg_append_var(g, out, name, type);
        sb_appendf(out, ", %s);\n", value);
    } else {
        cg_append_var(g, out, name, type);
        sb_appendf(out, " = %s;\n", value);
    }
}

void cg_emit_new_cell(Codegen *g, const char *name, Type type) {
    if (!cg_is_cell(g, name)) return;
    StrBuf cell;
    sb_init(&cell, g->arena);
    cg_append_cell(&cell, name, type);
    cg_indent(g);
    sb_appendf(&g->body, "if (!%s) %s = es_cell_%s(%s);\n", cell.data, cell.data, cg_value_suffix(type), cg_empty(type));
}

// --- Kinds ------------------------------------------------------------------------------

const char *cg_code_type(const Codegen *g, Type func) {
    const FuncKind *kind = ast_func_kind(g->records, func);
    StrBuf sb;
    sb_init(&sb, g->arena);
    sb_appendf(&sb, "%s (*)(int, EsRecord *", cg_ctype(kind->result));
    for (size_t i = 0; i < kind->count; i++) sb_appendf(&sb, ", %s", cg_ctype(kind->params[i]));
    sb_append_char(&sb, ')');
    return sb.data;
}

// "int es_call, EsRecord *es_self, double es_vn_x": a closure's C parameters.
static void append_parameters(StrBuf *out, const Stmt *fn) {
    sb_append(out, "(int es_call, EsRecord *es_self");
    for (size_t i = 0; i < fn->as.function.params.len; i++) {
        Type type = fn->as.function.param_types.items[i];
        sb_appendf(out, ", %s%s", cg_ctype(type), type_counted(type) ? "" : " ");
        cg_append_raw_var(out, fn->as.function.params.items[i].text, type);
    }
    sb_append_char(out, ')');
}

// --- The function double -------------------------------------------------------------------

void cg_emit_function_ref(Codegen *g, const Expr *expr, StrBuf *out) {
    const Stmt *fn = expr->as.ref.target;
    bool known = false;
    for (size_t i = 0; i < g->closures->refs.len; i++) known |= g->closures->refs.items[i] == fn;
    if (!known) vec_push(g->arena, &g->closures->refs, fn);
    cg_append_function(out, "((EsRecord *)&es_fv_", fn);  // immortal: never counted
    sb_append_char(out, ')');
}

// static double es_w_double(int es_call, EsRecord *es_self, double es_vn_x) {
//     (void)es_self;
//     return es_f_double(es_call, es_vn_x);
// }
// static const EsRecordType es_ft_double = {"the function double", 0, NULL, "", NULL, sizeof(EsClosure), ES_CLOSURE};
// static EsClosure es_fv_double = {{{ES_OBJ_RECORD, ES_IMMORTAL}, &es_ft_double, NULL, NULL, 0}, (EsCode)es_w_double};
void cg_emit_function_refs(Codegen *g, StrBuf *out) {
    for (size_t r = 0; r < g->closures->refs.len; r++) {
        const Stmt *fn = g->closures->refs.items[r];
        sb_appendf(out, "static %s ", cg_ctype(fn->as.function.result));
        cg_append_function(out, "es_w_", fn);
        append_parameters(out, fn);
        sb_appendf(out, " {\n    (void)es_self;\n    %s", fn->as.function.result == TYPE_NOTHING ? "" : "return ");
        cg_append_function(out, "es_f_", fn);
        sb_append(out, "(es_call");
        for (size_t i = 0; i < fn->as.function.params.len; i++) {
            sb_append(out, ", ");
            cg_append_raw_var(out, fn->as.function.params.items[i].text, fn->as.function.param_types.items[i]);
        }
        sb_append(out, ");\n}\n");
        cg_append_function(out, "static const EsRecordType es_ft_", fn);
        sb_append(out, " = {");
        const char *name = arena_sprintf(g->arena, "the function %s", fn->as.function.name.display);
        cg_append_c_string(out, name, strlen(name));
        sb_append(out, ", 0, NULL, \"\", NULL, sizeof(EsClosure), ES_CLOSURE};\n");
        cg_append_function(out, "static EsClosure es_fv_", fn);
        cg_append_function(out, " = {{{ES_OBJ_RECORD, ES_IMMORTAL}, &es_ft_", fn);
        cg_append_function(out, ", NULL, NULL, 0}, (EsCode)es_w_", fn);
        sb_append(out, "};\n");
    }
}

// --- A function taking x ... ------------------------------------------------------------------

// Its closure's struct, description and maker (or static closure), and the
// prototype of its C function, before every function that could make it.
static void emit_closure_head(Codegen *g, const Stmt *fn) {
    StrBuf *out = &g->closures->head;
    size_t n = fn->as.function.captures.len;
    sb_appendf(out, "static %s ", cg_ctype(fn->as.function.result));
    cg_append_function(out, "es_f_", fn);
    append_parameters(out, fn);
    sb_append(out, ";\n");
    const char *name = fn->as.function.name.display;
    if (n == 0) {
        cg_append_function(out, "static const EsRecordType es_lt", fn);
        sb_append(out, " = {");
        cg_append_c_string(out, name, strlen(name));
        sb_append(out, ", 0, NULL, \"\", NULL, sizeof(EsClosure), ES_CLOSURE};\n");
        cg_append_function(out, "static EsClosure es_lv", fn);
        cg_append_function(out, " = {{{ES_OBJ_RECORD, ES_IMMORTAL}, &es_lt", fn);
        cg_append_function(out, ", NULL, NULL, 0}, (EsCode)es_f_", fn);
        sb_append(out, "};\n");
        return;
    }
    sb_append(out, "typedef struct {\n    EsClosure base;\n");
    for (size_t i = 0; i < n; i++) sb_appendf(out, "    EsRecord *c%zu;\n", i);
    cg_append_function(out, "} es_L", fn);
    cg_append_function(out, ";\nstatic const char *const es_ln", fn);
    sb_append(out, "[] = {");
    for (size_t i = 0; i < n; i++) {
        sb_append(out, i ? ", " : "");
        cg_append_c_string(out, fn->as.function.captures.items[i].name, strlen(fn->as.function.captures.items[i].name));
    }
    cg_append_function(out, "};\nstatic const size_t es_lo", fn);
    sb_append(out, "[] = {");
    for (size_t i = 0; i < n; i++) {
        cg_append_function(out, i ? ", offsetof(es_L" : "offsetof(es_L", fn);
        sb_appendf(out, ", c%zu)", i);
    }
    cg_append_function(out, "};\nstatic const EsRecordType es_lt", fn);
    sb_append(out, " = {");
    cg_append_c_string(out, name, strlen(name));
    sb_appendf(out, ", %zu, ", n);
    cg_append_function(out, "es_ln", fn);
    sb_append(out, ", \"");
    for (size_t i = 0; i < n; i++) sb_append_char(out, 'r');
    cg_append_function(out, "\", es_lo", fn);
    cg_append_function(out, ", sizeof(es_L", fn);
    cg_append_function(out, "), ES_CLOSURE};\nstatic EsRecord *es_mk", fn);
    sb_append_char(out, '(');
    for (size_t i = 0; i < n; i++) sb_appendf(out, "%sEsRecord *c%zu", i ? ", " : "", i);
    cg_append_function(out, ") {\n    EsRecord *r = es_closure_new(&es_lt", fn);
    cg_append_function(out, ", (EsCode)es_f_", fn);
    sb_append(out, ");\n");
    for (size_t i = 0; i < n; i++) {
        cg_append_function(out, "    ((es_L", fn);
        sb_appendf(out, " *)r)->c%zu = c%zu;\n", i, i);
    }
    sb_append(out, "    return r;\n}\n");
}

// (EsRecord *)&es_lv0l1, or es_mk0l2(es_record_retain(es_cvn_count), ...)
void cg_emit_lambda(Codegen *g, const Expr *expr, StrBuf *out) {
    const Stmt *fn = expr->as.lambda;
    emit_closure_head(g, fn);
    cg_emit_function(g, fn, &g->closures->code);
    size_t n = fn->as.function.captures.len;
    if (n == 0) {
        cg_append_function(out, "((EsRecord *)&es_lv", fn);
        sb_append_char(out, ')');
        return;
    }
    cg_append_function(out, "es_mk", fn);
    sb_append_char(out, '(');
    for (size_t i = 0; i < n; i++) {
        const Capture *capture = &fn->as.function.captures.items[i];
        sb_append(out, i ? ", es_record_retain(" : "es_record_retain(");
        cg_append_cell(out, capture->name, capture->type);
        sb_append_char(out, ')');
    }
    sb_append_char(out, ')');
}

// --- Calling a function value ----------------------------------------------------------------

// A variable of this function that nothing else can change during the call
// (it isn't in a cell): the call can use it without holding it.
static bool borrowable(const Codegen *g, const Expr *callee) {
    return callee->kind == EXPR_NAME && !cg_is_cell(g, callee->as.name);
}

// ES_CODE(F, TYPE)(SITE, F, ARGS...): F is held (in a temporary) while it runs,
// unless it's a variable nothing else can change, and released afterwards.
void cg_emit_value_call(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *callee = expr->as.call.callee;
    Type result = ast_func_kind(g->records, callee->type)->result;
    const char *code_type = cg_code_type(g, callee->type);
    size_t site = cg_site(g, expr->pos);
    const char *f;
    size_t held = 0, kept = 0;
    if (borrowable(g, callee)) {
        StrBuf var;
        sb_init(&var, g->arena);
        cg_append_var(g, &var, callee->as.name, callee->type);
        f = var.data;
    } else {
        held = cg_new_temp(g, callee->type);
        sb_appendf(out, "(es_t%zu = %s, ", held, cg_operand(g, callee, false));
        f = arena_sprintf(g->arena, "es_t%zu", held);
        if (result != TYPE_NOTHING) kept = cg_new_temp(g, result);
    }
    size_t n = expr->as.call.args.len;
    const char **values = arena_alloc(g->arena, (n + 1) * sizeof(char *));
    bool opened = cg_ordered(g, expr->as.call.args.items, n, values, out);
    if (kept) sb_appendf(out, "es_t%zu = ", kept);
    sb_appendf(out, "ES_CODE(%s, %s)(%zu, %s", f, code_type, site, f);
    for (size_t i = 0; i < n; i++) sb_appendf(out, ", %s", values[i]);
    sb_append(out, opened ? "))" : ")");
    if (!held) return;
    sb_appendf(out, ", es_record_release(es_t%zu)", held);
    if (kept) sb_appendf(out, ", es_t%zu", kept);
    sb_append_char(out, ')');
}
