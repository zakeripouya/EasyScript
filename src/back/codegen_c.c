// C code generation. The checker has decided the kind of every value, so the
// program uses plain C types: a number is a double, yes/no a bool, and text
// an EsText * (runtime/es_memory.h). There is no tagged value. Runtime calls
// that can fail get the source line, so their errors can say where.
//
// Memory (docs/memory.md), for text only: every text expression produces an
// owned value, and runtime calls and EasyScript functions consume their text
// arguments, so nested calls need no bookkeeping. Reading a text variable
// retains it; storing goes through es_set, which releases the old text after
// the new one is worked out. A block releases the text variables first made
// in it when it ends; stop the loop, skip this one and give back release the
// ones of every block they leave, and main()'s are released when the program
// ends. Numbers and yes/no values own nothing and are never retained or
// released.
//
// Variables are C locals of main() or of their function, so their addresses
// never escape and the C compiler can keep numbers in registers.

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "back/codegen_internal.h"

// Generated from runtime/es_memory.h and es_runtime.h at build time by tools/embed.c.
extern const unsigned char es_runtime_source[];
extern const size_t es_runtime_source_len;

// Starts a line of the body at the current nesting depth.
static void indent(Codegen *g) {
    sb_append_repeat(&g->body, ' ', 4 * (g->depth + 1));
}

// --- Releasing text variables ---------------------------------------------------

static void emit_drop(Codegen *g, const char *name) {
    indent(g);
    sb_append(&g->body, "es_drop(&");
    cg_append_var(&g->body, name, TYPE_TEXT);
    sb_append(&g->body, ");\n");
}

// Releases the text variables of scopes[from] and every scope inside it.
static void emit_drop_scopes(Codegen *g, size_t from) {
    for (size_t i = g->scopes.len; i > from; i--) {
        const NameScope *scope = &g->scopes.items[i - 1];
        for (size_t j = 0; j < scope->names.len; j++) emit_drop(g, scope->names.items[j].name);
    }
}

static void push_scope(Codegen *g) {
    NameScope scope = {0};
    vec_push(g->arena, &g->scopes, scope);
}

// Releases the innermost scope's text variables, then leaves it.
static void pop_scope(Codegen *g) {
    emit_drop_scopes(g, g->scopes.len - 1);
    g->scopes.len--;
}

// Leaving a function (its text inputs and every text variable it makes) or
// the program (every text variable of main()).
static void emit_drop_all(Codegen *g) {
    if (g->function) {
        for (size_t i = 0; i < g->function->as.function.params.len; i++) {
            if (g->function->as.function.param_types.items[i] == TYPE_TEXT) {
                emit_drop(g, g->function->as.function.params.items[i].text);
            }
        }
    }
    for (size_t i = 0; i < g->vars.len; i++) {
        if (g->vars.items[i].type == TYPE_TEXT) emit_drop(g, g->vars.items[i].name);
    }
}

// --- Statements ----------------------------------------------------------------

// A variable gets a value: "es_vn_x = VALUE;", or "es_set(&es_vt_x, VALUE);"
// for text, which releases the text it held.
static void emit_store(Codegen *g, const char *name, Type type, const char *value) {
    indent(g);
    if (type == TYPE_TEXT) {
        sb_append(&g->body, "es_set(&");
        cg_append_var(&g->body, name, type);
        sb_appendf(&g->body, ", %s);\n", value);
    } else {
        cg_append_var(&g->body, name, type);
        sb_appendf(&g->body, " = %s;\n", value);
    }
}

static const char *expr_code(Codegen *g, const Expr *expr) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    cg_emit_expr(g, expr, &sb);
    return sb.data;
}

static const char *text_code(Codegen *g, const Expr *expr) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    cg_emit_text(g, expr, &sb);
    return sb.data;
}

// add/subtract/increase/decrease/multiply/divide: x = x + amount, or es_div.
static void emit_change(Codegen *g, const Stmt *stmt) {
    const char *name = stmt->as.change.target.text;
    StrBuf var;
    sb_init(&var, g->arena);
    cg_append_var(&var, name, TYPE_NUMBER);
    const char *amount = expr_code(g, stmt->as.change.amount);
    const char *value;
    switch (stmt->as.change.op) {
    case CHANGE_ADD: value = arena_sprintf(g->arena, "%s + %s", var.data, amount); break;
    case CHANGE_SUBTRACT: value = arena_sprintf(g->arena, "%s - %s", var.data, amount); break;
    case CHANGE_MULTIPLY: value = arena_sprintf(g->arena, "%s * %s", var.data, amount); break;
    default: value = arena_sprintf(g->arena, "es_div(%zu, %s, %s)", stmt->pos.line, var.data, amount); break;
    }
    emit_store(g, name, TYPE_NUMBER, value);
}

// write/append: the text first, then the file name (left to right).
static void emit_file_write(Codegen *g, const Stmt *stmt) {
    const char *text = text_code(g, stmt->as.file_write.text);
    if (cg_has_effects(g, stmt->as.file_write.text) && cg_has_effects(g, stmt->as.file_write.path)) {
        size_t t = cg_new_temp(g, TYPE_TEXT);
        indent(g);
        sb_appendf(&g->body, "es_t%zu = %s;\n", t, text);
        text = arena_sprintf(g->arena, "es_t%zu", t);
    }
    indent(g);
    sb_appendf(&g->body, "es_write_file(%zu, %s, %s, %d);\n", stmt->pos.line, text,
               expr_code(g, stmt->as.file_write.path), stmt->kind == STMT_APPEND_FILE);
}

static void emit_stmt(Codegen *g, const Stmt *stmt);

static void emit_block(Codegen *g, const Block *block) {
    g->depth++;
    push_scope(g);
    for (size_t i = 0; i < block->len; i++) {
        emit_stmt(g, block->items[i]);
    }
    pop_scope(g);
    g->depth--;
}

// if (C1) { ... } else if (C2) { ... } else { ... }
static void emit_if(Codegen *g, const Stmt *stmt) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        if (i == 0) {
            indent(g);
        } else {
            sb_append(&g->body, " else ");
        }
        if (branch->condition) sb_appendf(&g->body, "if (%s) ", expr_code(g, branch->condition));
        sb_append(&g->body, "{\n");
        emit_block(g, &branch->body);
        indent(g);
        sb_append_char(&g->body, '}');
    }
    sb_append_char(&g->body, '\n');
}

// es_tN = NUMBER; (so a count's values are worked out once, left to right)
static size_t emit_temp(Codegen *g, const Expr *expr) {
    size_t t = cg_new_temp(g, TYPE_NUMBER);
    indent(g);
    sb_appendf(&g->body, "es_t%zu = %s;\n", t, expr_code(g, expr));
    return t;
}

// The line that opens the C loop, for each kind of loop.
static void emit_loop_head(Codegen *g, const Stmt *stmt, size_t id) {
    size_t line = stmt->pos.line;
    switch (stmt->as.loop.kind) {
    case LOOP_COUNT: {
        size_t from = emit_temp(g, stmt->as.loop.from);
        size_t to = emit_temp(g, stmt->as.loop.to);
        size_t step = stmt->as.loop.step ? emit_temp(g, stmt->as.loop.step) : 0;
        indent(g);
        sb_appendf(&g->body, "EsCount es_c%zu = es_count_start(%zu, es_t%zu, es_t%zu, ", id, line, from, to);
        if (step) {
            sb_appendf(&g->body, "es_t%zu, 1, %d);\n", step, stmt->as.loop.down);
        } else {
            sb_appendf(&g->body, "1.0, 0, %d);\n", stmt->as.loop.down);
        }
        indent(g);
        sb_appendf(&g->body, "for (long long es_i%zu = 0; es_i%zu <= es_c%zu.last; es_i%zu++) {\n", id, id, id, id);
        indent(g);
        sb_append(&g->body, "    ");
        cg_append_var(&g->body, stmt->as.loop.var.text, TYPE_NUMBER);
        sb_appendf(&g->body, " = es_count_value(&es_c%zu, es_i%zu);\n", id, id);
        break;
    }
    case LOOP_TIMES:
        indent(g);
        sb_appendf(&g->body, "long long es_n%zu = es_times(%zu, %s);\n", id, line, expr_code(g, stmt->as.loop.times));
        indent(g);
        sb_appendf(&g->body, "for (long long es_i%zu = 1; es_i%zu <= es_n%zu; es_i%zu++) {\n", id, id, id, id);
        break;
    case LOOP_WHILE:
    case LOOP_UNTIL:
        indent(g);
        sb_appendf(&g->body, "while (%s%s) {\n", stmt->as.loop.kind == LOOP_UNTIL ? "!" : "",
                   expr_code(g, stmt->as.loop.condition));
        break;
    case LOOP_FOREVER:
        indent(g);
        sb_append(&g->body, "for (;;) {\n");
        break;
    }
}

// { <setup> for/while (...) { body } }
static void emit_loop(Codegen *g, const Stmt *stmt) {
    size_t id = ++g->loop_ids;
    indent(g);
    sb_append(&g->body, "{\n");
    g->depth++;
    push_scope(g);
    if (stmt->as.loop.kind == LOOP_COUNT) cg_declare_name(g, stmt->as.loop.var.text, TYPE_NUMBER);
    emit_loop_head(g, stmt, id);
    LoopCode loop = {stmt, id, g->scopes.len};
    vec_push(g->arena, &g->loops, loop);
    emit_block(g, &stmt->as.loop.body);
    g->loops.len--;
    indent(g);
    sb_append(&g->body, "}\n");
    pop_scope(g);
    g->depth--;
    indent(g);
    sb_append(&g->body, "}\n");
}

// give back E / return: the value is worked out before the function's text
// is released.
static void emit_return(Codegen *g, const Stmt *stmt) {
    size_t t = 0;
    if (stmt->as.returned) {
        t = cg_new_temp(g, stmt->as.returned->type);
        indent(g);
        sb_appendf(&g->body, "es_t%zu = %s;\n", t, expr_code(g, stmt->as.returned));
    }
    emit_drop_all(g);
    indent(g);
    if (t) {
        sb_appendf(&g->body, "es_leave();\n");
        indent(g);
        sb_appendf(&g->body, "return es_t%zu;\n", t);
    } else {
        sb_append(&g->body, "es_leave();\n");
        indent(g);
        sb_append(&g->body, "return;\n");
    }
}

// A call written as a sentence: text it gives back is released, other values ignored.
static void emit_call_statement(Codegen *g, const Stmt *stmt) {
    const Stmt *fn = cg_find_function(g, stmt->as.call->as.call.name);
    Type result = fn ? fn->as.function.result : TYPE_NOTHING;
    const char *call = expr_code(g, stmt->as.call);
    indent(g);
    if (result == TYPE_TEXT) {
        sb_appendf(&g->body, "es_release(%s);\n", call);
    } else if (result == TYPE_NOTHING) {
        sb_appendf(&g->body, "%s;\n", call);
    } else {
        sb_appendf(&g->body, "(void)%s;\n", call);
    }
}

static void emit_stop(Codegen *g) {
    indent(g);
    if (!g->function) {  // the end of main() releases everything
        sb_append(&g->body, "goto es_end;\n");
        g->stops = true;
        return;
    }
    sb_append(&g->body, "{\n");
    g->depth++;
    emit_drop_all(g);
    indent(g);
    sb_append(&g->body, "es_stop();\n");
    g->depth--;
    indent(g);
    sb_append(&g->body, "}\n");
}

static void emit_stmt(Codegen *g, const Stmt *stmt) {
    indent(g);
    sb_appendf(&g->body, "/* line %zu */\n", stmt->pos.line);
    switch (stmt->kind) {
    case STMT_LET:
    case STMT_SET: {
        const Expr *value = stmt->as.assign.value;
        if (stmt->kind == STMT_LET) cg_declare_name(g, stmt->as.assign.name.text, value->type);
        emit_store(g, stmt->as.assign.name.text, value->type, expr_code(g, value));
        break;
    }
    case STMT_CHANGE: emit_change(g, stmt); break;
    case STMT_SAY:
        indent(g);
        sb_appendf(&g->body, "es_say(%s);\n", text_code(g, stmt->as.value));
        break;
    case STMT_ASK:
        cg_declare_name(g, stmt->as.ask.answer.text, TYPE_TEXT);
        emit_store(g, stmt->as.ask.answer.text, TYPE_TEXT,
                   arena_sprintf(g->arena, "es_ask(%s)", text_code(g, stmt->as.ask.prompt)));
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: emit_file_write(g, stmt); break;
    case STMT_READ_FILE:
        cg_declare_name(g, stmt->as.read_file.name.text, TYPE_TEXT);
        emit_store(g, stmt->as.read_file.name.text, TYPE_TEXT,
                   arena_sprintf(g->arena, "es_read_file(%zu, %s)", stmt->pos.line,
                                 expr_code(g, stmt->as.read_file.path)));
        break;
    case STMT_STOP: emit_stop(g); break;
    case STMT_IF: emit_if(g, stmt); break;
    case STMT_LOOP: emit_loop(g, stmt); break;
    case STMT_FUNCTION: break;  // generated separately, before main()
    case STMT_CONSTANT: break;  // a static initializer, worked out by the compiler
    case STMT_RETURN: emit_return(g, stmt); break;
    case STMT_CALL: emit_call_statement(g, stmt); break;
    case STMT_BREAK:
    case STMT_CONTINUE:
        emit_drop_scopes(g, vec_last(&g->loops).scope);
        indent(g);
        sb_append(&g->body, stmt->kind == STMT_BREAK ? "break;\n" : "continue;\n");
        break;
    }
}

// --- Program -------------------------------------------------------------------

// static const double es_k_rate = 0.20000000000000001;
// static EsText *const es_k_greeting = &es_s3;  (immortal)
static void emit_constant(Codegen *g, const Stmt *stmt, StrBuf *out) {
    const ConstValue *v = &stmt->as.constant.folded;
    const char *name = stmt->as.constant.name.text;
    switch (v->kind) {
    case CONST_NUMBER:
        sb_append(out, "static const double ");
        cg_append_prefixed(out, "es_k_", name);
        if (isnan(v->number)) {
            sb_append(out, " = NAN;\n");
        } else if (isinf(v->number)) {
            sb_appendf(out, " = %sHUGE_VAL;\n", v->number < 0 ? "-" : "");
        } else {
            sb_appendf(out, " = %.17g;\n", v->number);
        }
        break;
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

// static double es_f_area(int es_line, double es_vn_width, double es_vn_height)
static void append_signature(StrBuf *out, const Stmt *fn) {
    sb_appendf(out, "static %s ", cg_ctype(fn->as.function.result));
    cg_append_prefixed(out, "es_f_", fn->as.function.name.text);
    sb_append(out, "(int es_line");
    for (size_t i = 0; i < fn->as.function.params.len; i++) {
        Type type = fn->as.function.param_types.items[i];
        sb_appendf(out, ", %s", cg_ctype(type));
        if (type != TYPE_TEXT) sb_append_char(out, ' ');
        cg_append_var(out, fn->as.function.params.items[i].text, type);
    }
    sb_append_char(out, ')');
}

// Temporaries, then hoisted variables (text starts as the empty text), then
// (void) for numbers and yes/no values that might never be read.
static void emit_declarations(const Codegen *g, StrBuf *out) {
    for (size_t t = 0; t < g->temps.len; t++) {
        Type type = g->temps.items[t];
        sb_appendf(out, "    %s%ses_t%zu;\n", cg_ctype(type), type == TYPE_TEXT ? "" : " ", t + 1);
    }
    for (size_t i = 0; i < g->vars.len; i++) {
        Type type = g->vars.items[i].type;
        sb_appendf(out, "    %s%s", cg_ctype(type), type == TYPE_TEXT ? "" : " ");
        cg_append_var(out, g->vars.items[i].name, type);
        sb_append(out, type == TYPE_TEXT ? " = &es_empty_text;\n" : type == TYPE_YESNO ? " = false;\n" : " = 0;\n");
    }
    for (size_t i = 0; i < g->vars.len; i++) {
        if (g->vars.items[i].type == TYPE_TEXT) continue;
        sb_append(out, "    (void)");
        cg_append_var(out, g->vars.items[i].name, g->vars.items[i].type);
        sb_append(out, ";\n");
    }
}

// The C function for one EasyScript function. Falling off the end releases
// its text; one that gives back a value stops with an error instead, because
// it has nothing to give back.
static void emit_function(Codegen *g, const Stmt *fn, StrBuf *out) {
    Codegen f = {0};
    f.arena = g->arena;
    f.texts = g->texts;
    f.functions = g->functions;
    f.constants = g->constants;
    f.function = fn;
    f.loop_ids = g->loop_ids;
    sb_init(&f.body, g->arena);
    push_scope(&f);
    for (size_t i = 0; i < fn->as.function.body.len; i++) {
        emit_stmt(&f, fn->as.function.body.items[i]);
    }
    g->loop_ids = f.loop_ids;

    append_signature(out, fn);
    sb_append(out, " {\n");
    emit_declarations(&f, out);
    for (size_t i = 0; i < fn->as.function.params.len; i++) {  // inputs it might never use
        sb_append(out, "    (void)");
        cg_append_var(out, fn->as.function.params.items[i].text, fn->as.function.param_types.items[i]);
        sb_append(out, ";\n");
    }
    sb_append(out, "    es_enter(es_line);\n");
    sb_append_n(out, f.body.data, f.body.len);
    f.body.len = 0;
    if (fn->as.function.result == TYPE_NOTHING) {
        emit_drop_all(&f);
        sb_append(&f.body, "    es_leave();\n");
    } else {
        sb_appendf(&f.body, "    es_no_result(%zu, ", fn->pos.line);
        cg_append_c_string(&f.body, fn->as.function.name.text, strlen(fn->as.function.name.text));
        sb_append(&f.body, ");\n");
    }
    sb_append_n(out, f.body.data, f.body.len);
    sb_append(out, "}\n\n");
}

void codegen_c(Arena *arena, const Block *program, StrBuf *out) {
    TextPool texts = {0};
    sb_init(&texts.code, arena);
    Codegen g = {0};
    g.arena = arena;
    g.texts = &texts;
    sb_init(&g.body, arena);
    push_scope(&g);
    for (size_t i = 0; i < program->len; i++) {
        if (program->items[i]->kind == STMT_FUNCTION) vec_push(arena, &g.functions, program->items[i]);
        if (program->items[i]->kind == STMT_CONSTANT) vec_push(arena, &g.constants, program->items[i]);
    }
    for (size_t i = 0; i < program->len; i++) {
        emit_stmt(&g, program->items[i]);
    }
    StrBuf functions;
    sb_init(&functions, arena);
    for (size_t i = 0; i < g.functions.len; i++) {
        emit_function(&g, g.functions.items[i], &functions);
    }

    sb_append_n(out, (const char *)es_runtime_source, es_runtime_source_len);
    sb_append(out, "\n/* --- Generated by EasyScript --- */\n\n");
    StrBuf constants;
    sb_init(&constants, arena);
    for (size_t i = 0; i < g.constants.len; i++) {
        emit_constant(&g, g.constants.items[i], &constants);
    }
    sb_append_n(out, texts.code.data, texts.code.len);  // after the constants added theirs
    sb_append_n(out, constants.data, constants.len);
    if (g.functions.len > 0) sb_append_char(out, '\n');
    for (size_t i = 0; i < g.functions.len; i++) {  // prototypes, for calls in any order
        append_signature(out, g.functions.items[i]);
        sb_append(out, ";\n");
    }
    sb_append_char(out, '\n');
    sb_append_n(out, functions.data, functions.len);
    sb_append(out, "int main(void) {\n");
    emit_declarations(&g, out);
    for (size_t i = 0; i < g.constants.len; i++) {  // constants nothing uses (costs nothing)
        sb_append(out, "    (void)");
        cg_append_prefixed(out, "es_k_", g.constants.items[i]->as.constant.name.text);
        sb_append(out, ";\n");
    }
    sb_append(out, "    es_init();\n");
    sb_append_n(out, g.body.data, g.body.len);
    if (g.stops) sb_append(out, "es_end:\n");
    g.depth = 0;
    g.body.len = 0;
    emit_drop_all(&g);
    sb_append_n(out, g.body.data, g.body.len);
    sb_append(out, "    es_finish(true);\n    return 0;\n}\n");
}
