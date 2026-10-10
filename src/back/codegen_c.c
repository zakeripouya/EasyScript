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

typedef struct {
    Arena *arena;
    StrBuf body;               // statements of main()
    Vec(const char *) globals; // variable names, in the order they're first made
    size_t temps;              // es_t1 ... es_tN, declared at the top of main()
    size_t depth;              // how many if blocks the current statement is inside
} Codegen;

// Starts a line of main()'s body at the current nesting depth.
static void indent(Codegen *g) {
    sb_append_repeat(&g->body, ' ', 4 * (g->depth + 1));
}

static void emit_expr(Codegen *g, const Expr *expr, StrBuf *out);

// --- Names and literals ------------------------------------------------------

// A variable's C name: "es_v_" plus the name with "_" doubled and "'" as "_q",
// so different EasyScript names can never collide.
static void append_c_name(StrBuf *out, const char *name) {
    sb_append(out, "es_v_");
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

static void declare_global(Codegen *g, const char *name) {
    for (size_t i = 0; i < g->globals.len; i++) {
        if (strcmp(g->globals.items[i], name) == 0) return;
    }
    vec_push(g->arena, &g->globals, name);
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
    case EXPR_NAME: append_c_name(out, expr->as.name); break;
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
    case EXPR_ERROR:
    case EXPR_IT:
    case EXPR_CALL:
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

static void emit_stmt(Codegen *g, const Stmt *stmt) {
    indent(g);
    sb_appendf(&g->body, "/* line %zu */\n", stmt->pos.line);
    switch (stmt->kind) {
    case STMT_LET:
        declare_global(g, stmt->as.assign.name.text);
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
        declare_global(g, stmt->as.ask.answer.text);
        begin_assignment(g, stmt->as.ask.answer.text);
        emit_call1(g, "es_ask", stmt->pos.line, stmt->as.ask.prompt, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: emit_file_write(g, stmt); break;
    case STMT_READ_FILE:
        declare_global(g, stmt->as.read_file.name.text);
        begin_assignment(g, stmt->as.read_file.name.text);
        emit_call1(g, "es_read_file", stmt->pos.line, stmt->as.read_file.path, &g->body);
        sb_append(&g->body, ";\n");
        break;
    case STMT_STOP:
        indent(g);
        sb_append(&g->body, "es_stop();\n");
        break;
    case STMT_IF: emit_if(g, stmt); break;
    }
}

// --- Program -------------------------------------------------------------------

void codegen_c(Arena *arena, const Block *program, StrBuf *out) {
    Codegen g = {0};
    g.arena = arena;
    sb_init(&g.body, arena);
    for (size_t i = 0; i < program->len; i++) {
        emit_stmt(&g, program->items[i]);
    }

    sb_append_n(out, (const char *)es_runtime_source, es_runtime_source_len);
    sb_append(out, "\n/* --- Generated by EasyScript --- */\n\n");
    for (size_t i = 0; i < g.globals.len; i++) {
        sb_append(out, "static EsValue ");
        append_c_name(out, g.globals.items[i]);
        sb_append(out, ";\n");
    }
    sb_append(out, "\nint main(void) {\n");
    for (size_t t = 1; t <= g.temps; t++) {
        sb_appendf(out, "    EsValue es_t%zu;\n", t);
    }
    sb_append(out, "    es_init();\n");
    sb_append_n(out, g.body.data, g.body.len);
    sb_append(out, "    return 0;\n}\n");
}
