// C code for names, literals, and expressions. Statements, functions, and
// the program around them are in codegen_c.c.

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "back/codegen_internal.h"

// --- Names and literals ------------------------------------------------------

// A C name: the prefix plus the name with "_" doubled and "'" as "_q", so
// different EasyScript names can never collide.
void cg_append_prefixed(StrBuf *out, const char *prefix, const char *name) {
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
void cg_append_name(StrBuf *out, const char *name) {
    cg_append_prefixed(out, "es_v_", name);
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

static bool in_scope(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->scopes.len; i++) {
        if (contains(g->scopes.items[i].names.items, g->scopes.items[i].names.len, name)) return true;
    }
    return false;
}

// A name made by the program: a global in main(), a hoisted local in a
// function. The innermost block it's first made in releases it at its end.
void cg_declare_name(Codegen *g, const char *name) {
    if (!is_param(g, name) && !in_scope(g, name) && g->scopes.len > 0) {
        vec_push(g->arena, &vec_last(&g->scopes).names, name);
    }
    if (g->function) {
        if (!is_param(g, name) && !contains(g->locals.items, g->locals.len, name)) {
            vec_push(g->arena, &g->locals, name);
        }
    } else if (!contains(g->globals.items, g->globals.len, name)) {
        vec_push(g->arena, &g->globals, name);
    }
}

static bool is_constant(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->constants.len; i++) {
        if (strcmp(g->constants.items[i]->as.constant.name.text, name) == 0) return true;
    }
    return false;
}

static bool is_function(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->functions.len; i++) {
        if (strcmp(g->functions.items[i]->as.function.name.text, name) == 0) return true;
    }
    return false;
}

// A C string literal. Non-ASCII and control bytes use three-digit octal
// escapes; "?" is escaped so "??" can never form a trigraph.
void cg_append_c_string(StrBuf *out, const char *s, size_t len) {
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

// static EsTextObject es_s1 = {{ES_OBJ_TEXT, ES_IMMORTAL}, 6, "Name? "};
size_t cg_static_text(Codegen *g, const char *s, size_t len) {
    size_t id = ++g->texts->count;
    sb_appendf(&g->texts->code, "static EsTextObject es_s%zu = {{ES_OBJ_TEXT, ES_IMMORTAL}, %zu, ", id, len);
    cg_append_c_string(&g->texts->code, s, len);
    sb_append(&g->texts->code, "};\n");
    return id;
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

size_t cg_new_temp(Codegen *g) {
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
    size_t t = cg_new_temp(g);
    sb_appendf(out, "(es_t%zu = ", t);
    cg_emit_expr(g, expr->as.binary.left, out);
    sb_appendf(out, ", %s(es_t%zu) ? es_t%zu : %s(%zu, es_t%zu, ", is_and ? "es_is_no" : "es_is_yes", t, t,
               is_and ? "es_and" : "es_or", expr->pos.line, t);
    cg_emit_expr(g, expr->as.binary.right, out);
    sb_append(out, "))");
}

static void emit_binary(Codegen *g, const Expr *expr, StrBuf *out) {
    BinaryOp op = expr->as.binary.op;
    if (op == BINARY_AND || op == BINARY_OR) {
        emit_short_circuit(g, expr, out);
        return;
    }
    size_t t = cg_new_temp(g);
    sb_appendf(out, "(es_t%zu = ", t);
    cg_emit_expr(g, expr->as.binary.left, out);
    if (op == BINARY_EQUAL || op == BINARY_NOT_EQUAL || op == BINARY_JOIN) {
        const char *fn = op == BINARY_EQUAL ? "es_eq" : op == BINARY_NOT_EQUAL ? "es_ne" : "es_join";
        sb_appendf(out, ", %s(es_t%zu, ", fn, t);
    } else {
        sb_appendf(out, ", %s(%zu, es_t%zu, ", binary_function(op), expr->pos.line, t);
    }
    cg_emit_expr(g, expr->as.binary.right, out);
    sb_append(out, "))");
}

// fn(line, OPERAND) or, with no line, fn(OPERAND).
void cg_emit_call1(Codegen *g, const char *fn, size_t line, const Expr *operand, StrBuf *out) {
    if (line) {
        sb_appendf(out, "%s(%zu, ", fn, line);
    } else {
        sb_appendf(out, "%s(", fn);
    }
    cg_emit_expr(g, operand, out);
    sb_append_char(out, ')');
}

// "it": the number of the innermost count or times loop (the checker made
// sure there is one).
static void emit_it(Codegen *g, StrBuf *out) {
    for (size_t i = g->loops.len; i > 0; i--) {
        const LoopCode *loop = &g->loops.items[i - 1];
        if (loop->stmt->as.loop.kind == LOOP_COUNT) {
            sb_append(out, "es_retain(");
            cg_append_name(out, loop->stmt->as.loop.var.text);
            sb_append_char(out, ')');
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

// (t1 = A, t2 = B, es_f_area(line, t1, t2)): arguments left to right. The
// function owns its arguments and releases them when it ends.
static void emit_call(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t n = expr->as.call.args.len;
    size_t first = g->temps + 1;
    g->temps += n;
    sb_append_char(out, '(');
    for (size_t i = 0; i < n; i++) {
        sb_appendf(out, "es_t%zu = ", first + i);
        cg_emit_expr(g, expr->as.call.args.items[i], out);
        sb_append(out, ", ");
    }
    cg_append_prefixed(out, "es_f_", expr->as.call.name);
    sb_appendf(out, "(%zu", expr->pos.line);
    for (size_t i = 0; i < n; i++) {
        sb_appendf(out, ", es_t%zu", first + i);
    }
    sb_append(out, "))");
}

void cg_emit_expr(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t line = expr->pos.line;
    switch (expr->kind) {
    case EXPR_NUMBER: emit_number(expr, out); break;
    case EXPR_TEXT:
        sb_appendf(out, "es_text(&es_s%zu)", cg_static_text(g, expr->as.text.value, expr->as.text.len));
        break;
    case EXPR_BOOLEAN: sb_appendf(out, "es_yesno(%d)", expr->as.boolean ? 1 : 0); break;
    case EXPR_NOTHING: sb_append(out, "es_nothing()"); break;
    case EXPR_NAME:
        if (is_constant(g, expr->as.name)) {
            cg_append_prefixed(out, "es_k_", expr->as.name);  // immortal: no retain
        } else if (is_function(g, expr->as.name)) {
            cg_append_prefixed(out, "es_f_", expr->as.name);  // a function that takes no values
            sb_appendf(out, "(%zu)", line);
        } else {
            sb_append(out, "es_retain(");  // reading a variable: one more owner
            cg_append_name(out, expr->as.name);
            sb_append_char(out, ')');
        }
        break;
    case EXPR_CALL: emit_call(g, expr, out); break;
    case EXPR_LENGTH: cg_emit_call1(g, "es_length", line, expr->as.operand, out); break;
    case EXPR_FILE_CONTENTS: cg_emit_call1(g, "es_read_file", line, expr->as.operand, out); break;
    case EXPR_UNARY:
        cg_emit_call1(g, expr->as.unary.op == UNARY_NEGATE ? "es_neg" : "es_not", line, expr->as.unary.operand, out);
        break;
    case EXPR_BINARY: emit_binary(g, expr, out); break;
    case EXPR_CONVERT:
        if (expr->as.convert.target == CONVERT_TO_NUMBER) {
            cg_emit_call1(g, "es_as_number", line, expr->as.convert.operand, out);
        } else {
            cg_emit_call1(g, "es_to_text", 0, expr->as.convert.operand, out);
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
