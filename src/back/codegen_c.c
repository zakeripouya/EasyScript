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
#include "back/codegen_c.h"

// Generated from runtime/es_runtime.h at build time by tools/embed.c.
extern const unsigned char es_runtime_source[];
extern const size_t es_runtime_source_len;

// A loop being generated, for "it".
typedef struct {
    const Stmt *stmt;
    size_t id;  // es_cN / es_iN / es_nN
} LoopCode;

typedef struct {
    Arena *arena;
    StrBuf body;               // statements of main()
    Vec(const char *) globals; // variable names, in the order they're first made
    size_t temps;              // es_t1 ... es_tN, declared at the top of main()
    size_t depth;              // how many blocks the current statement is inside
    Vec(LoopCode) loops;       // loops the current statement is inside, innermost last
    size_t loop_ids;
    Vec(const Stmt *) functions; // every top-level STMT_FUNCTION
    const Stmt *function;        // the function being generated, or NULL for main()
    Vec(const char *) locals;    // names made in that function, hoisted to its top
} Codegen;

// Starts a line of main()'s body at the current nesting depth.
static void indent(Codegen *g) {
    sb_append_repeat(&g->body, ' ', 4 * (g->depth + 1));
}

static void emit_expr(Codegen *g, const Expr *expr, StrBuf *out);

// --- Names and literals ------------------------------------------------------

// A C name: the prefix plus the name with "_" doubled and "'" as "_q", so
// different EasyScript names can never collide.
static void append_prefixed(StrBuf *out, const char *prefix, const char *name) {
    sb_append(out, prefix);
    for (const char *c = name; *c; c++) {
        if (*c == '_') {
            sb_append(out, "__");
        } else if (*c == '\'') {
            sb_append(out, "_q");
        } else {
            sb_append_char(out, *c);
        }
    }
}

// A variable: "es_v_total". A function: "es_f_area".
static void append_c_name(StrBuf *out, const char *name) {
    append_prefixed(out, "es_v_", name);
}

static bool contains(const char *const *items, size_t len, const char *name) {
    for (size_t i = 0; i < len; i++) {
        if (strcmp(items[i], name) == 0) return true;
    }
    return false;
}

static bool is_param(const Codegen *g, const char *name) {
    if (!g->function) return false;
    for (size_t i = 0; i < g->function->as.function.params.len; i++) {
        if (strcmp(g->function->as.function.params.items[i].text, name) == 0) return true;
    }
    return false;
}

// A name made by the program: a global in main(), a hoisted local in a function.
static void declare_name(Codegen *g, const char *name) {
    if (g->function) {
        if (!is_param(g, name) && !contains(g->locals.items, g->locals.len, name)) {
            vec_push(g->arena, &g->locals, name);
        }
    } else if (!contains(g->globals.items, g->globals.len, name)) {
        vec_push(g->arena, &g->globals, name);
    }
}

static bool is_function(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->functions.len; i++) {
        if (strcmp(g->functions.items[i]->as.function.name.text, name) == 0) return true;
    }
    return false;
}

// A C string literal. Non-ASCII and control bytes use three-digit octal
// escapes; "?" is escaped so "??" can never form a trigraph.
static void append_c_string(StrBuf *out, const char *s, size_t len) {
    sb_append_char(out, '"');
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\' || c == '?') {
            sb_append_char(out, '\\');
            sb_append_char(out, (char)c);
        } else if (c == '\n') {
            sb_append(out, "\\n");
        } else if (c == '\t') {
            sb_append(out, "\\t");
        } else if (c >= 0x20 && c < 0x7F) {
            sb_append_char(out, (char)c);
        } else {
            sb_appendf(out, "\\%03o", c);
        }
    }
    sb_append_char(out, '"');
}

static void emit_number(const Expr *expr, StrBuf *out) {
    double value = strtod(expr->as.number.text, NULL);
    if (isinf(value)) {
        sb_append(out, "es_num(HUGE_VAL)");
    } else {
        sb_appendf(out, "es_num(%.17g)", value);
    }
}

// --- Expressions ---------------------------------------------------------------

static size_t new_temp(Codegen *g) {
    return ++g->temps;
}

static const char *binary_function(BinaryOp op) {
    switch (op) {
    case BINARY_ADD: return "es_add";
    case BINARY_SUBTRACT: return "es_sub";
    case BINARY_MULTIPLY: return "es_mul";
    case BINARY_DIVIDE: return "es_div";
    case BINARY_MODULO: return "es_mod";
    case BINARY_LESS: return "es_lt";
    case BINARY_LESS_EQUAL: return "es_le";
    case BINARY_GREATER: return "es_gt";
    case BINARY_GREATER_EQUAL: return "es_ge";
    default: return NULL;
    }
}

// "and" stops at a "no" on the left and "or" at a "yes", without evaluating
// the right side: (t = LEFT, es_is_no(t) ? t : es_and(line, t, RIGHT)).
static void emit_short_circuit(Codegen *g, const Expr *expr, StrBuf *out) {
    bool is_and = expr->as.binary.op == BINARY_AND;
    size_t t = new_temp(g);
    sb_appendf(out, "(es_t%zu = ", t);
    emit_expr(g, expr->as.binary.left, out);
    sb_appendf(out, ", %s(es_t%zu) ? es_t%zu : %s(%zu, es_t%zu, ", is_and ? "es_is_no" : "es_is_yes", t, t,
               is_and ? "es_and" : "es_or", expr->pos.line, t);
    emit_expr(g, expr->as.binary.right, out);
    sb_append(out, "))");
}

static void emit_binary(Codegen *g, const Expr *expr, StrBuf *out) {
    BinaryOp op = expr->as.binary.op;
    if (op == BINARY_AND || op == BINARY_OR) {
        emit_short_circuit(g, expr, out);
        return;
    }
    size_t t = new_temp(g);
    sb_appendf(out, "(es_t%zu = ", t);
    emit_expr(g, expr->as.binary.left, out);
    if (op == BINARY_EQUAL || op == BINARY_NOT_EQUAL || op == BINARY_JOIN) {
        const char *fn = op == BINARY_EQUAL ? "es_eq" : op == BINARY_NOT_EQUAL ? "es_ne" : "es_join";
        sb_appendf(out, ", %s(es_t%zu, ", fn, t);
    } else {
        sb_appendf(out, ", %s(%zu, es_t%zu, ", binary_function(op), expr->pos.line, t);
    }
    emit_expr(g, expr->as.binary.right, out);
    sb_append(out, "))");
}

// fn(line, OPERAND) or, with no line, fn(OPERAND).
static void emit_call1(Codegen *g, const char *fn, size_t line, const Expr *operand, StrBuf *out) {
    if (line) {
        sb_appendf(out, "%s(%zu, ", fn, line);
    } else {
        sb_appendf(out, "%s(", fn);
    }
    emit_expr(g, operand, out);
    sb_append_char(out, ')');
}

// "it": the number of the innermost count or times loop (the checker made
// sure there is one).
static void emit_it(Codegen *g, StrBuf *out) {
    for (size_t i = g->loops.len; i > 0; i--) {
        const LoopCode *loop = &g->loops.items[i - 1];
        if (loop->stmt->as.loop.kind == LOOP_COUNT) {
            append_c_name(out, loop->stmt->as.loop.var.text);
            return;
        }
        if (loop->stmt->as.loop.kind == LOOP_TIMES) {
            sb_appendf(out, "es_num((double)es_i%zu)", loop->id);
            return;
        }
    }
    assert(!"\"it\" outside a counting loop reached codegen");
    sb_append(out, "es_nothing()");
}

// (t1 = A, t2 = B, es_f_area(line, t1, t2)): arguments left to right.
static void emit_call(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t n = expr->as.call.args.len;
    size_t first = g->temps + 1;
    g->temps += n;
    sb_append_char(out, '(');
    for (size_t i = 0; i < n; i++) {
        sb_appendf(out, "es_t%zu = ", first + i);
        emit_expr(g, expr->as.call.args.items[i], out);
        sb_append(out, ", ");
    }
    append_prefixed(out, "es_f_", expr->as.call.name);
    sb_appendf(out, "(%zu", expr->pos.line);
    for (size_t i = 0; i < n; i++) {
        sb_appendf(out, ", es_t%zu", first + i);
    }
    sb_append(out, "))");
}

static void emit_expr(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t line = expr->pos.line;
    switch (expr->kind) {
    case EXPR_NUMBER: emit_number(expr, out); break;
    case EXPR_TEXT:
        sb_append(out, "es_text_lit(");
        append_c_string(out, expr->as.text.value, expr->as.text.len);
        sb_appendf(out, ", %zu)", expr->as.text.len);
        break;
    case EXPR_BOOLEAN: sb_appendf(out, "es_yesno(%d)", expr->as.boolean ? 1 : 0); break;
    case EXPR_NOTHING: sb_append(out, "es_nothing()"); break;
    case EXPR_NAME:
        if (is_function(g, expr->as.name)) {
            append_prefixed(out, "es_f_", expr->as.name);  // a function that takes no values
            sb_appendf(out, "(%zu)", line);
        } else {
            append_c_name(out, expr->as.name);
        }
        break;
    case EXPR_CALL: emit_call(g, expr, out); break;
    case EXPR_LENGTH: emit_call1(g, "es_length", line, expr->as.operand, out); break;
    case EXPR_FILE_CONTENTS: emit_call1(g, "es_read_file", line, expr->as.operand, out); break;
    case EXPR_UNARY:
        emit_call1(g, expr->as.unary.op == UNARY_NEGATE ? "es_neg" : "es_not", line, expr->as.unary.operand, out);
        break;
    case EXPR_BINARY: emit_binary(g, expr, out); break;
    case EXPR_CONVERT:
        if (expr->as.convert.target == CONVERT_TO_NUMBER) {
            emit_call1(g, "es_as_number", line, expr->as.convert.operand, out);
        } else {
            emit_call1(g, "es_to_text", 0, expr->as.convert.operand, out);
        }
        break;
    case EXPR_IT: emit_it(g, out); break;
    case EXPR_ERROR:
        // The checker rejects these before codegen runs.
        assert(!"unchecked expression reached codegen");
        sb_append(out, "es_nothing()");
        break;
    }
}

// --- Statements ----------------------------------------------------------------

static const char *change_function(ChangeOp op) {
    static const char *const names[] = {"es_add", "es_sub", "es_mul", "es_div"};
    return names[op];
}

// name = <rest of the line, written by the caller>
static void begin_assignment(Codegen *g, const char *name) {
    indent(g);
    append_c_name(&g->body, name);
    sb_append(&g->body, " = ");
}

static void emit_assign(Codegen *g, const char *name, const Expr *value) {
    begin_assignment(g, name);
    emit_expr(g, value, &g->body);
    sb_append(&g->body, ";\n");
}

static void emit_change(Codegen *g, const Stmt *stmt) {
    const char *name = stmt->as.change.target.text;
    begin_assignment(g, name);
    sb_appendf(&g->body, "%s(%zu, ", change_function(stmt->as.change.op), stmt->pos.line);
    append_c_name(&g->body, name);
    sb_append(&g->body, ", ");
    emit_expr(g, stmt->as.change.amount, &g->body);
    sb_append(&g->body, ");\n");
}

static void emit_file_write(Codegen *g, const Stmt *stmt) {
    size_t t = new_temp(g);
    indent(g);
    sb_appendf(&g->body, "es_t%zu = ", t);
    emit_expr(g, stmt->as.file_write.text, &g->body);
    sb_append(&g->body, ";\n");
    indent(g);
    sb_appendf(&g->body, "es_write_file(%zu, es_t%zu, ", stmt->pos.line, t);
    emit_expr(g, stmt->as.file_write.path, &g->body);
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
            emit_expr(g, branch->condition, &g->body);
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
    size_t t = new_temp(g);
    indent(g);
    sb_appendf(&g->body, "es_t%zu = ", t);
    emit_expr(g, expr, &g->body);
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
        append_c_name(&g->body, stmt->as.loop.var.text);
        sb_appendf(&g->body, "); es_i%zu++) {\n", id);
        break;
    }
    case LOOP_TIMES:
        indent(g);
        sb_appendf(&g->body, "long long es_n%zu = es_times(%zu, ", id, line);
        emit_expr(g, stmt->as.loop.times, &g->body);
        sb_append(&g->body, ");\n");
        indent(g);
        sb_appendf(&g->body, "for (long long es_i%zu = 1; es_i%zu <= es_n%zu; es_i%zu++) {\n", id, id, id, id);
        break;
    case LOOP_WHILE:
    case LOOP_UNTIL:
        indent(g);
        sb_appendf(&g->body, "while (%ses_loop_condition(%zu, ", stmt->as.loop.kind == LOOP_UNTIL ? "!" : "", line);
        emit_expr(g, stmt->as.loop.condition, &g->body);
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
    if (stmt->as.loop.kind == LOOP_COUNT) declare_name(g, stmt->as.loop.var.text);
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
        declare_name(g, stmt->as.assign.name.text);
        emit_assign(g, stmt->as.assign.name.text, stmt->as.assign.value);
        break;
    case STMT_SET: emit_assign(g, stmt->as.assign.name.text, stmt->as.assign.value); break;
    case STMT_CHANGE: emit_change(g, stmt); break;
    case STMT_SAY:
        indent(g);
        sb_append(&g->body, "es_say(");
        emit_expr(g, stmt->as.value, &g->body);
        sb_append(&g->body, ");\n");
        break;
    case STMT_ASK:
        declare_name(g, stmt->as.ask.answer.text);
        begin_assignment(g, stmt->as.ask.answer.text);
        emit_call1(g, "es_ask", stmt->pos.line, stmt->as.ask.prompt, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: emit_file_write(g, stmt); break;
    case STMT_READ_FILE:
        declare_name(g, stmt->as.read_file.name.text);
        begin_assignment(g, stmt->as.read_file.name.text);
        emit_call1(g, "es_read_file", stmt->pos.line, stmt->as.read_file.path, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_STOP:
        indent(g);
        sb_append(&g->body, "es_stop();\n");
        break;
    case STMT_IF: emit_if(g, stmt); break;
    case STMT_LOOP: emit_loop(g, stmt); break;
    case STMT_FUNCTION: break;  // generated separately, before main()
    case STMT_RETURN:
        indent(g);
        sb_append(&g->body, "return es_leave(");
        if (stmt->as.returned) {
            emit_expr(g, stmt->as.returned, &g->body);
        } else {
            sb_append(&g->body, "es_nothing()");
        }
        sb_append(&g->body, ");\n");
        break;
    case STMT_CALL:
        indent(g);
        emit_expr(g, stmt->as.call, &g->body);
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

// static EsValue es_f_area(int es_line, EsValue es_v_width, EsValue es_v_height)
static void append_signature(StrBuf *out, const Stmt *fn) {
    sb_append(out, "static EsValue ");
    append_prefixed(out, "es_f_", fn->as.function.name.text);
    sb_append(out, "(int es_line");
    for (size_t i = 0; i < fn->as.function.params.len; i++) {
        sb_append(out, ", EsValue ");
        append_c_name(out, fn->as.function.params.items[i].text);
    }
    sb_append_char(out, ')');
}

// The C function for one EasyScript function: temporaries and locals
// (hoisted), then the body; falling off the end gives back nothing.
static void emit_function(Codegen *g, const Stmt *fn, StrBuf *out) {
    Codegen f = {0};
    f.arena = g->arena;
    f.functions = g->functions;
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
        append_c_name(out, f.locals.items[i]);
        sb_append(out, " = es_nothing();\n");
    }
    for (size_t i = 0; i < fn->as.function.params.len; i++) {  // inputs it never uses
        sb_append(out, "    (void)");
        append_c_name(out, fn->as.function.params.items[i].text);
        sb_append(out, ";\n");
    }
    for (size_t i = 0; i < f.locals.len; i++) {  // names it makes but never reads
        sb_append(out, "    (void)");
        append_c_name(out, f.locals.items[i]);
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
        append_c_name(out, g.globals.items[i]);
        sb_append(out, ";\n");
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
    sb_append(out, "    es_init();\n");
    sb_append_n(out, g.body.data, g.body.len);
    sb_append(out, "    return 0;\n}\n");
}
