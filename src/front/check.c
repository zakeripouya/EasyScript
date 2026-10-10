#include <string.h>
#include "front/check.h"

typedef struct {
    const char *name;
    size_t line;        // where it was made
    const char *where;  // in `ended`: "the \"if\"" or "the loop" that held it
    bool loop_number;   // a loop's own number ("count ... as n")
} Symbol;

typedef struct {
    bool has_it;  // count and times loops have a number that "it" refers to
} Loop;

typedef struct {
    Arena *arena;
    Diag *diag;
    Vec(Symbol) made;    // names that exist here: made so far, in blocks still open
    Vec(Symbol) later;   // every name the program makes anywhere, for "you make it later"
    Vec(Symbol) ended;   // names whose block has ended; line = the if or loop that held them
    Vec(Loop) loops;     // loops the current statement is inside, innermost last
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
    Symbol symbol = {name->text, name->pos.line, NULL, false};
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
    const Symbol *ended = find(c->ended.items, c->ended.len, name);
    if (ended && ended->loop_number) {
        diag_note(c->diag, "\"%s\" is the number of the loop on line %zu, so it only exists inside that loop. "
                           "To keep it, make a variable before the loop and set it inside.", name, ended->line);
    } else if (ended) {
        diag_note(c->diag, "You made \"%s\" inside %s on line %zu, so it only exists inside that block. "
                           "To use it afterwards, make it before %s.", name, ended->where, ended->line, ended->where);
    } else if (near && count == 1) {
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

// "it" is the number of the innermost loop that has one (count and times loops).
static void check_it(Checker *c, const Expr *expr) {
    for (size_t i = c->loops.len; i > 0; i--) {
        if (c->loops.items[i - 1].has_it) return;
    }
    diag_error(c->diag, expr->pos.span, "\"it\" doesn't refer to anything here.");
    if (c->loops.len > 0) {
        diag_note(c->diag, "\"it\" means the number of a counting loop or a \"repeat ... times\" loop, "
                           "but this loop doesn't count. Use a variable instead.");
    } else {
        diag_note(c->diag, "\"it\" only means something inside a loop, where it's the loop's number.");
    }
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
    case EXPR_IT: check_it(c, expr); break;
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

static void check_stmt(Checker *c, const Stmt *stmt);

// A block's statements in a scope of their own: names made inside are
// forgotten (and remembered as "ended") when it ends.
// Ends a scope opened when made.len was `outer`: its names are forgotten,
// and remembered as "ended" in `where` on `line`.
static void end_scope(Checker *c, size_t outer, size_t line, const char *where) {
    for (size_t i = outer; i < c->made.len; i++) {
        Symbol ended = {c->made.items[i].name, line, where, c->made.items[i].loop_number};
        vec_push(c->arena, &c->ended, ended);
    }
    c->made.len = outer;
}

static void check_statements(Checker *c, const Block *block) {
    for (size_t i = 0; i < block->len; i++) {
        check_stmt(c, block->items[i]);
    }
}

static void check_block(Checker *c, const Block *block, size_t line, const char *where) {
    size_t outer = c->made.len;
    check_statements(c, block);
    end_scope(c, outer, line, where);
}

static void check_if(Checker *c, const Stmt *stmt) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        if (branch->condition) check_expr(c, branch->condition);
        check_block(c, &branch->body, stmt->pos.line, "the \"if\"");
    }
}

static void check_loop_variable(Checker *c, const Stmt *stmt) {
    const Name *var = &stmt->as.loop.var;
    const Symbol *existing = find_made(c, var->text);
    if (!existing) {
        Symbol symbol = {var->text, var->pos.line, NULL, true};
        vec_push(c->arena, &c->made, symbol);
        return;
    }
    if (stmt->as.loop.var_named) {
        diag_error(c->diag, var->pos.span, "You already made \"%s\" on line %zu.", var->text, existing->line);
    } else {
        diag_error(c->diag, var->pos.span, "This loop calls each number \"number\", but you already made "
                                           "\"number\" on line %zu.", existing->line);
    }
    diag_note(c->diag, "Give this loop's number its own name, like \"count from 1 to 10 as n\".");
}

// The loop's own values are checked outside it; its number and anything
// made in its body exist only inside.
static void check_loop(Checker *c, const Stmt *stmt) {
    const Expr *values[] = {stmt->as.loop.from, stmt->as.loop.to, stmt->as.loop.step, stmt->as.loop.times,
                            stmt->as.loop.condition};
    for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++) {
        if (values[i]) check_expr(c, values[i]);
    }
    size_t outer = c->made.len;
    if (stmt->as.loop.kind == LOOP_COUNT) check_loop_variable(c, stmt);
    Loop loop = {stmt->as.loop.kind == LOOP_COUNT || stmt->as.loop.kind == LOOP_TIMES};
    vec_push(c->arena, &c->loops, loop);
    check_statements(c, &stmt->as.loop.body);
    c->loops.len--;
    end_scope(c, outer, stmt->pos.line, "the loop");
}

static void check_loop_control(Checker *c, const Stmt *stmt) {
    if (c->loops.len > 0) return;
    if (stmt->kind == STMT_BREAK) {
        diag_error(c->diag, stmt->pos.span, "\"%s\" only works inside a loop.", "stop the loop");
        diag_note(c->diag, "It leaves the loop it's in. To end the whole program, write \"stop the program\".");
    } else {
        diag_error(c->diag, stmt->pos.span, "\"%s\" only works inside a loop.", "skip this one");
        diag_note(c->diag, "It jumps to the next round of the loop it's in.");
    }
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
    case STMT_IF: check_if(c, stmt); break;
    case STMT_LOOP: check_loop(c, stmt); break;
    case STMT_BREAK:
    case STMT_CONTINUE: check_loop_control(c, stmt); break;
    }
}

// Every name the program makes, with the line of its first "let", "ask", or "read".
static void collect_names(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        const Stmt *stmt = program->items[i];
        if (stmt->kind == STMT_IF) {
            for (size_t b = 0; b < stmt->as.if_stmt.branches.len; b++) {
                collect_names(c, &stmt->as.if_stmt.branches.items[b].body);
            }
            continue;
        }
        if (stmt->kind == STMT_LOOP) {
            collect_names(c, &stmt->as.loop.body);
            continue;
        }
        const Name *name = stmt->kind == STMT_LET    ? &stmt->as.assign.name
                           : stmt->kind == STMT_ASK  ? &stmt->as.ask.answer
                           : stmt->kind == STMT_READ_FILE ? &stmt->as.read_file.name
                                                          : NULL;
        if (name && !find(c->later.items, c->later.len, name->text)) {
            Symbol symbol = {name->text, name->pos.line, NULL, false};
            vec_push(c->arena, &c->later, symbol);
        }
    }
}

void check_program(Arena *arena, Diag *diag, const Block *program) {
    Checker c = {0};
    c.arena = arena;
    c.diag = diag;
    collect_names(&c, program);
    check_statements(&c, program);
}
