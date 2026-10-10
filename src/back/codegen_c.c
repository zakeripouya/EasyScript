// C code generation. Every EasyScript value is an EsValue (see
// runtime/es_runtime.h) and every operation is a runtime call that gets the
// source line, so runtime errors can say where they happened.
//
// C doesn't fix the order in which function arguments are evaluated, so the
// left operand of a binary operation is stored in a temporary first:
// (t1 = LEFT, es_add(line, t1, RIGHT)). That keeps evaluation, and so which
// runtime error appears first, left to right.

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "back/codegen_internal.h"

// Generated from runtime/es_runtime.h at build time by tools/embed.c.
extern const unsigned char es_runtime_source[];
extern const size_t es_runtime_source_len;


// Starts a line of main()'s body at the current nesting depth.
static void indent(Codegen *g) {
    sb_append_repeat(&g->body, ' ', 4 * (g->depth + 1));
}


// --- Statements ----------------------------------------------------------------

static const char *change_function(ChangeOp op) {
    static const char *const names[] = {"es_add", "es_sub", "es_mul", "es_div"};
    return names[op];
}

// name = <rest of the line, written by the caller>
static void begin_assignment(Codegen *g, const char *name) {
    indent(g);
    cg_append_name(&g->body, name);
    sb_append(&g->body, " = ");
}

static void emit_assign(Codegen *g, const char *name, const Expr *value) {
    begin_assignment(g, name);
    cg_emit_expr(g, value, &g->body);
    sb_append(&g->body, ";\n");
}

static void emit_change(Codegen *g, const Stmt *stmt) {
    const char *name = stmt->as.change.target.text;
    begin_assignment(g, name);
    sb_appendf(&g->body, "%s(%zu, ", change_function(stmt->as.change.op), stmt->pos.line);
    cg_append_name(&g->body, name);
    sb_append(&g->body, ", ");
    cg_emit_expr(g, stmt->as.change.amount, &g->body);
    sb_append(&g->body, ");\n");
}

static void emit_file_write(Codegen *g, const Stmt *stmt) {
    size_t t = cg_new_temp(g);
    indent(g);
    sb_appendf(&g->body, "es_t%zu = ", t);
    cg_emit_expr(g, stmt->as.file_write.text, &g->body);
    sb_append(&g->body, ";\n");
    indent(g);
    sb_appendf(&g->body, "es_write_file(%zu, es_t%zu, ", stmt->pos.line, t);
    cg_emit_expr(g, stmt->as.file_write.path, &g->body);
    sb_appendf(&g->body, ", %d);\n", stmt->kind == STMT_APPEND_FILE);
}

static void emit_stmt(Codegen *g, const Stmt *stmt);

static void emit_block(Codegen *g, const Block *block) {
    g->depth++;
    for (size_t i = 0; i < block->len; i++) {
        emit_stmt(g, block->items[i]);
    }
    g->depth--;
}

// if (es_if(line, C1)) { ... } else if (es_if(line, C2)) { ... } else { ... }
static void emit_if(Codegen *g, const Stmt *stmt) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        if (i == 0) {
            indent(g);
        } else {
            sb_append(&g->body, " else ");
        }
        if (branch->condition) {
            sb_appendf(&g->body, "if (es_if(%zu, ", branch->pos.line);
            cg_emit_expr(g, branch->condition, &g->body);
            sb_append(&g->body, ")) ");
        }
        sb_append(&g->body, "{\n");
        emit_block(g, &branch->body);
        indent(g);
        sb_append_char(&g->body, '}');
    }
    sb_append_char(&g->body, '\n');
}

// es_tN = EXPR; (so a loop's values are worked out once, left to right)
static size_t emit_temp(Codegen *g, const Expr *expr) {
    size_t t = cg_new_temp(g);
    indent(g);
    sb_appendf(&g->body, "es_t%zu = ", t);
    cg_emit_expr(g, expr, &g->body);
    sb_append(&g->body, ";\n");
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
            sb_appendf(&g->body, "es_nothing(), 0, %d);\n", stmt->as.loop.down);
        }
        indent(g);
        sb_appendf(&g->body, "for (long long es_i%zu = 0; es_count_next(&es_c%zu, es_i%zu, &", id, id, id);
        cg_append_name(&g->body, stmt->as.loop.var.text);
        sb_appendf(&g->body, "); es_i%zu++) {\n", id);
        break;
    }
    case LOOP_TIMES:
        indent(g);
        sb_appendf(&g->body, "long long es_n%zu = es_times(%zu, ", id, line);
        cg_emit_expr(g, stmt->as.loop.times, &g->body);
        sb_append(&g->body, ");\n");
        indent(g);
        sb_appendf(&g->body, "for (long long es_i%zu = 1; es_i%zu <= es_n%zu; es_i%zu++) {\n", id, id, id, id);
        break;
    case LOOP_WHILE:
    case LOOP_UNTIL:
        indent(g);
        sb_appendf(&g->body, "while (%ses_loop_condition(%zu, ", stmt->as.loop.kind == LOOP_UNTIL ? "!" : "", line);
        cg_emit_expr(g, stmt->as.loop.condition, &g->body);
        sb_append(&g->body, ")) {\n");
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
    if (stmt->as.loop.kind == LOOP_COUNT) cg_declare_name(g, stmt->as.loop.var.text);
    indent(g);
    sb_append(&g->body, "{\n");
    g->depth++;
    emit_loop_head(g, stmt, id);
    LoopCode loop = {stmt, id};
    vec_push(g->arena, &g->loops, loop);
    emit_block(g, &stmt->as.loop.body);
    g->loops.len--;
    indent(g);
    sb_append(&g->body, "}\n");
    g->depth--;
    indent(g);
    sb_append(&g->body, "}\n");
}

static void emit_stmt(Codegen *g, const Stmt *stmt) {
    indent(g);
    sb_appendf(&g->body, "/* line %zu */\n", stmt->pos.line);
    switch (stmt->kind) {
    case STMT_LET:
        cg_declare_name(g, stmt->as.assign.name.text);
        emit_assign(g, stmt->as.assign.name.text, stmt->as.assign.value);
        break;
    case STMT_SET: emit_assign(g, stmt->as.assign.name.text, stmt->as.assign.value); break;
    case STMT_CHANGE: emit_change(g, stmt); break;
    case STMT_SAY:
        indent(g);
        sb_append(&g->body, "es_say(");
        cg_emit_expr(g, stmt->as.value, &g->body);
        sb_append(&g->body, ");\n");
        break;
    case STMT_ASK:
        cg_declare_name(g, stmt->as.ask.answer.text);
        begin_assignment(g, stmt->as.ask.answer.text);
        cg_emit_call1(g, "es_ask", stmt->pos.line, stmt->as.ask.prompt, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: emit_file_write(g, stmt); break;
    case STMT_READ_FILE:
        cg_declare_name(g, stmt->as.read_file.name.text);
        begin_assignment(g, stmt->as.read_file.name.text);
        cg_emit_call1(g, "es_read_file", stmt->pos.line, stmt->as.read_file.path, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_STOP:
        indent(g);
        sb_append(&g->body, "es_stop();\n");
        break;
    case STMT_IF: emit_if(g, stmt); break;
    case STMT_LOOP: emit_loop(g, stmt); break;
    case STMT_FUNCTION: break;  // generated separately, before main()
    case STMT_CONSTANT: break;  // a static initializer, worked out by the compiler
    case STMT_RETURN:
        indent(g);
        sb_append(&g->body, "return es_leave(");
        if (stmt->as.returned) {
            cg_emit_expr(g, stmt->as.returned, &g->body);
        } else {
            sb_append(&g->body, "es_nothing()");
        }
        sb_append(&g->body, ");\n");
        break;
    case STMT_CALL:
        indent(g);
        cg_emit_expr(g, stmt->as.call, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_BREAK:
        indent(g);
        sb_append(&g->body, "break;\n");
        break;
    case STMT_CONTINUE:
        indent(g);
        sb_append(&g->body, "continue;\n");
        break;
    }
}

// --- Program -------------------------------------------------------------------

// static const EsValue es_k_rate = {ES_NUMBER, 0.20000000000000001, false, "", 0};
static void emit_constant(const Stmt *stmt, StrBuf *out) {
    const ConstValue *v = &stmt->as.constant.folded;
    sb_append(out, "static const EsValue ");
    cg_append_prefixed(out, "es_k_", stmt->as.constant.name.text);
    switch (v->kind) {
    case CONST_NUMBER:
        if (isnan(v->number)) {
            sb_append(out, " = {ES_NUMBER, NAN, false, \"\", 0};\n");
        } else if (isinf(v->number)) {
            sb_appendf(out, " = {ES_NUMBER, %sHUGE_VAL, false, \"\", 0};\n", v->number < 0 ? "-" : "");
        } else {
            sb_appendf(out, " = {ES_NUMBER, %.17g, false, \"\", 0};\n", v->number);
        }
        break;
    case CONST_TEXT:
        sb_append(out, " = {ES_TEXT, 0, false, ");
        cg_append_c_string(out, v->text, v->len);
        sb_appendf(out, ", %zu};\n", v->len);
        break;
    case CONST_YESNO: sb_appendf(out, " = {ES_YESNO, 0, %s, \"\", 0};\n", v->yes ? "true" : "false"); break;
    }
}

// static EsValue es_f_area(int es_line, EsValue es_v_width, EsValue es_v_height)
static void append_signature(StrBuf *out, const Stmt *fn) {
    sb_append(out, "static EsValue ");
    cg_append_prefixed(out, "es_f_", fn->as.function.name.text);
    sb_append(out, "(int es_line");
    for (size_t i = 0; i < fn->as.function.params.len; i++) {
        sb_append(out, ", EsValue ");
        cg_append_name(out, fn->as.function.params.items[i].text);
    }
    sb_append_char(out, ')');
}

// The C function for one EasyScript function: temporaries and locals
// (hoisted), then the body; falling off the end gives back nothing.
static void emit_function(Codegen *g, const Stmt *fn, StrBuf *out) {
    Codegen f = {0};
    f.arena = g->arena;
    f.functions = g->functions;
    f.constants = g->constants;
    f.function = fn;
    f.loop_ids = g->loop_ids;
    sb_init(&f.body, g->arena);
    for (size_t i = 0; i < fn->as.function.body.len; i++) {
        emit_stmt(&f, fn->as.function.body.items[i]);
    }
    g->loop_ids = f.loop_ids;

    append_signature(out, fn);
    sb_append(out, " {\n");
    for (size_t t = 1; t <= f.temps; t++) {
        sb_appendf(out, "    EsValue es_t%zu;\n", t);
    }
    for (size_t i = 0; i < f.locals.len; i++) {
        sb_append(out, "    EsValue ");
        cg_append_name(out, f.locals.items[i]);
        sb_append(out, " = es_nothing();\n");
    }
    for (size_t i = 0; i < fn->as.function.params.len; i++) {  // inputs it never uses
        sb_append(out, "    (void)");
        cg_append_name(out, fn->as.function.params.items[i].text);
        sb_append(out, ";\n");
    }
    for (size_t i = 0; i < f.locals.len; i++) {  // names it makes but never reads
        sb_append(out, "    (void)");
        cg_append_name(out, f.locals.items[i]);
        sb_append(out, ";\n");
    }
    sb_append(out, "    es_enter(es_line);\n");
    sb_append_n(out, f.body.data, f.body.len);
    sb_append(out, "    return es_leave(es_nothing());\n}\n\n");
}

void codegen_c(Arena *arena, const Block *program, StrBuf *out) {
    Codegen g = {0};
    g.arena = arena;
    sb_init(&g.body, arena);
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
    for (size_t i = 0; i < g.globals.len; i++) {
        sb_append(out, "static EsValue ");
        cg_append_name(out, g.globals.items[i]);
        sb_append(out, ";\n");
    }
    for (size_t i = 0; i < g.constants.len; i++) {
        emit_constant(g.constants.items[i], out);
    }
    if (g.functions.len > 0) sb_append_char(out, '\n');
    for (size_t i = 0; i < g.functions.len; i++) {  // prototypes, for calls in any order
        append_signature(out, g.functions.items[i]);
        sb_append(out, ";\n");
    }
    sb_append_char(out, '\n');
    sb_append_n(out, functions.data, functions.len);
    sb_append(out, "int main(void) {\n");
    for (size_t t = 1; t <= g.temps; t++) {
        sb_appendf(out, "    EsValue es_t%zu;\n", t);
    }
    for (size_t i = 0; i < g.constants.len; i++) {  // constants nothing uses (costs nothing)
        sb_append(out, "    (void)");
        cg_append_prefixed(out, "es_k_", g.constants.items[i]->as.constant.name.text);
        sb_append(out, ";\n");
    }
    sb_append(out, "    es_init();\n");
    sb_append_n(out, g.body.data, g.body.len);
    sb_append(out, "    return 0;\n}\n");
}
