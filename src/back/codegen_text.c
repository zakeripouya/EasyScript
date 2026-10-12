// C code for text tools (runtime/es_text.h): characters and pieces of text,
// searching, splitting and joining, changing case, and character codes.
// Like the list operations, each runtime function consumes the text it's
// given, and the parts of a form are worked out in the order they're written.

#include <assert.h>
#include <string.h>
#include "back/codegen_internal.h"

static const char *where_code(ItemWhere where) {
    return where == ITEM_FIRST ? "ES_FIRST" : where == ITEM_LAST ? "ES_LAST" : "ES_AT";
}

// Whether a text tool can fail or reads a list (whose items a call could change).
bool cg_text_op_effects(const Codegen *g, const Expr *expr) {
    switch (expr->as.text_op.op) {
    case TEXT_SPLIT:
    case TEXT_JOINED:
    case TEXT_REPLACE:
    case TEXT_CODE:
    case TEXT_FROM_CODE: return true;
    case TEXT_POSITION:
        if (type_is_list(expr->as.text_op.operand->type)) return true;
        break;
    default: break;
    }
    Expr *const parts[] = {expr->as.text_op.operand, expr->as.text_op.arg, expr->as.text_op.arg2};
    for (size_t i = 0; i < 3; i++) {
        if (parts[i] && cg_has_effects(g, parts[i])) return true;
    }
    return false;
}

// "character P of T": es_char_at(SITE, T, ES_AT, P), the position worked out first.
static void emit_character(Codegen *g, const Expr *expr, StrBuf *out) {
    Expr *parts[] = {expr->as.item.position, expr->as.item.list};
    const char *v[2] = {"0", NULL};
    bool opened = cg_ordered(g, parts, 2, v, out);
    sb_appendf(out, "es_char_at(%zu, %s, %s, %s)%s", cg_site(g, expr->pos), v[1], where_code(expr->as.item.where), v[0],
               opened ? ")" : "");
}

// The text versions of the list slices (codegen_list.c's emit_slice).
static void emit_characters(Codegen *g, const Expr *expr, StrBuf *out) {
    SliceKind kind = expr->as.slice.kind;
    if (kind == SLICE_WITHOUT_AT) {  // the text is written first: es_text_without_at(SITE, T, POSITION)
        Expr *parts[] = {expr->as.slice.list, expr->as.slice.from};
        const char *v[2];
        bool opened = cg_ordered(g, parts, 2, v, out);
        sb_appendf(out, "es_text_without_at(%zu, %s, %s)%s", cg_site(g, expr->pos), v[0], v[1], opened ? ")" : "");
        return;
    }
    Expr *parts[] = {expr->as.slice.from, expr->as.slice.to, expr->as.slice.list};
    const char *v[3] = {"0", "0", NULL};
    bool opened = cg_ordered(g, parts, 3, v, out);
    size_t site = kind == SLICE_EVERY ? 0 : cg_site(g, expr->pos);  // SLICE_EVERY can't fail
    switch (kind) {
    case SLICE_RANGE:
        sb_appendf(out, "es_text_range(%zu, %s, %s, %s, %d)", site, v[2], v[0], v[1], expr->as.slice.to == NULL);
        break;
    case SLICE_FIRST:
    case SLICE_LAST: sb_appendf(out, "es_text_end(%zu, %s, %s, %d)", site, v[2], v[0], kind == SLICE_LAST); break;
    case SLICE_WITHOUT_FIRST:
    case SLICE_WITHOUT_LAST: sb_appendf(out, "es_text_without(%zu, %s, %d)", site, v[2], kind == SLICE_WITHOUT_LAST); break;
    case SLICE_EVERY:
        sb_appendf(out, "es_text_every(%s, %zu, %zu)", v[2], expr->as.slice.step, expr->as.slice.start);
        break;
    case SLICE_WITHOUT_AT: break;  // above
    }
    if (opened) sb_append_char(out, ')');
}

void cg_emit_characters(Codegen *g, const Expr *expr, StrBuf *out) {
    if (expr->kind == EXPR_ITEM) {
        emit_character(g, expr, out);
    } else {
        emit_characters(g, expr, out);
    }
}

// "T contains X" for text, "T starts with X", "T ends with X".
void cg_emit_text_search(Codegen *g, const Expr *expr, StrBuf *out) {
    BinaryOp op = expr->as.binary.op;
    Expr *parts[] = {expr->as.binary.left, expr->as.binary.right};
    const char *v[2];
    bool opened = cg_ordered(g, parts, 2, v, out);
    bool negated = op == BINARY_NOT_CONTAINS || op == BINARY_NOT_STARTS_WITH || op == BINARY_NOT_ENDS_WITH;
    sb_append(out, negated ? "!" : "");
    if (op == BINARY_CONTAINS || op == BINARY_NOT_CONTAINS) {
        sb_appendf(out, "es_text_contains(%s, %s)", v[0], v[1]);
    } else {
        bool end = op == BINARY_ENDS_WITH || op == BINARY_NOT_ENDS_WITH;
        sb_appendf(out, "es_text_starts(%s, %s, %d)", v[0], v[1], end);
    }
    if (opened) sb_append_char(out, ')');
}

// "the position of X in T" (or in a list): X is written first, so it's worked out first.
// es_found(es_text_position(X, T)): a number or nothing (the runtime's 0 is nothing).
static void emit_position(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *in = expr->as.text_op.operand;
    Expr *parts[] = {expr->as.text_op.arg, expr->as.text_op.operand};
    const char *v[2];
    sb_append(out, "es_found(");
    bool opened = cg_ordered(g, parts, 2, v, out);
    if (!opened && type_is_list(in->type) && in->kind == EXPR_NAME) v[1] = cg_list_operand(g, in);
    if (type_is_list(in->type)) {
        sb_appendf(out, "es_position_%s(%s, %s)", cg_item_suffix(in->type), v[0], v[1]);
    } else {
        sb_appendf(out, "es_text_position(%s, %s)", v[0], v[1]);
    }
    sb_append(out, opened ? "))" : ")");
}

// The forms with more than one part: split, joined, replaced.
static void emit_with_parts(Codegen *g, const Expr *expr, StrBuf *out) {
    TextOp op = expr->as.text_op.op;
    Expr *parts[] = {expr->as.text_op.operand, expr->as.text_op.arg, expr->as.text_op.arg2};
    const char *v[3] = {NULL, NULL, NULL};
    bool opened = cg_ordered(g, parts, 3, v, out);
    if (op == TEXT_JOINED) {
        if (!opened && expr->as.text_op.operand->kind == EXPR_NAME) v[0] = cg_list_operand(g, expr->as.text_op.operand);
        sb_appendf(out, "es_list_joined(%s, %s)", v[0], v[1]);
    } else if (op == TEXT_SPLIT) {
        sb_appendf(out, "es_text_split(%zu, %s, %s)", cg_site(g, expr->as.text_op.arg->pos), v[0], v[1]);
    } else {
        sb_appendf(out, "es_text_replace(%zu, %s, %s, %s)", cg_site(g, expr->as.text_op.arg->pos), v[0], v[1], v[2]);
    }
    if (opened) sb_append_char(out, ')');
}

void cg_emit_text_op(Codegen *g, const Expr *expr, StrBuf *out) {
    TextOp op = expr->as.text_op.op;
    if (op == TEXT_SPLIT || op == TEXT_JOINED || op == TEXT_REPLACE) {
        emit_with_parts(g, expr, out);
        return;
    }
    if (op == TEXT_POSITION) {
        emit_position(g, expr, out);
        return;
    }
    const char *operand = cg_operand(g, expr->as.text_op.operand, false);
    const char *negated = expr->as.text_op.negated ? "!" : "";
    switch (op) {
    case TEXT_CHARACTERS: sb_appendf(out, "es_text_characters(%s)", operand); break;
    case TEXT_WORDS: sb_appendf(out, "es_text_words(%s)", operand); break;
    case TEXT_LINES: sb_appendf(out, "es_text_lines(%s)", operand); break;
    case TEXT_UPPERCASE:
    case TEXT_LOWERCASE: sb_appendf(out, "es_text_case(%s, %d)", operand, op == TEXT_UPPERCASE); break;
    case TEXT_TRIM: sb_appendf(out, "es_text_trim(%s)", operand); break;
    case TEXT_CODE: sb_appendf(out, "es_text_code(%zu, %s)", cg_site(g, expr->pos), operand); break;
    case TEXT_FROM_CODE: sb_appendf(out, "es_text_from_code(%zu, %s)", cg_site(g, expr->pos), operand); break;
    case TEXT_IS_LETTER: sb_appendf(out, "%ses_text_is_letter(%s)", negated, operand); break;
    case TEXT_IS_DIGIT: sb_appendf(out, "%ses_text_is_digit(%s)", negated, operand); break;
    case TEXT_IS_SPACE: sb_appendf(out, "%ses_text_is_space(%s)", negated, operand); break;
    default: assert(!"text tool with several parts"); break;
    }
}

// --- Building text -------------------------------------------------------------------

// Whether the expression reads the variable `name`. (Only a name can: a
// function can't see the caller's variables, and text never changes. A
// function made inside one can, but then the name is kept in a cell, which
// is never built in place.)
static bool reads(const Expr *expr, const char *name) {
    if (!expr) return false;
    switch (expr->kind) {
    case EXPR_NAME: return strcmp(expr->as.name, name) == 0;
    case EXPR_FUNCTION_REF: return false;
    case EXPR_LIBRARY: return reads(expr->as.library.a, name) || reads(expr->as.library.b, name);
    case EXPR_LAMBDA: return true;  // it could use the name (in place building isn't worth working that out)
    case EXPR_APPLY:
        return reads(expr->as.apply.function, name) || reads(expr->as.apply.list, name) ||
               reads(expr->as.apply.start, name);
    case EXPR_CALL:
        if (reads(expr->as.call.callee, name)) return true;
        // fall through
    case EXPR_LIST:
    case EXPR_MAP: {
        Expr *const *items = expr->kind == EXPR_CALL ? expr->as.call.args.items : expr->as.items.items;
        size_t n = expr->kind == EXPR_CALL ? expr->as.call.args.len : expr->as.items.len;
        for (size_t i = 0; i < n; i++) {
            if (reads(items[i], name)) return true;
        }
        return false;
    }
    case EXPR_LENGTH:
    case EXPR_FILE_CONTENTS: return reads(expr->as.operand, name);
    case EXPR_UNARY: return reads(expr->as.unary.operand, name);
    case EXPR_BINARY: return reads(expr->as.binary.left, name) || reads(expr->as.binary.right, name);
    case EXPR_CONVERT: return reads(expr->as.convert.operand, name);
    case EXPR_ITEM: return reads(expr->as.item.position, name) || reads(expr->as.item.list, name);
    case EXPR_LIST_OP: return reads(expr->as.list_op.operand, name);
    case EXPR_EVERY: return reads(expr->as.every.list, name) || reads(expr->as.every.body, name);
    case EXPR_ONLY: return reads(expr->as.only.list, name) || reads(expr->as.only.condition, name);
    case EXPR_DEFAULT: return reads(expr->as.lookup.map, name) || reads(expr->as.lookup.fallback, name);
    case EXPR_TABLE:
        if (reads(expr->as.table.subject, name)) return true;
        for (size_t i = 0; i < expr->as.table.values.len; i++) {
            if (reads(expr->as.table.values.items[i], name) || reads(expr->as.table.conditions.items[i], name)) return true;
        }
        return false;
    case EXPR_AGGREGATE:
        return reads(expr->as.aggregate.count, name) || reads(expr->as.aggregate.list, name) ||
               reads(expr->as.aggregate.condition, name) || reads(expr->as.aggregate.key, name) ||
               reads(expr->as.aggregate.group, name);
    case EXPR_SLICE:
        return reads(expr->as.slice.from, name) || reads(expr->as.slice.to, name) || reads(expr->as.slice.list, name);
    case EXPR_TEXT_OP:
        return reads(expr->as.text_op.operand, name) || reads(expr->as.text_op.arg, name) ||
               reads(expr->as.text_op.arg2, name);
    case EXPR_FIELD: return reads(expr->as.field.record, name);
    case EXPR_KIND_TEST: return reads(expr->as.kind_test.operand, name);
    case EXPR_MAP_GET:
    case EXPR_MAP_WITHOUT:
        return reads(expr->as.lookup.key, name) || reads(expr->as.lookup.map, name) ||
               reads(expr->as.lookup.fallback, name);
    case EXPR_NEW:
        for (size_t i = 0; i < expr->as.new_record.values.len; i++) {
            if (reads(expr->as.new_record.values.items[i].value, name)) return true;
        }
        return false;
    case EXPR_ERROR:
    case EXPR_NUMBER:
    case EXPR_TEXT:
    case EXPR_BOOLEAN:
    case EXPR_NOTHING:
    case EXPR_IT:
    case EXPR_EMPTY_LIST:
    case EXPR_EMPTY_MAP: return false;
    }
    return true;
}

// "set T to T followed by A followed by B" (or "[T][A]"), where only the first
// part reads T: each part is added to T in place (es_append), so text built
// piece by piece isn't copied every time. Returns false for anything else.
bool cg_emit_append(Codegen *g, const Stmt *stmt) {
    const char *name = stmt->as.assign.name.text;
    const Expr *value = stmt->as.assign.value;
    Vec(const Expr *) parts = {0};
    while (value->type == TYPE_TEXT && value->kind == EXPR_BINARY &&
           value->as.binary.op == BINARY_JOIN) {
        if (reads(value->as.binary.right, name)) return false;
        vec_push(g->arena, &parts, value->as.binary.right);
        value = value->as.binary.left;
    }
    if (parts.len == 0 || value->kind != EXPR_NAME || strcmp(value->as.name, name) != 0 || value->type != TYPE_TEXT ||
        value->narrowed || stmt->as.assign.type != TYPE_TEXT ||
        cg_is_constant(g, name) || cg_find_function(g, name) || cg_is_cell(g, name)) {
        return false;
    }
    for (size_t i = parts.len; i > 0; i--) {
        StrBuf part;
        sb_init(&part, g->arena);
        cg_emit_text(g, parts.items[i - 1], &part);
        sb_append_repeat(&g->body, ' ', 4 * (g->depth + 1));
        sb_append(&g->body, "es_append(&");
        cg_append_var(g, &g->body, name, TYPE_TEXT);
        sb_appendf(&g->body, ", %s);\n", part.data);
    }
    return true;
}
