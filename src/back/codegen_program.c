// C code generation for the whole program: constants, functions (their
// descriptions for the call stack, frames and bodies), and main(). The
// statements themselves are in codegen_c.c.

#include <math.h>
#include <string.h>
#include "back/codegen_internal.h"

// Generated from the runtime/ headers at build time by tools/embed.c.
extern const unsigned char es_runtime_source[];
extern const size_t es_runtime_source_len;

// static const double es_k_rate = 0.20000000000000001;
// static EsText *const es_k_greeting = &es_s3;  (immortal)
// A worked-out number as C: "0.20000000000000001", "NAN", "-HUGE_VAL".
void cg_append_number(StrBuf *out, double x) {
    if (isnan(x)) {
        sb_append(out, "NAN");
    } else if (isinf(x)) {
        sb_appendf(out, "%sHUGE_VAL", x < 0 ? "-" : "");
    } else {
        sb_appendf(out, "%.17g", x);
    }
}

// A fixed list: its items, then a static list marked immortal, so it's never
// counted, freed, or changed (the checker rejects changes, and codegen copies
// it wherever it could be kept and changed; see cg_list_operand):
//   static EsText *es_ki_days[] = {&es_s1, &es_s2};
//   static EsList es_k_days = {{ES_OBJ_LIST, ES_IMMORTAL}, ES_ITEMS_TEXT, 2, 2, {.t = es_ki_days}};
void cg_emit_fixed_list(Codegen *g, const char *name, const ConstValue *v, StrBuf *out) {
    Type item = type_item(v->list_type);
    const char *field = item == TYPE_TEXT ? "t" : item == TYPE_YESNO ? "b" : "n";
    if (v->len > 0) {
        sb_appendf(out, "static %s", item == TYPE_TEXT ? "EsText *" : item == TYPE_YESNO ? "bool " : "double ");
        cg_append_prefixed(out, "es_ki_", name);
        sb_append(out, "[] = {");
        for (size_t i = 0; i < v->len; i++) {
            const ConstValue *x = &v->items[i];
            sb_append(out, i ? ", " : "");
            if (item == TYPE_TEXT) {
                sb_appendf(out, "&es_s%zu", cg_static_text(g, x->text, x->len));
            } else if (item == TYPE_YESNO) {
                sb_append(out, x->yes ? "true" : "false");
            } else {
                cg_append_number(out, x->number);
            }
        }
        sb_append(out, "};\n");
    }
    sb_append(out, "static EsList ");
    cg_append_prefixed(out, "es_k_", name);
    sb_appendf(out, " = {{ES_OBJ_LIST, ES_IMMORTAL}, %s, %zu, %zu, {", cg_items(item), v->len, v->len);
    if (v->len > 0) {
        sb_appendf(out, ".%s = ", field);
        cg_append_prefixed(out, "es_ki_", name);
    } else {
        sb_append(out, "NULL");
    }
    sb_append(out, "}};\n");
}

static void emit_constant(Codegen *g, const Stmt *stmt, StrBuf *out) {
    const ConstValue *v = &stmt->as.constant.folded;
    const char *name = stmt->as.constant.name.text;
    switch (v->kind) {
    case CONST_NUMBER:
        sb_append(out, "static const double ");
        cg_append_prefixed(out, "es_k_", name);
        sb_append(out, " = ");
        cg_append_number(out, v->number);
        sb_append(out, ";\n");
        break;
    case CONST_LIST: cg_emit_fixed_list(g, name, v, out); break;
    case CONST_MAP: cg_emit_fixed_map(g, name, v, out); break;
    case CONST_TEXT:
        sb_append(out, "static EsText *const ");
        cg_append_prefixed(out, "es_k_", name);
        sb_appendf(out, " = &es_s%zu;\n", cg_static_text(g, v->text, v->len));
        break;
    case CONST_YESNO:
        sb_append(out, "static const bool ");
        cg_append_prefixed(out, "es_k_", name);
        sb_appendf(out, " = %s;\n", v->yes ? "true" : "false");
        break;
    }
}

// static double es_f_area(int es_call, double es_vn_width, double es_vn_height)
// (es_call is the site of the call, for the call stack). A function made
// inside a value also gets the closure it's called through: es_f_0l1(int
// es_call, EsRecord *es_self, ...).
static void append_signature(StrBuf *out, const Stmt *fn) {
    sb_appendf(out, "static %s ", cg_ctype(fn->as.function.result));
    cg_append_function(out, "es_f_", fn);
    sb_append(out, fn->as.function.lambda ? "(int es_call, EsRecord *es_self" : "(int es_call");
    for (size_t i = 0; i < fn->as.function.params.len; i++) {
        Type type = fn->as.function.param_types.items[i];
        sb_appendf(out, ", %s", cg_ctype(type));
        if (!type_counted(type)) sb_append_char(out, ' ');
        cg_append_raw_var(out, fn->as.function.params.items[i].text, type);
    }
    sb_append_char(out, ')');
}

// Temporaries, then hoisted variables (text starts as the empty text, a list
// as es_no_list), then (void) for numbers and yes/no values that might never
// be read.
void cg_emit_declarations(const Codegen *g, StrBuf *out) {
    for (size_t t = 0; t < g->temps.len; t++) {
        Type type = g->temps.items[t];
        // Initialized, so GCC can't think one is read before it's set (a map
        // lookup's temporary is only set when the key is there).
        const char *ctype = cg_ctype(type);
        const char *zero = ctype[strlen(ctype) - 1] == '*' ? "NULL" : type == TYPE_YESNO ? "false" : "0.0";
        sb_appendf(out, "    %s%ses_t%zu = %s;\n", ctype, type_counted(type) ? "" : " ", t + 1, zero);
    }
    for (size_t i = 0; i < g->vars.len; i++) {
        Type type = g->vars.items[i].type;
        if (cg_is_cell(g, g->vars.items[i].name)) {  // made by its "let" (or loop), each time
            sb_append(out, "    EsRecord *");
            cg_append_cell(out, g->vars.items[i].name, type);
            sb_append(out, " = NULL;\n");
            continue;
        }
        sb_appendf(out, "    %s%s", cg_ctype(type), type_counted(type) ? "" : " ");
        cg_append_raw_var(out, g->vars.items[i].name, type);
        sb_appendf(out, " = %s;\n", cg_empty(type));
    }
    for (size_t i = 0; i < g->vars.len; i++) {
        if (type_counted(g->vars.items[i].type) || cg_is_cell(g, g->vars.items[i].name)) continue;
        sb_append(out, "    (void)");
        cg_append_raw_var(out, g->vars.items[i].name, g->vars.items[i].type);
        sb_append(out, ";\n");
    }
}

// The function's description for the call stack:
//   static const char *const es_in_area[] = {"width", "height"};
//   static const EsFunction es_fn_area = {"area", 2, es_in_area, "nn"};
static void emit_function_info(const Stmt *fn, StrBuf *out) {
    size_t n = fn->as.function.params.len;
    if (n > 0) {
        cg_append_function(out, "static const char *const es_in_", fn);
        sb_append(out, "[] = {");
        for (size_t i = 0; i < n; i++) {
            const char *display = fn->as.function.params.items[i].display;
            if (i > 0) sb_append(out, ", ");
            cg_append_c_string(out, display, strlen(display));
        }
        sb_append(out, "};\n");
    }
    cg_append_function(out, "static const EsFunction es_fn_", fn);
    sb_append(out, " = {");
    cg_append_c_string(out, fn->as.function.name.display, strlen(fn->as.function.name.display));
    sb_appendf(out, ", %zu, ", n);
    if (n > 0) {
        cg_append_function(out, "es_in_", fn);
    } else {
        sb_append(out, "NULL");
    }
    sb_append(out, ", \"");
    for (size_t i = 0; i < n; i++) {
        Type type = fn->as.function.param_types.items[i];
        sb_append_char(out, type_is_list(type) ? 'l' : type_is_object(type) ? 'r' : type_is_map(type) ? 'm'
                            : type == TYPE_TEXT ? 't' : type == TYPE_YESNO ? 'b' : type == TYPE_DATE ? 'd' : 'n');
    }
    sb_append(out, "\"};\n");
}

// One input's value for its frame: "{.n = es_vn_width}", "{.t = es_retain(es_vt_label)}".
static void append_input(const Stmt *fn, size_t i, StrBuf *out) {
    Type type = fn->as.function.param_types.items[i];
    bool record = type_is_object(type);
    sb_append(out, type_is_list(type) || type_is_map(type) ? "{.list = " : record ? "{.r = es_record_retain("
                   : type == TYPE_TEXT ? "{.t = es_retain(" : type == TYPE_YESNO ? "{.b = " : "{.n = ");
    cg_append_raw_var(out, fn->as.function.params.items[i].text, type);
    sb_append(out, type == TYPE_TEXT || record ? ")}" : "}");
}

// The function's frame, pushed onto the call stack with the values it was given:
//   es_enter_one(&es_fn_halve, es_call, (EsArg){.n = es_vn_x});
//   es_enter_many(&es_fn_area, es_call, (EsArg[]){{.n = es_vn_width}, {.t = es_retain(es_vt_label)}});
static void emit_frame(const Stmt *fn, StrBuf *out) {
    size_t n = fn->as.function.params.len;
    if (n == 0) {
        cg_append_function(out, "    es_enter(&es_fn_", fn);
        sb_append(out, ", es_call);\n");
        return;
    }
    if (n == 1) {
        cg_append_function(out, "    es_enter_one(&es_fn_", fn);
        sb_append(out, ", es_call, (EsArg)");
        append_input(fn, 0, out);
        sb_append(out, ");\n");
        return;
    }
    cg_append_function(out, "    es_enter_many(&es_fn_", fn);
    sb_append(out, ", es_call, (EsArg[]){");
    for (size_t i = 0; i < n; i++) {
        sb_append(out, i > 0 ? ", " : "");
        append_input(fn, i, out);
    }
    sb_append(out, "});\n");
}

// Inputs that functions made inside share go into cells first; a function
// made inside a value finds the cells it uses in its closure:
//   EsRecord *es_cvn_x = es_cell_n(es_vn_x);
//   EsRecord *es_cvn_count = ((es_L1 *)es_self)->c0;
static void emit_cells(const Codegen *f, const Stmt *fn, StrBuf *out) {
    for (size_t i = 0; i < fn->as.function.captures.len; i++) {
        const Capture *capture = &fn->as.function.captures.items[i];
        sb_append(out, "    EsRecord *");
        cg_append_cell(out, capture->name, capture->type);
        sb_append(out, " = ");
        cg_append_function(out, "((es_L", fn);
        sb_appendf(out, " *)es_self)->c%zu;\n", i);
    }
    if (fn->as.function.lambda) sb_append(out, "    (void)es_self;\n");
    for (size_t i = 0; i < fn->as.function.params.len; i++) {
        const char *param = fn->as.function.params.items[i].text;
        Type type = fn->as.function.param_types.items[i];
        if (!cg_is_cell(f, param)) continue;
        sb_append(out, "    EsRecord *");
        cg_append_cell(out, param, type);
        sb_appendf(out, " = es_cell_%s(%s", cg_value_suffix(type), type_counted(type) ? cg_retain(type) : "");
        sb_append_char(out, '(');
        cg_append_raw_var(out, param, type);
        sb_append(out, "));\n");
    }
}

// The C function for one EasyScript function. Falling off the end releases
// its text; one that gives back a value stops with an error instead, because
// it has nothing to give back.
void cg_emit_function(Codegen *g, const Stmt *fn, StrBuf *out) {
    Codegen f = {0};
    f.arena = g->arena;
    f.records = g->records;
    f.texts = g->texts;
    f.inits = g->inits;
    f.helpers = g->helpers;
    f.sites = g->sites;
    f.functions = g->functions;
    f.constants = g->constants;
    f.function = fn;
    f.loop_ids = g->loop_ids;
    f.closures = g->closures;
    f.plan = g->plan;
    for (size_t i = 0; i < fn->as.function.cells.len; i++) vec_push(g->arena, &f.cells, fn->as.function.cells.items[i]);
    for (size_t i = 0; i < fn->as.function.captures.len; i++) {
        vec_push(g->arena, &f.borrowed, fn->as.function.captures.items[i].name);
    }
    sb_init(&f.body, g->arena);
    cg_push_scope(&f);
    for (size_t i = 0; i < fn->as.function.body.len; i++) {
        cg_emit_stmt(&f, fn->as.function.body.items[i]);
    }
    g->loop_ids = f.loop_ids;

    emit_function_info(fn, out);
    append_signature(out, fn);
    sb_append(out, " {\n");
    cg_emit_declarations(&f, out);
    for (size_t i = 0; i < fn->as.function.params.len; i++) {  // inputs it might never use
        sb_append(out, "    (void)");
        cg_append_raw_var(out, fn->as.function.params.items[i].text, fn->as.function.param_types.items[i]);
        sb_append(out, ";\n");
    }
    emit_cells(&f, fn, out);
    emit_frame(fn, out);
    sb_append_n(out, f.body.data, f.body.len);
    f.body.len = 0;
    if (fn->as.function.result == TYPE_NOTHING) {
        cg_emit_drop_all(&f);
        sb_appendf(&f.body, "    %s", cg_leave_code(&f));
    } else {
        sb_appendf(&f.body, "    es_no_result(%zu, ", cg_site(&f, fn->as.function.name.pos));
        cg_append_c_string(&f.body, fn->as.function.name.display, strlen(fn->as.function.name.display));
        sb_append(&f.body, ");\n");
    }
    sb_append_n(out, f.body.data, f.body.len);
    sb_append(out, "}\n\n");
}

void cg_plan(Codegen *g, SourcePos pos, const char *fmt, ...) {
    if (!g->plan) return;
    StrBuf text;
    sb_init(&text, g->arena);
    va_list args;
    va_start(args, fmt);
    sb_vappendf(&text, fmt, args);
    va_end(args);
    PlanLine line = {pos.line, pos.file, text.data};
    vec_push(g->arena, &g->plan->lines, line);
}

// The plan's lines in the order of the program (stable for one line).
static void write_plan(const Program *whole, PlanPool *pool, StrBuf *plan) {
    for (size_t i = 1; i < pool->lines.len; i++) {  // insertion sort: plans are short
        PlanLine x = pool->lines.items[i];
        size_t j = i;
        while (j > 0 && (pool->lines.items[j - 1].file > x.file ||
                         (pool->lines.items[j - 1].file == x.file && pool->lines.items[j - 1].line > x.line))) {
            pool->lines.items[j] = pool->lines.items[j - 1];
            j--;
        }
        pool->lines.items[j] = x;
    }
    for (size_t i = 0; i < pool->lines.len; i++) {
        const PlanLine *line = &pool->lines.items[i];
        sb_appendf(plan, "Line %zu", line->line);
        if (whole->files.len > 0) sb_appendf(plan, " of %s", whole->files.items[line->file].display);
        sb_appendf(plan, ": %s\n", line->text);
    }
    if (pool->lines.len == 0) sb_append(plan, "There are no list questions, filters or sorts here.\n");
}

void codegen_c(Arena *arena, const Program *whole, const char *source_table, StrBuf *out, StrBuf *plan) {
    const Block *program = &whole->body;
    TextPool texts = {0};
    sb_init(&texts.code, arena);
    HelperPool helpers = {0};
    sb_init(&helpers.code, arena);
    SitePool sites = {0};
    sb_init(&sites.code, arena);
    StrBuf inits;
    sb_init(&inits, arena);
    ClosurePool closures = {0};
    sb_init(&closures.head, arena);
    sb_init(&closures.code, arena);
    Codegen g = {0};
    g.closures = &closures;
    PlanPool plan_pool = {0};
    if (plan) g.plan = &plan_pool;
    for (size_t i = 0; i < whole->cells.len; i++) vec_push(arena, &g.cells, whole->cells.items[i]);
    g.arena = arena;
    g.records = &whole->records;
    g.texts = &texts;
    g.inits = &inits;
    g.helpers = &helpers;
    g.sites = &sites;
    sb_init(&g.body, arena);
    cg_push_scope(&g);
    for (size_t i = 0; i < program->len; i++) {
        if (program->items[i]->kind == STMT_FUNCTION) vec_push(arena, &g.functions, program->items[i]);
        if (program->items[i]->kind == STMT_CONSTANT) vec_push(arena, &g.constants, program->items[i]);
    }
    for (size_t i = 0; i < program->len; i++) {
        cg_emit_stmt(&g, program->items[i]);
    }
    StrBuf functions;
    sb_init(&functions, arena);
    for (size_t i = 0; i < g.functions.len; i++) {
        cg_emit_function(&g, g.functions.items[i], &functions);
    }

    sb_append_n(out, (const char *)es_runtime_source, es_runtime_source_len);
    sb_append(out, "\n/* --- The program's lines, for runtime errors --- */\n\n");
    sb_append(out, source_table);
    sb_append(out, "\n/* --- Generated by EasyScript --- */\n\n");
    StrBuf constants;
    sb_init(&constants, arena);
    for (size_t i = 0; i < g.constants.len; i++) {
        emit_constant(&g, g.constants.items[i], &constants);
    }
    cg_emit_records(&g, &constants);
    cg_emit_variants(&g, &constants);
    sb_append_n(out, texts.code.data, texts.code.len);  // after the constants and records added theirs
    sb_append_n(out, constants.data, constants.len);
    if (g.functions.len > 0) sb_append_char(out, '\n');
    for (size_t i = 0; i < g.functions.len; i++) {  // prototypes, for calls in any order
        append_signature(out, g.functions.items[i]);
        sb_append(out, ";\n");
    }
    sb_append_char(out, '\n');
    sb_append_n(out, closures.head.data, closures.head.len);  // functions kept as values
    cg_emit_function_refs(&g, out);
    sb_append(out, "static const EsSite es_sites[] = {\n    {0, 0, 0, 0},\n");  // where runtime calls come from
    sb_append_n(out, sites.code.data, sites.code.len);
    sb_append(out, "};\n\n");
    sb_append_n(out, helpers.code.data, helpers.code.len);  // "every" and "only", before the functions that use them
    sb_append_n(out, functions.data, functions.len);
    sb_append_n(out, closures.code.data, closures.code.len);
    sb_append(out, "int main(int argc, char **argv) {\n");
    cg_emit_declarations(&g, out);
    for (size_t i = 0; i < g.constants.len; i++) {  // constants nothing uses (costs nothing)
        sb_append(out, "    (void)");
        cg_append_prefixed(out, "es_k_", g.constants.items[i]->as.constant.name.text);
        sb_append(out, ";\n");
    }
    sb_append(out, "    es_init(es_sites, es_source, es_source_line_count, es_source_files);\n");
    sb_append(out, "    es_args(argc, argv);\n");
    sb_append_n(out, inits.data, inits.len);  // the fixed maps
    sb_append_n(out, g.body.data, g.body.len);
    if (g.stops) sb_append(out, "es_end:\n");
    g.depth = 0;
    g.body.len = 0;
    cg_emit_drop_all(&g);
    sb_append_n(out, g.body.data, g.body.len);
    sb_append(out, "    es_finish(true);\n    return es_status;\n}\n");
    if (plan) write_plan(whole, &plan_pool, plan);
}
