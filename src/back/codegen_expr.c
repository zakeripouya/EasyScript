// C code for names, literals, and expressions. Statements, functions, and
// the program around them are in codegen_c.c.
//
// The checker has decided every kind, so a number is a C double, yes/no a C
// bool, and text an EsText * (reference counted, see docs/memory.md). Number
// and yes/no operations are plain C (+, <, &&, ...) or small inline runtime
// helpers (es_div checks for zero; es_close compares with a tolerance).

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

const char *cg_ctype(Type type) {
    switch (type) {
    case TYPE_NUMBER: return "double";
    case TYPE_YESNO: return "bool";
    case TYPE_TEXT: return "EsText *";
    case TYPE_NOTHING: return "void";
    default: assert(!"a value whose kind wasn't decided reached codegen"); return "void";
    }
}

// A variable: "es_vn_total" (number), "es_vt_name" (text), "es_vb_done" (yes/no).
void cg_append_var(StrBuf *out, const char *name, Type type) {
    cg_append_prefixed(out, type == TYPE_TEXT ? "es_vt_" : type == TYPE_YESNO ? "es_vb_" : "es_vn_", name);
}

static bool is_param(const Codegen *g, const char *name) {
    if (!g->function) return false;
    for (size_t i = 0; i < g->function->as.function.params.len; i++) {
        if (strcmp(g->function->as.function.params.items[i].text, name) == 0) return true;
    }
    return false;
}

static bool has_var(const VarCode *items, size_t len, const char *name, Type type) {
    for (size_t i = 0; i < len; i++) {
        if (items[i].type == type && strcmp(items[i].name, name) == 0) return true;
    }
    return false;
}

static bool in_scope(const Codegen *g, const char *name, Type type) {
    for (size_t i = 0; i < g->scopes.len; i++) {
        if (has_var(g->scopes.items[i].names.items, g->scopes.items[i].names.len, name, type)) return true;
    }
    return false;
}

void cg_declare_name(Codegen *g, const char *name, Type type) {
    if (is_param(g, name)) return;
    VarCode var = {name, type};
    if (type == TYPE_TEXT && !in_scope(g, name, type) && g->scopes.len > 0) {
        vec_push(g->arena, &vec_last(&g->scopes).names, var);
    }
    if (!has_var(g->vars.items, g->vars.len, name, type)) vec_push(g->arena, &g->vars, var);
}

static bool is_constant(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->constants.len; i++) {
        if (strcmp(g->constants.items[i]->as.constant.name.text, name) == 0) return true;
    }
    return false;
}

const Stmt *cg_find_function(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->functions.len; i++) {
        if (strcmp(g->functions.items[i]->as.function.name.text, name) == 0) return g->functions.items[i];
    }
    return NULL;
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

// static EsText es_s1 = {{ES_OBJ_TEXT, ES_IMMORTAL}, 6, "Name? "};
size_t cg_static_text(Codegen *g, const char *s, size_t len) {
    size_t id = ++g->texts->count;
    sb_appendf(&g->texts->code, "static EsText es_s%zu = {{ES_OBJ_TEXT, ES_IMMORTAL}, %zu, ", id, len);
    cg_append_c_string(&g->texts->code, s, len);
    sb_append(&g->texts->code, "};\n");
    return id;
}

// A double literal: always with a "." or exponent, so C never does integer
// arithmetic (1000000 * 1000000 would overflow an int).
static void emit_number(const Expr *expr, StrBuf *out) {
    double value = strtod(expr->as.number.text, NULL);
    if (isinf(value)) {
        sb_append(out, "HUGE_VAL");
        return;
    }
    size_t start = out->len;
    sb_appendf(out, "%.17g", value);
    if (!strpbrk(out->data + start, ".e")) sb_append(out, ".0");
}

// --- Order and effects ---------------------------------------------------------

size_t cg_new_temp(Codegen *g, Type type) {
    vec_push(g->arena, &g->temps, type);
    return g->temps.len;
}

bool cg_has_effects(const Codegen *g, const Expr *expr) {
    switch (expr->kind) {
    case EXPR_CALL:
    case EXPR_FILE_CONTENTS: return true;
    case EXPR_NAME: return cg_find_function(g, expr->as.name) != NULL;
    case EXPR_LENGTH: return cg_has_effects(g, expr->as.operand);
    case EXPR_UNARY: return cg_has_effects(g, expr->as.unary.operand);
    case EXPR_CONVERT:
        return (expr->as.convert.target == CONVERT_TO_NUMBER && expr->as.convert.operand->type == TYPE_TEXT) ||
               cg_has_effects(g, expr->as.convert.operand);
    case EXPR_BINARY:
        return expr->as.binary.op == BINARY_DIVIDE || expr->as.binary.op == BINARY_MODULO ||
               cg_has_effects(g, expr->as.binary.left) || cg_has_effects(g, expr->as.binary.right);
    default: return false;
    }
}

// The C code for one operand, as text or as its own kind.
static const char *operand(Codegen *g, const Expr *expr, bool as_text) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    if (as_text) {
        cg_emit_text(g, expr, &sb);
    } else {
        cg_emit_expr(g, expr, &sb);
    }
    return sb.data;
}

// Both operands of a binary operation, left to right: when both have effects
// the left one goes into a temporary first, "(es_tN = LEFT, ...)", and
// *close is set to the ")" that ends it.
static void operands(Codegen *g, const Expr *left, const Expr *right, bool as_text, StrBuf *out, const char **l,
                     const char **r, const char **close) {
    *l = operand(g, left, as_text);
    *r = operand(g, right, as_text);
    *close = "";
    if (!cg_has_effects(g, left) || !cg_has_effects(g, right)) return;
    size_t t = cg_new_temp(g, as_text ? TYPE_TEXT : left->type);
    sb_appendf(out, "(es_t%zu = %s, ", t, *l);
    *l = arena_sprintf(g->arena, "es_t%zu", t);
    *close = ")";
}

// --- Expressions ---------------------------------------------------------------

static const char *arithmetic_operator(BinaryOp op) {
    switch (op) {
    case BINARY_ADD: return "+";
    case BINARY_SUBTRACT: return "-";
    case BINARY_MULTIPLY: return "*";
    default: return NULL;
    }
}

static const char *order_operator(BinaryOp op) {
    switch (op) {
    case BINARY_LESS: return "<";
    case BINARY_LESS_EQUAL: return "<=";
    case BINARY_GREATER: return ">";
    default: return ">=";
    }
}

static void emit_compare(Codegen *g, const Expr *expr, StrBuf *out) {
    BinaryOp op = expr->as.binary.op;
    Type type = expr->as.binary.left->type;
    const char *l, *r, *close;
    operands(g, expr->as.binary.left, expr->as.binary.right, false, out, &l, &r, &close);
    bool equality = op == BINARY_EQUAL || op == BINARY_NOT_EQUAL;
    const char *negate = op == BINARY_NOT_EQUAL ? "!" : "";
    if (equality && type == TYPE_YESNO) {
        sb_appendf(out, "(%s %s %s)", l, op == BINARY_EQUAL ? "==" : "!=", r);
    } else if (equality) {
        sb_appendf(out, "%s%s(%s, %s)", negate, type == TYPE_TEXT ? "es_text_same" : "es_close", l, r);
    } else {
        sb_appendf(out, "(%s(%s, %s) %s 0)", type == TYPE_TEXT ? "es_text_order" : "es_number_order", l, r,
                   order_operator(op));
    }
    sb_append(out, close);
}

static void emit_binary(Codegen *g, const Expr *expr, StrBuf *out) {
    BinaryOp op = expr->as.binary.op;
    const Expr *left = expr->as.binary.left, *right = expr->as.binary.right;
    const char *l, *r, *close;
    switch (op) {
    case BINARY_AND:
    case BINARY_OR:
        if (expr->type == TYPE_YESNO) {  // && and || work left to right and stop early, as "and"/"or" do
            sb_appendf(out, "(%s %s %s)", operand(g, left, false), op == BINARY_AND ? "&&" : "||",
                       operand(g, right, false));
            return;
        }
        // "and" with text joins: fall through
    case BINARY_JOIN:
        operands(g, left, right, true, out, &l, &r, &close);
        sb_appendf(out, "es_join(%s, %s)%s", l, r, close);
        return;
    case BINARY_DIVIDE:
    case BINARY_MODULO:
        operands(g, left, right, false, out, &l, &r, &close);
        sb_appendf(out, "%s(%zu, %s, %s)%s", op == BINARY_DIVIDE ? "es_div" : "es_mod", expr->pos.line, l, r, close);
        return;
    case BINARY_ADD:
    case BINARY_SUBTRACT:
    case BINARY_MULTIPLY:
        operands(g, left, right, false, out, &l, &r, &close);
        sb_appendf(out, "(%s %s %s)%s", l, arithmetic_operator(op), r, close);
        return;
    default: emit_compare(g, expr, out); return;
    }
}

// "it": the number of the innermost count or times loop (the checker made
// sure there is one).
static void emit_it(Codegen *g, StrBuf *out) {
    for (size_t i = g->loops.len; i > 0; i--) {
        const LoopCode *loop = &g->loops.items[i - 1];
        if (loop->stmt->as.loop.kind == LOOP_COUNT) {
            cg_append_var(out, loop->stmt->as.loop.var.text, TYPE_NUMBER);
            return;
        }
        if (loop->stmt->as.loop.kind == LOOP_TIMES) {
            sb_appendf(out, "((double)es_i%zu)", loop->id);
            return;
        }
    }
    assert(!"\"it\" outside a counting loop reached codegen");
}

// es_f_area(line, A, B). When two or more arguments have effects, they go
// into temporaries first, left to right: (t1 = A, t2 = B, es_f_area(line, t1, t2)).
// The function owns its text arguments and releases them when it ends.
static void emit_call(Codegen *g, const char *name, Expr *const *args, size_t n, size_t line, StrBuf *out) {
    size_t effects = 0;
    for (size_t i = 0; i < n; i++) effects += cg_has_effects(g, args[i]);
    const char **values = arena_alloc(g->arena, (n + 1) * sizeof(char *));
    if (effects >= 2) sb_append_char(out, '(');
    for (size_t i = 0; i < n; i++) {
        values[i] = operand(g, args[i], false);
        if (effects < 2) continue;
        size_t t = cg_new_temp(g, args[i]->type);
        sb_appendf(out, "es_t%zu = %s, ", t, values[i]);
        values[i] = arena_sprintf(g->arena, "es_t%zu", t);
    }
    cg_append_prefixed(out, "es_f_", name);
    sb_appendf(out, "(%zu", line);
    for (size_t i = 0; i < n; i++) sb_appendf(out, ", %s", values[i]);
    sb_append(out, effects >= 2 ? "))" : ")");
}

static void emit_name(Codegen *g, const Expr *expr, StrBuf *out) {
    const char *name = expr->as.name;
    if (is_constant(g, name)) {
        cg_append_prefixed(out, "es_k_", name);  // text constants are immortal: no retain
    } else if (cg_find_function(g, name)) {
        emit_call(g, name, NULL, 0, expr->pos.line, out);  // a function that takes no values
    } else if (expr->type == TYPE_TEXT) {
        sb_append(out, "es_retain(");  // reading a text variable: one more owner
        cg_append_var(out, name, TYPE_TEXT);
        sb_append_char(out, ')');
    } else {
        cg_append_var(out, name, expr->type);
    }
}

static void emit_convert(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *operand_expr = expr->as.convert.operand;
    if (expr->as.convert.target == CONVERT_TO_TEXT) {
        cg_emit_text(g, operand_expr, out);
    } else if (operand_expr->type == TYPE_TEXT) {
        sb_appendf(out, "es_text_number(%zu, %s)", expr->pos.line, operand(g, operand_expr, false));
    } else {
        cg_emit_expr(g, operand_expr, out);
    }
}

void cg_emit_expr(Codegen *g, const Expr *expr, StrBuf *out) {
    switch (expr->kind) {
    case EXPR_NUMBER: emit_number(expr, out); break;
    case EXPR_TEXT: sb_appendf(out, "&es_s%zu", cg_static_text(g, expr->as.text.value, expr->as.text.len)); break;
    case EXPR_BOOLEAN: sb_append(out, expr->as.boolean ? "true" : "false"); break;
    case EXPR_NAME: emit_name(g, expr, out); break;
    case EXPR_CALL:
        emit_call(g, expr->as.call.name, expr->as.call.args.items, expr->as.call.args.len, expr->pos.line, out);
        break;
    case EXPR_LENGTH: sb_appendf(out, "es_length(%s)", operand(g, expr->as.operand, false)); break;
    case EXPR_FILE_CONTENTS:
        sb_appendf(out, "es_read_file(%zu, %s)", expr->pos.line, operand(g, expr->as.operand, false));
        break;
    case EXPR_UNARY:
        sb_appendf(out, "(%s%s)", expr->as.unary.op == UNARY_NEGATE ? "-" : "!",
                   operand(g, expr->as.unary.operand, false));
        break;
    case EXPR_BINARY: emit_binary(g, expr, out); break;
    case EXPR_CONVERT: emit_convert(g, expr, out); break;
    case EXPR_IT: emit_it(g, out); break;
    case EXPR_NOTHING:
    case EXPR_ERROR:
        // The checker rejects these before codegen runs.
        assert(!"unchecked expression reached codegen");
        break;
    }
}

void cg_emit_text(Codegen *g, const Expr *expr, StrBuf *out) {
    switch (expr->type) {
    case TYPE_TEXT: cg_emit_expr(g, expr, out); break;
    case TYPE_NUMBER: sb_appendf(out, "es_number_text(%s)", operand(g, expr, false)); break;
    case TYPE_YESNO: sb_appendf(out, "es_yesno_text(%s)", operand(g, expr, false)); break;
    default: assert(!"a value without a kind reached codegen"); break;
    }
}
