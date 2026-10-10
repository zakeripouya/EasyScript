#include <string.h>
#include "front/check.h"

typedef struct {
    const char *name;
    size_t line;  // where it was made
} Symbol;

typedef struct {
    Arena *arena;
    Diag *diag;
    Vec(Symbol) made;    // names made so far, in order
    Vec(Symbol) later;   // every name the program makes anywhere, for "you make it later"
} Checker;

static const Symbol *find(const Symbol *items, size_t len, const char *name) {
    for (size_t i = 0; i < len; i++) {
        if (strcmp(items[i].name, name) == 0) return &items[i];
    }
    return NULL;
}

static const Symbol *find_made(const Checker *c, const char *name) {
    return find(c->made.items, c->made.len, name);
}

static void make(Checker *c, const Name *name) {
    if (find_made(c, name->text)) return;
    Symbol symbol = {name->text, name->pos.line};
    vec_push(c->arena, &c->made, symbol);
}

// The made names closest to `name` by spelling; *count is how many tied.
static const Symbol *closest(const Checker *c, const char *name, size_t *count) {
    size_t limit = strlen(name) <= 3 ? 1 : 2;
    size_t best = limit + 1;
    const Symbol *found = NULL;
    *count = 0;
    for (size_t i = 0; i < c->made.len; i++) {
        size_t d = edit_distance(c->arena, name, c->made.items[i].name);
        if (d < best) {
            best = d;
            found = &c->made.items[i];
            *count = 1;
        } else if (d == best) {
            (*count)++;
        }
    }
    return found;
}

static void note_choices(Checker *c, const char *name, size_t limit) {
    StrBuf choices;
    sb_init(&choices, c->arena);
    size_t total = 0;
    for (size_t i = 0; i < c->made.len; i++) {
        if (edit_distance(c->arena, name, c->made.items[i].name) == limit) total++;
    }
    size_t n = 0;
    for (size_t i = 0; i < c->made.len; i++) {
        if (edit_distance(c->arena, name, c->made.items[i].name) != limit) continue;
        n++;
        if (n > 1) sb_append(&choices, n == total ? " or " : ", ");
        sb_appendf(&choices, "\"%s\"", c->made.items[i].name);
    }
    diag_note(c->diag, "Did you mean %s?", choices.data);
}

static void report_unknown(Checker *c, const char *name, SourcePos pos) {
    diag_error(c->diag, pos.span, "I don't know anything called \"%s\".", name);
    size_t count;
    const Symbol *near = closest(c, name, &count);
    const Symbol *later = find(c->later.items, c->later.len, name);
    if (near && count == 1) {
        diag_note(c->diag, "Did you mean \"%s\"? You made it on line %zu.", near->name, near->line);
    } else if (near) {
        note_choices(c, name, edit_distance(c->arena, name, near->name));
    } else if (later && later->line > pos.line) {
        diag_note(c->diag, "You make \"%s\" later, on line %zu. Make it before you use it.", name, later->line);
    } else {
        diag_note(c->diag, "Make it first, like \"let %s be 0\".", name);
    }
}

static void check_target(Checker *c, const Name *name) {
    if (!find_made(c, name->text)) report_unknown(c, name->text, name->pos);
}

static void check_expr(Checker *c, const Expr *expr) {
    switch (expr->kind) {
    case EXPR_NAME:
        if (!find_made(c, expr->as.name)) report_unknown(c, expr->as.name, expr->pos);
        break;
    case EXPR_CALL:
        diag_error(c->diag, expr->pos.span, "I don't know a function called \"%s\".", expr->as.call.name);
        diag_note(c->diag, "Making your own functions isn't available yet.");
        break;
    case EXPR_IT:
        diag_error(c->diag, expr->pos.span, "\"it\" doesn't refer to anything here.");
        diag_note(c->diag, "Use the name of a variable instead.");
        break;
    case EXPR_LENGTH:
    case EXPR_FILE_CONTENTS: check_expr(c, expr->as.operand); break;
    case EXPR_UNARY: check_expr(c, expr->as.unary.operand); break;
    case EXPR_BINARY:
        check_expr(c, expr->as.binary.left);
        check_expr(c, expr->as.binary.right);
        break;
    case EXPR_CONVERT: check_expr(c, expr->as.convert.operand); break;
    default: break;
    }
}

static void check_let(Checker *c, const Stmt *stmt) {
    const Name *name = &stmt->as.assign.name;
    check_expr(c, stmt->as.assign.value);
    const Symbol *existing = find_made(c, name->text);
    if (existing) {
        diag_error(c->diag, name->pos.span, "You already made \"%s\" on line %zu.", name->text, existing->line);
        diag_note(c->diag, "To change it, write \"set %s to ...\".", name->text);
        return;
    }
    make(c, name);
}

static void check_stmt(Checker *c, const Stmt *stmt) {
    switch (stmt->kind) {
    case STMT_LET: check_let(c, stmt); break;
    case STMT_SET:
        check_expr(c, stmt->as.assign.value);
        check_target(c, &stmt->as.assign.name);
        break;
    case STMT_CHANGE:
        check_expr(c, stmt->as.change.amount);
        check_target(c, &stmt->as.change.target);
        break;
    case STMT_SAY: check_expr(c, stmt->as.value); break;
    case STMT_ASK:
        check_expr(c, stmt->as.ask.prompt);
        make(c, &stmt->as.ask.answer);
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE:
        check_expr(c, stmt->as.file_write.text);
        check_expr(c, stmt->as.file_write.path);
        break;
    case STMT_READ_FILE:
        check_expr(c, stmt->as.read_file.path);
        make(c, &stmt->as.read_file.name);
        break;
    case STMT_STOP: break;
    }
}

// Every name the program makes, with the line of its first "let", "ask", or "read".
static void collect_names(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        const Stmt *stmt = program->items[i];
        const Name *name = stmt->kind == STMT_LET    ? &stmt->as.assign.name
                           : stmt->kind == STMT_ASK  ? &stmt->as.ask.answer
                           : stmt->kind == STMT_READ_FILE ? &stmt->as.read_file.name
                                                          : NULL;
        if (name && !find(c->later.items, c->later.len, name->text)) {
            Symbol symbol = {name->text, name->pos.line};
            vec_push(c->arena, &c->later, symbol);
        }
    }
}

void check_program(Arena *arena, Diag *diag, const Block *program) {
    Checker c = {0};
    c.arena = arena;
    c.diag = diag;
    collect_names(&c, program);
    for (size_t i = 0; i < program->len; i++) {
        check_stmt(&c, program->items[i]);
    }
}
