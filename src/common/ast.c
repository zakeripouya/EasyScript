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

const char *ast_binary_op_name(BinaryOp op) {
    static const char *const names[] = {
        "or", "and", "equal", "not equal", "less", "less or equal", "greater",
        "greater or equal", "add", "subtract", "join", "multiply", "divide", "modulo",
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
    }
    return "loop";
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
        }
        sb_append_char(scratch, ')');
        break;
    case STMT_RETURN: return "give back";
    case STMT_CALL: return "call";
    case STMT_CONSTANT: sb_appendf(scratch, "keep %s", stmt->as.constant.name.text); break;
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
    }
}

void ast_dump_block(const Block *block, StrBuf *out) {
    for (size_t i = 0; i < block->len; i++) {
        dump_stmt(block->items[i], 0, out);
    }
}
