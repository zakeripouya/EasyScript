// C code for names, literals, and expressions. Statements, functions, and
// the program around them are in codegen_c.c.
//
// The checker has decided every kind, so a number is a C double, yes/no a C
// bool, text an EsText * and a list an EsList * (both reference counted, see
// docs/memory.md). Number and yes/no operations are plain C (+, <, &&, ...)
// or small inline runtime helpers (es_div checks for zero; es_close compares
// with a tolerance). List values are in codegen_list.c.

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
    if (type_is_list(type)) return "EsList *";
    switch (type) {
    case TYPE_NUMBER: return "double";
    case TYPE_YESNO: return "bool";
    case TYPE_TEXT: return "EsText *";
    case TYPE_NOTHING: return "void";
    default: assert(!"a value whose kind wasn't decided reached codegen"); return "void";
    }
}

// A variable: "es_vn_total" (number), "es_vt_name" (text), "es_vb_done"
// (yes/no), "es_vl1n_scores" (a list of numbers), "es_vl2t_rows" (a list of
// lists of text).
void cg_append_var(StrBuf *out, const char *name, Type type) {
    char base = type_base(type) == TYPE_TEXT ? 't' : type_base(type) == TYPE_YESNO ? 'b' : 'n';
    if (type_is_list(type)) {
        sb_appendf(out, "es_vl%u%c", type / TYPE_LIST, base);
        cg_append_prefixed(out, "_", name);
        return;
    }
    sb_appendf(out, "es_v%c", base);
    cg_append_prefixed(out, "_", name);
}

const char *cg_retain(Type type) {
    return type_is_list(type) ? "es_list_retain" : "es_retain";
}

const char *cg_release(Type type) {
    return type_is_list(type) ? "es_list_release" : "es_release";
}

const char *cg_set(Type type) {
    return type_is_list(type) ? "es_list_set" : "es_set";
}

const char *cg_drop(Type type) {
    return type_is_list(type) ? "es_list_drop" : "es_drop";
}

const char *cg_empty(Type type) {
    if (type_is_list(type)) return "&es_no_list";
    return type == TYPE_TEXT ? "&es_empty_text" : type == TYPE_YESNO ? "false" : "0";
}

void cg_emit_var(StrBuf *out, const char *name, Type type) {
    if (!type_counted(type)) {
        cg_append_var(out, name, type);
        return;
    }
    sb_appendf(out, "%s(", cg_retain(type));  // reading text or a list: one more owner
    cg_append_var(out, name, type);
    sb_append_char(out, ')');
}

static bool is_param(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->captured.len; i++) {
        if (strcmp(g->captured.items[i].name, name) == 0) return true;
    }
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
    if (type_counted(type) && !in_scope(g, name, type) && g->scopes.len > 0) {
        vec_push(g->arena, &vec_last(&g->scopes).names, var);
    }
    if (!has_var(g->vars.items, g->vars.len, name, type)) vec_push(g->arena, &g->vars, var);
}

bool cg_is_constant(const Codegen *g, const char *name) {
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

// Reading what's in a list counts as an effect too: a call could change the
// list, so a call next to it has to keep its place in the order.
bool cg_has_effects(const Codegen *g, const Expr *expr) {
    switch (expr->kind) {
    case EXPR_CALL:
    case EXPR_FILE_CONTENTS:
    case EXPR_ITEM:
    case EXPR_LIST_OP:
    case EXPR_EVERY:
    case EXPR_ONLY:
    case EXPR_SLICE: return true;
    case EXPR_LIST:
        for (size_t i = 0; i < expr->as.items.len; i++) {
            if (cg_has_effects(g, expr->as.items.items[i])) return true;
        }
        return false;
    case EXPR_NAME: return cg_find_function(g, expr->as.name) != NULL;
    case EXPR_LENGTH: return type_is_list(expr->as.operand->type) || cg_has_effects(g, expr->as.operand);
    case EXPR_UNARY: return cg_has_effects(g, expr->as.unary.operand);
    case EXPR_CONVERT:
        return (expr->as.convert.target == CONVERT_TO_NUMBER && expr->as.convert.operand->type == TYPE_TEXT) ||
               cg_has_effects(g, expr->as.convert.operand);
    case EXPR_BINARY:
        return expr->as.binary.op == BINARY_DIVIDE || expr->as.binary.op == BINARY_MODULO ||
               expr->as.binary.op == BINARY_CONTAINS || expr->as.binary.op == BINARY_NOT_CONTAINS ||
               type_is_list(expr->as.binary.left->type) || cg_has_effects(g, expr->as.binary.left) ||
               cg_has_effects(g, expr->as.binary.right);
    default: return false;
    }
}

const char *cg_operand(Codegen *g, const Expr *expr, bool as_text) {
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
    *l = cg_operand(g, left, as_text);
    *r = cg_operand(g, right, as_text);
    *close = "";
    // Turning a list into text reads it, so it keeps its place like an effect.
    bool left_effects = cg_has_effects(g, left) || (as_text && type_is_list(left->type));
    if (!left_effects || !cg_has_effects(g, right)) return;
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
        const char *same = type_is_list(type) ? "es_list_same" : type == TYPE_TEXT ? "es_text_same" : "es_close";
        sb_appendf(out, "%s%s(%s, %s)", negate, same, l, r);
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
            sb_appendf(out, "(%s %s %s)", cg_operand(g, left, false), op == BINARY_AND ? "&&" : "||",
                       cg_operand(g, right, false));
            return;
        }
        // "and" with text joins, like "followed by":
        // fall through
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
    case BINARY_CONTAINS:
    case BINARY_NOT_CONTAINS: cg_emit_contains(g, expr, out); return;
    default: emit_compare(g, expr, out); return;
    }
}

// "it": the number of the innermost count or times loop, or the item of a
// for-each loop or helper (the checker made sure there is one).
static void emit_it(Codegen *g, StrBuf *out) {
    for (size_t i = g->loops.len; i > 0; i--) {
        const LoopCode *loop = &g->loops.items[i - 1];
        if (loop->item) {
            cg_emit_var(out, loop->item, loop->item_type);
            return;
        }
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
bool cg_ordered(Codegen *g, Expr *const *exprs, size_t n, const char **values, StrBuf *out) {
    size_t effects = 0;
    for (size_t i = 0; i < n; i++) effects += exprs[i] && cg_has_effects(g, exprs[i]);
    if (effects >= 2) sb_append_char(out, '(');
    for (size_t i = 0; i < n; i++) {
        if (!exprs[i]) continue;
        values[i] = cg_operand(g, exprs[i], false);
        if (effects < 2) continue;
        size_t t = cg_new_temp(g, exprs[i]->type);
        sb_appendf(out, "es_t%zu = %s, ", t, values[i]);
        values[i] = arena_sprintf(g->arena, "es_t%zu", t);
    }
    return effects >= 2;
}

static void emit_call(Codegen *g, const char *name, Expr *const *args, size_t n, size_t line, StrBuf *out) {
    const char **values = arena_alloc(g->arena, (n + 1) * sizeof(char *));
    bool opened = cg_ordered(g, args, n, values, out);
    cg_append_prefixed(out, "es_f_", name);
    sb_appendf(out, "(%zu", line);
    for (size_t i = 0; i < n; i++) sb_appendf(out, ", %s", values[i]);
    sb_append(out, opened ? "))" : ")");
}

static void emit_name(Codegen *g, const Expr *expr, StrBuf *out) {
    const char *name = expr->as.name;
    if (cg_is_constant(g, name)) {
        cg_append_prefixed(out, "es_k_", name);  // text constants are immortal: no retain
    } else if (cg_find_function(g, name)) {
        emit_call(g, name, NULL, 0, expr->pos.line, out);  // a function that takes no values
    } else {
        cg_emit_var(out, name, expr->type);
    }
}

static void emit_convert(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *operand_expr = expr->as.convert.operand;
    if (expr->as.convert.target == CONVERT_TO_TEXT) {
        cg_emit_text(g, operand_expr, out);
    } else if (operand_expr->type == TYPE_TEXT) {
        sb_appendf(out, "es_text_number(%zu, %s)", expr->pos.line, cg_operand(g, operand_expr, false));
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
    case EXPR_LENGTH:
        sb_appendf(out, "%s(%s)", type_is_list(expr->as.operand->type) ? "es_list_length" : "es_length",
                   cg_operand(g, expr->as.operand, false));
        break;
    case EXPR_FILE_CONTENTS:
        sb_appendf(out, "es_read_file(%zu, %s)", expr->pos.line, cg_operand(g, expr->as.operand, false));
        break;
    case EXPR_UNARY:
        sb_appendf(out, "(%s%s)", expr->as.unary.op == UNARY_NEGATE ? "-" : "!",
                   cg_operand(g, expr->as.unary.operand, false));
        break;
    case EXPR_BINARY: emit_binary(g, expr, out); break;
    case EXPR_CONVERT: emit_convert(g, expr, out); break;
    case EXPR_IT: emit_it(g, out); break;
    case EXPR_LIST:
    case EXPR_EMPTY_LIST:
    case EXPR_ITEM:
    case EXPR_LIST_OP:
    case EXPR_EVERY:
    case EXPR_ONLY:
    case EXPR_SLICE: cg_emit_list_expr(g, expr, out); break;
    case EXPR_NOTHING:
    case EXPR_ERROR:
        // The checker rejects these before codegen runs.
        assert(!"unchecked expression reached codegen");
        break;
    }
}

void cg_emit_text(Codegen *g, const Expr *expr, StrBuf *out) {
    if (type_is_list(expr->type)) {
        sb_appendf(out, "es_list_text(%s)", cg_operand(g, expr, false));
        return;
    }
    switch (expr->type) {
    case TYPE_TEXT: cg_emit_expr(g, expr, out); break;
    case TYPE_NUMBER: sb_appendf(out, "es_number_text(%s)", cg_operand(g, expr, false)); break;
    case TYPE_YESNO: sb_appendf(out, "es_yesno_text(%s)", cg_operand(g, expr, false)); break;
    default: assert(!"a value without a kind reached codegen"); break;
    }
}
