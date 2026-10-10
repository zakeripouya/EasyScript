// C code generation. Every EasyScript value is an EsValue (see
// runtime/es_value.h) and every operation is a runtime call that gets the
// source line, so runtime errors can say where they happened.
//
// Memory (docs/memory.md): every expression produces an owned value, and
// runtime calls and EasyScript functions consume their arguments, so nested
// calls need no bookkeeping. Reading a variable retains it; storing goes
// through es_set, which releases the old value after the new one is worked
// out. A block releases the names first made in it when it ends; stop the
// loop, skip this one and give back release the names of every block they
// leave, and main()'s variables are released when the program ends.
//
// main()'s variables are locals of main(), not C globals: their addresses
// never escape, so the C compiler can keep numbers in registers even though
// a release might call free().
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

// Generated from runtime/es_value.h and es_runtime.h at build time by tools/embed.c.
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

// es_set(&name, <value, written by the caller>);
static void begin_assignment(Codegen *g, const char *name) {
    indent(g);
    sb_append(&g->body, "es_set(&");
    cg_append_name(&g->body, name);
    sb_append(&g->body, ", ");
}

static void emit_assign(Codegen *g, const char *name, const Expr *value) {
    begin_assignment(g, name);
    cg_emit_expr(g, value, &g->body);
    sb_append(&g->body, ");\n");
}

static void emit_change(Codegen *g, const Stmt *stmt) {
    const char *name = stmt->as.change.target.text;
    begin_assignment(g, name);
    sb_appendf(&g->body, "%s(%zu, es_retain(", change_function(stmt->as.change.op), stmt->pos.line);
    cg_append_name(&g->body, name);
    sb_append(&g->body, "), ");
    cg_emit_expr(g, stmt->as.change.amount, &g->body);
    sb_append(&g->body, "));\n");
}

// --- Releasing names ---------------------------------------------------------

static void emit_drop(Codegen *g, const char *name) {
    indent(g);
    sb_append(&g->body, "es_drop(&");
    cg_append_name(&g->body, name);
    sb_append(&g->body, ");\n");
}

// Releases the names of scopes[from] and every scope inside it.
static void emit_drop_scopes(Codegen *g, size_t from) {
    for (size_t i = g->scopes.len; i > from; i--) {
        const NameScope *scope = &g->scopes.items[i - 1];
        for (size_t j = 0; j < scope->names.len; j++) emit_drop(g, scope->names.items[j]);
    }
}

static void push_scope(Codegen *g) {
    NameScope scope = {0};
    vec_push(g->arena, &g->scopes, scope);
}

// Releases the innermost scope's names, then leaves it.
static void pop_scope(Codegen *g) {
    emit_drop_scopes(g, g->scopes.len - 1);
    g->scopes.len--;
}

// Leaving a function (its inputs and every name it makes) or the program
// (every variable of main()).
static void emit_drop_all(Codegen *g) {
    if (!g->function) {
        for (size_t i = 0; i < g->globals.len; i++) emit_drop(g, g->globals.items[i]);
        return;
    }
    for (size_t i = 0; i < g->function->as.function.params.len; i++) {
        emit_drop(g, g->function->as.function.params.items[i].text);
    }
    for (size_t i = 0; i < g->locals.len; i++) emit_drop(g, g->locals.items[i]);
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
    push_scope(g);
    for (size_t i = 0; i < block->len; i++) {
        emit_stmt(g, block->items[i]);
    }
    pop_scope(g);
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

// { <setup> for/while (...) { body } <release the loop's number> }
static void emit_loop(Codegen *g, const Stmt *stmt) {
    size_t id = ++g->loop_ids;
    indent(g);
    sb_append(&g->body, "{\n");
    g->depth++;
    push_scope(g);
    if (stmt->as.loop.kind == LOOP_COUNT) cg_declare_name(g, stmt->as.loop.var.text);
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
        sb_append(&g->body, ");\n");
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: emit_file_write(g, stmt); break;
    case STMT_READ_FILE:
        cg_declare_name(g, stmt->as.read_file.name.text);
        begin_assignment(g, stmt->as.read_file.name.text);
        cg_emit_call1(g, "es_read_file", stmt->pos.line, stmt->as.read_file.path, &g->body);
        sb_append(&g->body, ");\n");
        break;
    case STMT_STOP:
        indent(g);
        if (!g->function) {  // the end of main() releases everything
            sb_append(&g->body, "goto es_end;\n");
            g->stops = true;
            break;
        }
        sb_append(&g->body, "{\n");
        g->depth++;
        emit_drop_all(g);
        indent(g);
        sb_append(&g->body, "es_stop();\n");
        g->depth--;
        indent(g);
        sb_append(&g->body, "}\n");
        break;
    case STMT_IF: emit_if(g, stmt); break;
    case STMT_LOOP: emit_loop(g, stmt); break;
    case STMT_FUNCTION: break;  // generated separately, before main()
    case STMT_CONSTANT: break;  // a static initializer, worked out by the compiler
    case STMT_RETURN: {
        // The value is worked out before the function's names are released.
        size_t t = cg_new_temp(g);
        indent(g);
        sb_appendf(&g->body, "es_t%zu = ", t);
        if (stmt->as.returned) {
            cg_emit_expr(g, stmt->as.returned, &g->body);
        } else {
            sb_append(&g->body, "es_nothing()");
        }
        sb_append(&g->body, ";\n");
        emit_drop_all(g);
        indent(g);
        sb_appendf(&g->body, "return es_leave(es_t%zu);\n", t);
        break;
    }
    case STMT_CALL:
        indent(g);
        cg_emit_expr(g, stmt->as.call, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_BREAK:
    case STMT_CONTINUE:
        emit_drop_scopes(g, vec_last(&g->loops).scope);
        indent(g);
        sb_append(&g->body, stmt->kind == STMT_BREAK ? "break;\n" : "continue;\n");
        break;
    }
}

// --- Program -------------------------------------------------------------------

// static const EsValue es_k_rate = {ES_NUMBER, {.number = 0.20000000000000001}};
// Text constants point to a static immortal text object.
static void emit_constant(Codegen *g, const Stmt *stmt, StrBuf *out) {
    const ConstValue *v = &stmt->as.constant.folded;
    sb_append(out, "static const EsValue ");
    cg_append_prefixed(out, "es_k_", stmt->as.constant.name.text);
    switch (v->kind) {
    case CONST_NUMBER:
        if (isnan(v->number)) {
            sb_append(out, " = {ES_NUMBER, {.number = NAN}};\n");
        } else if (isinf(v->number)) {
            sb_appendf(out, " = {ES_NUMBER, {.number = %sHUGE_VAL}};\n", v->number < 0 ? "-" : "");
        } else {
            sb_appendf(out, " = {ES_NUMBER, {.number = %.17g}};\n", v->number);
        }
        break;
    case CONST_TEXT: sb_appendf(out, " = {ES_TEXT, {.text = &es_s%zu}};\n", cg_static_text(g, v->text, v->len)); break;
    case CONST_YESNO: sb_appendf(out, " = {ES_YESNO, {.yes = %s}};\n", v->yes ? "true" : "false"); break;
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
// (hoisted), then the body; falling off the end releases its inputs and
// names and gives back nothing.
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
    emit_drop_all(&f);
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
    sb_append(out, "    es_enter(es_line);\n");
    sb_append_n(out, f.body.data, f.body.len);
    sb_append(out, "    return es_leave(es_nothing());\n}\n\n");
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
    for (size_t i = 0; i < g.globals.len; i++) {
        sb_append(out, "    EsValue ");
        cg_append_name(out, g.globals.items[i]);
        sb_append(out, " = es_nothing();\n");
    }
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
    if (g.stops) sb_append(out, "es_end:\n");
    g.depth = 0;
    g.body.len = 0;
    emit_drop_all(&g);
    sb_append_n(out, g.body.data, g.body.len);
    sb_append(out, "    es_finish(true);\n    return 0;\n}\n");
}
