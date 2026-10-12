// C code for names, literals, and expressions (C names are in codegen_names.c). Statements, functions, and
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

void cg_emit_var(const Codegen *g, StrBuf *out, const char *name, Type type) {
    if (!type_counted(type)) {
        cg_append_var(g, out, name, type);
        return;
    }
    sb_appendf(out, "%s(", cg_retain(type));  // reading text or a list: one more owner
    cg_append_var(g, out, name, type);
    sb_append_char(out, ')');
}

static bool is_param(const Codegen *g, const char *name) {
    for (size_t i = 0; i < g->captured.len; i++) {
        if (strcmp(g->captured.items[i].name, name) == 0) return true;
    }
    for (size_t i = 0; i < g->borrowed.len; i++) {  // a cell from around a function made inside a value
        if (strcmp(g->borrowed.items[i], name) == 0) return true;
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
    if ((type_counted(type) || cg_is_cell(g, name)) && !in_scope(g, name, type) && g->scopes.len > 0) {
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

// static EsText es_s1 = {{ES_OBJ_TEXT, ES_IMMORTAL}, 6, "Name? ", 6, 0}; the
// second number is how many characters it has (UTF-8), so the runtime never
// has to count them.
size_t cg_static_text(Codegen *g, const char *s, size_t len) {
    size_t id = ++g->texts->count;
    size_t chars = 0;
    for (size_t i = 0; i < len; i++) chars += ((unsigned char)s[i] & 0xC0) != 0x80;
    sb_appendf(&g->texts->code, "static EsText es_s%zu = {{ES_OBJ_TEXT, ES_IMMORTAL}, %zu, ", id, len);
    cg_append_c_string(&g->texts->code, s, len);
    sb_appendf(&g->texts->code, ", %zu, 0};\n", chars);
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

// {line, column, length}: a new entry in es_sites.
size_t cg_site(Codegen *g, SourcePos pos) {
    size_t id = ++g->sites->count;
    sb_appendf(&g->sites->code, "    {%zu, %zu, %zu, %u},\n", pos.line, pos.column, pos.span.length, pos.file);
    return id;
}

size_t cg_new_temp(Codegen *g, Type type) {
    vec_push(g->arena, &g->temps, type);
    return g->temps.len;
}

// Reading what's in a list counts as an effect too: a call could change the
// list, so a call next to it has to keep its place in the order.
bool cg_has_effects(const Codegen *g, const Expr *expr) {
    if (expr->narrowed) return true;  // checked again while the program runs
    switch (expr->kind) {
    case EXPR_FIELD: return true;  // a call could change the field
    case EXPR_KIND_TEST: return cg_has_effects(g, expr->as.kind_test.operand);
    case EXPR_NEW:
        for (size_t i = 0; i < expr->as.new_record.values.len; i++) {
            if (cg_has_effects(g, expr->as.new_record.values.items[i].value)) return true;
        }
        return false;
    case EXPR_CALL:
    case EXPR_APPLY:
    case EXPR_FILE_CONTENTS:
    case EXPR_LIST_OP:
    case EXPR_EVERY:
    case EXPR_ONLY:
    case EXPR_MAP_GET:
    case EXPR_MAP_WITHOUT: return true;  // reads the map (and can fail)
    case EXPR_ITEM:
    case EXPR_SLICE: return true;  // can fail (a position past the end), for text too
    case EXPR_TEXT_OP: return cg_text_op_effects(g, expr);
    case EXPR_LIBRARY: return cg_library_effects(g, expr);
    case EXPR_LIST:
    case EXPR_MAP:
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
               type_is_list(expr->as.binary.left->type) || type_is_map(expr->as.binary.left->type) ||
               cg_has_effects(g, expr->as.binary.left) ||
               cg_has_effects(g, expr->as.binary.right);
    default: return false;
    }
}

const char *cg_list_operand(Codegen *g, const Expr *expr) {
    if (expr->kind == EXPR_NAME && cg_is_constant(g, expr->as.name)) {
        StrBuf sb;
        sb_init(&sb, g->arena);
        // immortal: releasing it does nothing (a fixed map is already a pointer)
        cg_append_prefixed(&sb, type_is_map(expr->type) ? "es_k_" : "&es_k_", expr->as.name);
        return sb.data;
    }
    return cg_operand(g, expr, false);
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
    const Expr *left = expr->as.binary.left, *right = expr->as.binary.right;
    if (left->kind == EXPR_NOTHING || right->kind == EXPR_NOTHING) {  // "x is nothing"
        sb_appendf(out, "%ses_is_nothing(%s)", op == BINARY_NOT_EQUAL ? "!" : "",
                   cg_operand(g, left->kind == EXPR_NOTHING ? right : left, false));
        return;
    }
    const char *l, *r, *close;
    operands(g, expr->as.binary.left, expr->as.binary.right, false, out, &l, &r, &close);
    bool equality = op == BINARY_EQUAL || op == BINARY_NOT_EQUAL;
    const char *negate = op == BINARY_NOT_EQUAL ? "!" : "";
    if (equality && type == TYPE_YESNO) {
        sb_appendf(out, "(%s %s %s)", l, op == BINARY_EQUAL ? "==" : "!=", r);
    } else if (equality) {
        const char *same = type_is_list(type) ? "es_list_same" : type_is_object(type) ? "es_record_same"
                           : type_is_map(type) ? "es_map_same" : type == TYPE_TEXT ? "es_text_same" : "es_close";
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
    case BINARY_OR:  // && and || work left to right and stop early, as "and"/"or" do
        sb_appendf(out, "(%s %s %s)", cg_operand(g, left, false), op == BINARY_AND ? "&&" : "||",
                   cg_operand(g, right, false));
        return;
    case BINARY_PIECES:
        operands(g, left, right, true, out, &l, &r, &close);
        sb_appendf(out, "es_join_spaced(%s, %s)%s", l, r, close);
        return;
    case BINARY_JOIN:
        if (type_is_list(expr->type)) {  // two lists: one list
            operands(g, left, right, false, out, &l, &r, &close);
            sb_appendf(out, "es_list_join(%s, %s)%s", l, r, close);
            return;
        }
        operands(g, left, right, true, out, &l, &r, &close);
        sb_appendf(out, "es_join(%s, %s)%s", l, r, close);
        return;
    case BINARY_DIVIDE:
    case BINARY_MODULO:
        operands(g, left, right, false, out, &l, &r, &close);
        sb_appendf(out, "%s(%zu, %s, %s)%s", op == BINARY_DIVIDE ? "es_div" : "es_mod", cg_site(g, expr->pos), l, r,
                   close);
        return;
    case BINARY_ADD:
    case BINARY_SUBTRACT:
    case BINARY_MULTIPLY:
        operands(g, left, right, false, out, &l, &r, &close);
        sb_appendf(out, "(%s %s %s)%s", l, arithmetic_operator(op), r, close);
        return;
    case BINARY_CONTAINS:
    case BINARY_NOT_CONTAINS:
        if (expr->as.binary.left->type == TYPE_TEXT) {
            cg_emit_text_search(g, expr, out);
        } else {
            cg_emit_contains(g, expr, out);
        }
        return;
    case BINARY_STARTS_WITH:
    case BINARY_NOT_STARTS_WITH:
    case BINARY_ENDS_WITH:
    case BINARY_NOT_ENDS_WITH: cg_emit_text_search(g, expr, out); return;
    case BINARY_HAS:
    case BINARY_NOT_HAS: cg_emit_has(g, expr, out); return;
    default: emit_compare(g, expr, out); return;
    }
}

// The runtime's letter for a number, text or yes/no in a box: es_box_n, es_unbox_t, ...
static char box_letter(Type present) {
    return present == TYPE_TEXT ? 't' : present == TYPE_YESNO ? 'b' : 'n';
}

// "it": the number of the innermost count or times loop, or the item of a
// for-each loop or helper, or the value of a "when" (out of its box where
// it's checked not to be nothing). The checker made sure there is one.
static void emit_it(Codegen *g, const Expr *expr, StrBuf *out) {
    for (size_t i = g->loops.len; i > 0; i--) {
        const LoopCode *loop = &g->loops.items[i - 1];
        if (loop->item && type_is_maybe(loop->item_type) && type_boxed(expr->type)) {
            sb_appendf(out, "es_unbox_%c(%zu, ", box_letter(expr->type), cg_site(g, expr->pos));
            cg_emit_var(g, out, loop->item, loop->item_type);
            sb_append_char(out, ')');
            return;
        }
        if (loop->item) {
            cg_emit_var(g, out, loop->item, loop->item_type);
            return;
        }
        if (loop->stmt->as.loop.kind == LOOP_COUNT) {
            cg_append_var(g, out, loop->stmt->as.loop.var.text, TYPE_NUMBER);
            return;
        }
        if (loop->stmt->as.loop.kind == LOOP_TIMES) {
            sb_appendf(out, "((double)es_i%zu)", loop->id);
            return;
        }
    }
    assert(!"\"it\" outside a counting loop reached codegen");
}

// es_f_area(SITE, A, B), SITE being where the call is (for the call stack).
// When two or more arguments have effects, they go into temporaries first,
// left to right: (t1 = A, t2 = B, es_f_area(SITE, t1, t2)).
// The function owns its text arguments and releases them when it ends.
bool cg_ordered(Codegen *g, Expr *const *exprs, size_t n, const char **values, StrBuf *out) {
    size_t effects = 0;
    for (size_t i = 0; i < n; i++) effects += exprs[i] && cg_has_effects(g, exprs[i]);
    if (effects >= 2) sb_append_char(out, '(');
    for (size_t i = 0; i < n; i++) {
        if (!exprs[i]) continue;
        values[i] = cg_operand(g, exprs[i], false);
        if (effects < 2) continue;
        size_t t = cg_new_temp(g, cg_value_type(exprs[i]));
        sb_appendf(out, "es_t%zu = %s, ", t, values[i]);
        values[i] = arena_sprintf(g->arena, "es_t%zu", t);
    }
    return effects >= 2;
}

static void emit_call(Codegen *g, const Stmt *fn, Expr *const *args, size_t n, SourcePos pos, StrBuf *out) {
    const char **values = arena_alloc(g->arena, (n + 1) * sizeof(char *));
    bool opened = cg_ordered(g, args, n, values, out);
    cg_append_function(out, "es_f_", fn);
    sb_appendf(out, "(%zu", cg_site(g, pos));
    for (size_t i = 0; i < n; i++) sb_appendf(out, ", %s", values[i]);
    sb_append(out, opened ? "))" : ")");
}

static void emit_name(Codegen *g, const Expr *expr, StrBuf *out) {
    const char *name = expr->as.name;
    if (cg_is_constant(g, name) && type_is_list(expr->type)) {
        // A fixed list read where it could be kept (and changed) gets copied;
        // operations that only look at it use it directly (cg_list_operand).
        cg_append_prefixed(out, "es_list_copy(&es_k_", name);
        sb_append_char(out, ')');
    } else if (cg_is_constant(g, name) && type_is_map(expr->type)) {  // a fixed map, likewise
        cg_append_prefixed(out, "es_map_copy(es_k_", name);
        sb_append_char(out, ')');
    } else if (cg_is_constant(g, name)) {
        cg_append_prefixed(out, "es_k_", name);  // text constants are immortal: no retain
    } else if (cg_find_function(g, name)) {
        emit_call(g, cg_find_function(g, name), NULL, 0, expr->pos, out);  // a function that takes no values
    } else {  // a record or nothing checked not to be nothing is still kept as one
        cg_emit_var(g, out, name, expr->narrowed ? expr->type + TYPE_MAYBE : expr->type);
    }
}

static void emit_convert(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *operand_expr = expr->as.convert.operand;
    if (expr->as.convert.target == CONVERT_TO_TEXT) {
        cg_emit_text(g, operand_expr, out);
    } else if (expr->as.convert.target == CONVERT_TO_EXACT_TEXT) {
        sb_append(out, "es_number_exact_text(");
        cg_emit_expr(g, operand_expr, out);
        sb_append(out, ")");
    } else if (operand_expr->type == TYPE_TEXT) {
        sb_appendf(out, "es_text_number(%zu, %s)", cg_site(g, expr->pos), cg_operand(g, operand_expr, false));
    } else {
        cg_emit_expr(g, operand_expr, out);
    }
}

static void emit_expr(Codegen *g, const Expr *expr, StrBuf *out);

// A value that can be nothing, checked not to be: a record or a value of a
// kind is checked again (another name for the same record could have
// changed it since), and a number, text or yes/no comes out of its box. A
// value given where "or nothing" is kept goes into a box.
void cg_emit_expr(Codegen *g, const Expr *expr, StrBuf *out) {
    bool box = expr->lifted && type_boxed(expr->type);
    if (box) sb_appendf(out, "es_box_%c(", box_letter(expr->type));
    if (!expr->narrowed) {
        emit_expr(g, expr, out);
    } else if (type_boxed(expr->type)) {
        sb_appendf(out, "es_unbox_%c(%zu, ", box_letter(expr->type), cg_site(g, expr->pos));
        emit_expr(g, expr, out);
        sb_append_char(out, ')');
    } else {
        sb_appendf(out, "es_present(%zu, ", cg_site(g, expr->pos));
        emit_expr(g, expr, out);
        sb_append_char(out, ')');
    }
    if (box) sb_append_char(out, ')');
}

Type cg_value_type(const Expr *expr) {
    return expr->lifted && type_boxed(expr->type) ? expr->type + TYPE_MAYBE : expr->type;
}

static void emit_expr(Codegen *g, const Expr *expr, StrBuf *out) {
    switch (expr->kind) {
    case EXPR_NUMBER: emit_number(expr, out); break;
    case EXPR_TEXT: sb_appendf(out, "&es_s%zu", cg_static_text(g, expr->as.text.value, expr->as.text.len)); break;
    case EXPR_BOOLEAN: sb_append(out, expr->as.boolean ? "true" : "false"); break;
    case EXPR_NAME: emit_name(g, expr, out); break;
    case EXPR_CALL:
        if (expr->as.call.callee) {
            cg_emit_value_call(g, expr, out);
            break;
        }
        if (expr->as.call.dispatch) {
            cg_emit_dispatch(g, expr, out);
            break;
        }
        emit_call(g, expr->as.call.target, expr->as.call.args.items, expr->as.call.args.len, expr->pos, out);
        break;
    case EXPR_FUNCTION_REF: cg_emit_function_ref(g, expr, out); break;
    case EXPR_LAMBDA: cg_emit_lambda(g, expr, out); break;
    case EXPR_APPLY: cg_emit_apply(g, expr, out); break;
    case EXPR_LIBRARY: cg_emit_library(g, expr, out); break;
    case EXPR_LENGTH:
        if (type_is_list(expr->as.operand->type)) {
            sb_appendf(out, "es_list_length(%s)", cg_list_operand(g, expr->as.operand));
        } else {
            sb_appendf(out, "es_length(%s)", cg_operand(g, expr->as.operand, false));
        }
        break;
    case EXPR_FILE_CONTENTS:
        sb_appendf(out, "es_read_file(%zu, %s)", cg_site(g, expr->as.operand->pos), cg_operand(g, expr->as.operand, false));
        break;
    case EXPR_UNARY:
        sb_appendf(out, "(%s%s)", expr->as.unary.op == UNARY_NEGATE ? "-" : "!",
                   cg_operand(g, expr->as.unary.operand, false));
        break;
    case EXPR_BINARY: emit_binary(g, expr, out); break;
    case EXPR_CONVERT: emit_convert(g, expr, out); break;
    case EXPR_IT: emit_it(g, expr, out); break;
    case EXPR_LIST:
    case EXPR_EMPTY_LIST:
    case EXPR_ITEM:
    case EXPR_LIST_OP:
    case EXPR_EVERY:
    case EXPR_ONLY:
    case EXPR_SLICE: cg_emit_list_expr(g, expr, out); break;
    case EXPR_TEXT_OP: cg_emit_text_op(g, expr, out); break;
    case EXPR_FIELD: cg_emit_field(g, expr, out); break;
    case EXPR_NEW: cg_emit_new(g, expr, out); break;
    case EXPR_KIND_TEST: cg_emit_kind_test(g, expr, out); break;
    case EXPR_MAP:
    case EXPR_EMPTY_MAP:
    case EXPR_MAP_GET:
    case EXPR_MAP_WITHOUT: cg_emit_map_expr(g, expr, out); break;
    case EXPR_NOTHING: sb_append(out, "NULL"); break;  // where a record or nothing goes (the checker made sure)
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
    if (type_is_object(expr->type)) {
        sb_appendf(out, "es_record_text(%s)", cg_operand(g, expr, false));
        return;
    }
    if (type_is_map(expr->type)) {
        sb_appendf(out, "es_map_text(%s)", cg_list_operand(g, expr));
        return;
    }
    switch (expr->type) {
    case TYPE_TEXT: cg_emit_expr(g, expr, out); break;
    case TYPE_NUMBER: sb_appendf(out, "es_number_text(%s)", cg_operand(g, expr, false)); break;
    case TYPE_YESNO: sb_appendf(out, "es_yesno_text(%s)", cg_operand(g, expr, false)); break;
    case TYPE_DATE: sb_appendf(out, "es_date_text(%s)", cg_operand(g, expr, false)); break;
    default: assert(!"a value without a kind reached codegen"); break;
    }
}
