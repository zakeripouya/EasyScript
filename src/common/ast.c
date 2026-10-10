#include "common/ast.h"

Expr *ast_new_expr(Arena *arena, ExprKind kind, SourcePos pos) {
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->kind = kind;
    expr->pos = pos;
    return expr;
}

Stmt *ast_new_stmt(Arena *arena, StmtKind kind, SourcePos pos) {
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->kind = kind;
    stmt->pos = pos;
    return stmt;
}

SourcePos ast_pos_join(SourcePos first, SourcePos last) {
    SourcePos pos = first;
    size_t end = last.span.offset + last.span.length;
    pos.span.length = end > first.span.offset ? end - first.span.offset : 0;
    return pos;
}

// "a list of numbers", "a list of lists of text"; "a list" (or "a list of
// lists") when the innermost kind isn't decided.
static const char *list_name(Arena *arena, Type type) {
    static const char *const plural[] = {NULL, "numbers", "text", "yes/no values"};
    StrBuf sb;
    sb_init(&sb, arena);
    sb_append(&sb, "a list");
    Type base = type_base(type);
    bool known = base == TYPE_NUMBER || base == TYPE_TEXT || base == TYPE_YESNO;
    for (Type t = type_item(type); type_is_list(t); t = type_item(t)) sb_append(&sb, " of lists");
    if (known) sb_appendf(&sb, " of %s", plural[base]);
    return sb.data;
}

const char *ast_type_name(Arena *arena, Type type) {
    if (type_is_list(type)) return list_name(arena, type);
    switch (type) {
    case TYPE_NUMBER: return "a number";
    case TYPE_TEXT: return "text";
    case TYPE_YESNO: return "a yes/no value";
    case TYPE_NOTHING: return "nothing";
    default: return "an unknown kind of value";
    }
}

const char *ast_binary_op_name(BinaryOp op) {
    static const char *const names[] = {
        "or", "and", "equal", "not equal", "less", "less or equal", "greater",
        "greater or equal", "add", "subtract", "join", "multiply", "divide", "modulo",
        "contains", "does not contain",
    };
    return names[op];
}

const char *ast_unary_op_name(UnaryOp op) {
    return op == UNARY_NEGATE ? "negate" : "not";
}

const char *ast_change_op_name(ChangeOp op) {
    static const char *const names[] = {"add", "subtract", "multiply", "divide"};
    return names[op];
}

const char *ast_list_op_name(ListOp op) {
    static const char *const names[] = {
        "added together", "biggest of", "smallest of", "reversed", "copy of", "is empty", "is not empty",
    };
    return names[op];
}

static void dump_expr(const Expr *expr, size_t depth, StrBuf *out);

static void dump_line(const SourcePos *pos, size_t depth, StrBuf *out, const char *label) {
    sb_append_repeat(out, ' ', depth * 2);
    sb_append(out, label);
    sb_appendf(out, " [%zu:%zu]\n", pos->line, pos->column);
}

static void dump_children(const Expr *a, const Expr *b, size_t depth, StrBuf *out) {
    if (a) dump_expr(a, depth + 1, out);
    if (b) dump_expr(b, depth + 1, out);
}

// The label for a leaf or a node whose label carries a value.
static const char *expr_label(const Expr *expr, StrBuf *scratch) {
    switch (expr->kind) {
    case EXPR_ERROR: return "error";
    case EXPR_NUMBER: sb_appendf(scratch, "number %s", expr->as.number.text); break;
    case EXPR_TEXT:
        sb_append(scratch, "text ");
        sb_append_quoted(scratch, expr->as.text.value, expr->as.text.len);
        break;
    case EXPR_BOOLEAN: return expr->as.boolean ? "yes" : "no";
    case EXPR_NOTHING: return "nothing";
    case EXPR_IT: return "it";
    case EXPR_NAME: sb_appendf(scratch, "name %s", expr->as.name); break;
    case EXPR_CALL: sb_appendf(scratch, "call %s", expr->as.call.name); break;
    case EXPR_LENGTH: return "length of";
    case EXPR_FILE_CONTENTS: return "contents of file";
    case EXPR_UNARY: return ast_unary_op_name(expr->as.unary.op);
    case EXPR_BINARY: return ast_binary_op_name(expr->as.binary.op);
    case EXPR_CONVERT: return expr->as.convert.target == CONVERT_TO_NUMBER ? "as number" : "as text";
    case EXPR_LIST: return "list of";
    case EXPR_EMPTY_LIST:
        sb_appendf(scratch, "empty %s", ast_type_name(scratch->arena, expr->as.list_type) + 2);  // without "a "
        break;
    case EXPR_ITEM:
        return expr->as.item.where == ITEM_FIRST ? "first item" : expr->as.item.where == ITEM_LAST ? "last item" : "item";
    case EXPR_LIST_OP: return ast_list_op_name(expr->as.list_op.op);
    case EXPR_EVERY: sb_appendf(scratch, "every %s in", expr->as.every.var.text); break;
    case EXPR_ONLY: return "only";
    }
    return scratch->data;
}

static void dump_expr(const Expr *expr, size_t depth, StrBuf *out) {
    StrBuf label;
    sb_init(&label, out->arena);
    dump_line(&expr->pos, depth, out, expr_label(expr, &label));
    switch (expr->kind) {
    case EXPR_CALL:
        for (size_t i = 0; i < expr->as.call.args.len; i++) {
            dump_expr(expr->as.call.args.items[i], depth + 1, out);
        }
        break;
    case EXPR_LENGTH:
    case EXPR_FILE_CONTENTS: dump_children(expr->as.operand, NULL, depth, out); break;
    case EXPR_UNARY: dump_children(expr->as.unary.operand, NULL, depth, out); break;
    case EXPR_BINARY: dump_children(expr->as.binary.left, expr->as.binary.right, depth, out); break;
    case EXPR_CONVERT: dump_children(expr->as.convert.operand, NULL, depth, out); break;
    case EXPR_LIST:
        for (size_t i = 0; i < expr->as.items.len; i++) dump_expr(expr->as.items.items[i], depth + 1, out);
        break;
    case EXPR_ITEM: dump_children(expr->as.item.position, expr->as.item.list, depth, out); break;
    case EXPR_LIST_OP: dump_children(expr->as.list_op.operand, NULL, depth, out); break;
    case EXPR_EVERY: dump_children(expr->as.every.list, expr->as.every.body, depth, out); break;
    case EXPR_ONLY: dump_children(expr->as.only.list, expr->as.only.condition, depth, out); break;
    default: break;
    }
}

static const char *loop_label(const Stmt *stmt, StrBuf *scratch) {
    switch (stmt->as.loop.kind) {
    case LOOP_COUNT:
        sb_appendf(scratch, "count%s, call each number %s", stmt->as.loop.down ? " down" : "", stmt->as.loop.var.text);
        return scratch->data;
    case LOOP_TIMES: return "repeat times";
    case LOOP_WHILE: return "repeat while";
    case LOOP_UNTIL: return "repeat until";
    case LOOP_FOREVER: return "repeat forever";
    case LOOP_EACH: sb_appendf(scratch, "for each %s in", stmt->as.loop.var.text); return scratch->data;
    }
    return "loop";
}

static const char *list_label(const Stmt *stmt, StrBuf *scratch) {
    static const char *const where[] = {"item", "first item", "last item"};
    switch (stmt->as.list.op) {
    case LIST_ADD_END: return "add to the end of";
    case LIST_INSERT: return "insert at position";
    case LIST_REMOVE: sb_appendf(scratch, "remove %s", where[stmt->as.list.where]); break;
    case LIST_SET_ITEM: sb_appendf(scratch, "set %s", where[stmt->as.list.where]); break;
    case LIST_SORT:
        sb_append(scratch, stmt->as.list.by_length ? "sort by length" : "sort");
        if (stmt->as.list.descending) sb_append(scratch, stmt->as.list.by_length ? ", longest first" : ", biggest first");
        break;
    }
    return scratch->data;
}

// The first line of a statement, e.g. "let total" or "read file, call it notes".
static const char *stmt_label(const Stmt *stmt, StrBuf *scratch) {
    switch (stmt->kind) {
    case STMT_LET: sb_appendf(scratch, "let %s", stmt->as.assign.name.text); break;
    case STMT_SET: sb_appendf(scratch, "set %s", stmt->as.assign.name.text); break;
    case STMT_CHANGE:
        sb_appendf(scratch, "%s %s", ast_change_op_name(stmt->as.change.op), stmt->as.change.target.text);
        break;
    case STMT_SAY: return "say";
    case STMT_ASK: sb_appendf(scratch, "ask, call the answer %s", stmt->as.ask.answer.text); break;
    case STMT_WRITE_FILE: return "write to file";
    case STMT_APPEND_FILE: return "append to file";
    case STMT_READ_FILE: sb_appendf(scratch, "read file, call it %s", stmt->as.read_file.name.text); break;
    case STMT_STOP: return "stop the program";
    case STMT_IF: return stmt->as.if_stmt.one_line ? "if (one line)" : "if";
    case STMT_LOOP: return loop_label(stmt, scratch);
    case STMT_BREAK: return "stop the loop";
    case STMT_CONTINUE: return "skip this one";
    case STMT_FUNCTION:
        sb_appendf(scratch, "define %s (", stmt->as.function.name.text);
        for (size_t i = 0; i < stmt->as.function.params.len; i++) {
            sb_appendf(scratch, "%s%s", i ? ", " : "", stmt->as.function.params.items[i].text);
            Type declared = stmt->as.function.declared.items[i];
            if (declared != TYPE_UNKNOWN) sb_appendf(scratch, ": %s", ast_type_name(scratch->arena, declared));
        }
        sb_append_char(scratch, ')');
        if (stmt->as.function.declared_result != TYPE_UNKNOWN) {
            sb_appendf(scratch, " giving back %s", ast_type_name(scratch->arena, stmt->as.function.declared_result));
        }
        break;
    case STMT_RETURN: return "give back";
    case STMT_CALL: return "call";
    case STMT_CONSTANT: sb_appendf(scratch, "keep %s", stmt->as.constant.name.text); break;
    case STMT_LIST: return list_label(stmt, scratch);
    }
    return scratch->data;
}

static void dump_stmt(const Stmt *stmt, size_t depth, StrBuf *out);

static void dump_label(size_t depth, StrBuf *out, const char *label) {
    sb_append_repeat(out, ' ', depth * 2);
    sb_append(out, label);
    sb_append_char(out, '\n');
}

// branch if / branch otherwise if / branch otherwise, then "condition" and
// "then" sections.
static void dump_if(const Stmt *stmt, size_t depth, StrBuf *out) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        const char *label = i == 0 ? "branch if" : branch->condition ? "branch otherwise if" : "branch otherwise";
        dump_line(&branch->pos, depth + 1, out, label);
        if (branch->condition) {
            dump_label(depth + 2, out, "condition");
            dump_expr(branch->condition, depth + 3, out);
        }
        dump_label(depth + 2, out, "then");
        for (size_t j = 0; j < branch->body.len; j++) {
            dump_stmt(branch->body.items[j], depth + 3, out);
        }
    }
}

static void dump_section(const char *label, const Expr *expr, size_t depth, StrBuf *out) {
    if (!expr) return;
    dump_label(depth, out, label);
    dump_expr(expr, depth + 1, out);
}

// Sections "from", "to", "by", "times", "condition", then "do" with the body.
static void dump_loop(const Stmt *stmt, size_t depth, StrBuf *out) {
    dump_section("from", stmt->as.loop.from, depth + 1, out);
    dump_section("to", stmt->as.loop.to, depth + 1, out);
    dump_section("by", stmt->as.loop.step, depth + 1, out);
    dump_section("times", stmt->as.loop.times, depth + 1, out);
    dump_section("condition", stmt->as.loop.condition, depth + 1, out);
    dump_section("in", stmt->as.loop.list, depth + 1, out);
    dump_label(depth + 1, out, "do");
    for (size_t i = 0; i < stmt->as.loop.body.len; i++) {
        dump_stmt(stmt->as.loop.body.items[i], depth + 2, out);
    }
}

static void dump_stmt(const Stmt *stmt, size_t depth, StrBuf *out) {
    StrBuf label;
    sb_init(&label, out->arena);
    dump_line(&stmt->pos, depth, out, stmt_label(stmt, &label));
    switch (stmt->kind) {
    case STMT_LET:
    case STMT_SET: dump_expr(stmt->as.assign.value, depth + 1, out); break;
    case STMT_CHANGE: dump_expr(stmt->as.change.amount, depth + 1, out); break;
    case STMT_SAY: dump_expr(stmt->as.value, depth + 1, out); break;
    case STMT_ASK: dump_expr(stmt->as.ask.prompt, depth + 1, out); break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE:
        dump_children(stmt->as.file_write.text, stmt->as.file_write.path, depth, out);
        break;
    case STMT_READ_FILE: dump_expr(stmt->as.read_file.path, depth + 1, out); break;
    case STMT_STOP: break;
    case STMT_IF: dump_if(stmt, depth, out); break;
    case STMT_LOOP: dump_loop(stmt, depth, out); break;
    case STMT_BREAK:
    case STMT_CONTINUE: break;
    case STMT_FUNCTION:
        dump_label(depth + 1, out, "do");
        for (size_t i = 0; i < stmt->as.function.body.len; i++) {
            dump_stmt(stmt->as.function.body.items[i], depth + 2, out);
        }
        break;
    case STMT_RETURN:
        if (stmt->as.returned) dump_expr(stmt->as.returned, depth + 1, out);
        break;
    case STMT_CALL: dump_expr(stmt->as.call, depth + 1, out); break;
    case STMT_CONSTANT: dump_expr(stmt->as.constant.value, depth + 1, out); break;
    case STMT_LIST:
        // In the order they're written: "set item P of L to V" ends with its
        // value, the others start with it.
        if (stmt->as.list.op != LIST_SET_ITEM) dump_section("value", stmt->as.list.value, depth + 1, out);
        dump_section("position", stmt->as.list.position, depth + 1, out);
        dump_section("list", stmt->as.list.list, depth + 1, out);
        if (stmt->as.list.op == LIST_SET_ITEM) dump_section("value", stmt->as.list.value, depth + 1, out);
        break;
    }
}

void ast_dump_block(const Block *block, StrBuf *out) {
    for (size_t i = 0; i < block->len; i++) {
        dump_stmt(block->items[i], 0, out);
    }
}
