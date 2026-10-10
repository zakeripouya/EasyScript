#include <string.h>
#include "front/check_internal.h"


static const Symbol *find(const Symbol *items, size_t len, const char *name) {
    for (size_t i = 0; i < len; i++) {
        if (strcmp(items[i].name, name) == 0) return &items[i];
    }
    return NULL;
}

const Symbol *checker_find_made(const Checker *c, const char *name) {
    if (c->made.len == c->floor) return NULL;  // also avoids NULL + 0 on an empty list
    return find(c->made.items + c->floor, c->made.len - c->floor, name);
}

const Function *checker_find_function(const Checker *c, const char *name) {
    for (size_t i = 0; i < c->functions.len; i++) {
        if (strcmp(c->functions.items[i].name, name) == 0) return &c->functions.items[i];
    }
    return NULL;
}

const Constant *checker_find_constant(const Checker *c, const char *name) {
    for (size_t i = 0; i < c->constants.len; i++) {
        if (strcmp(c->constants.items[i].name, name) == 0) return &c->constants.items[i];
    }
    return NULL;
}

static size_t param_count(const Function *f) {
    return f->definition->as.function.params.len;
}

// A variable can't share a name with a function or a constant. Reports and
// returns true if it does.
bool checker_clashes(Checker *c, const Name *name) {
    const Constant *k = checker_find_constant(c, name->text);
    if (k) {
        diag_error(c->diag, name->pos.span, "\"%s\" is a constant, made on line %zu.", name->text,
                   k->definition->pos.line);
        diag_note(c->diag, "Give the variable a different name.");
        return true;
    }
    const Function *f = checker_find_function(c, name->text);
    if (!f) return false;
    diag_error(c->diag, name->pos.span, "\"%s\" is the name of a function, defined on line %zu.", name->text,
               f->definition->pos.line);
    diag_note(c->diag, "Give the variable a different name.");
    return true;
}

static void make(Checker *c, const Name *name) {
    if (checker_find_made(c, name->text) || checker_clashes(c, name)) return;
    Symbol symbol = {name->text, name->pos.line, NULL, false};
    vec_push(c->arena, &c->made, symbol);
}

// The made names closest to `name` by spelling; *count is how many tied.
static const Symbol *closest(const Checker *c, const char *name, size_t *count) {
    size_t limit = strlen(name) <= 3 ? 1 : 2;
    size_t best = limit + 1;
    const Symbol *found = NULL;
    *count = 0;
    for (size_t i = c->floor; i < c->made.len; i++) {
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
    for (size_t i = c->floor; i < c->made.len; i++) {
        if (edit_distance(c->arena, name, c->made.items[i].name) == limit) total++;
    }
    size_t n = 0;
    for (size_t i = c->floor; i < c->made.len; i++) {
        if (edit_distance(c->arena, name, c->made.items[i].name) != limit) continue;
        n++;
        if (n > 1) sb_append(&choices, n == total ? " or " : ", ");
        sb_appendf(&choices, "\"%s\"", c->made.items[i].name);
    }
    diag_note(c->diag, "Did you mean %s?", choices.data);
}


void checker_report_unknown(Checker *c, const char *name, SourcePos pos) {
    diag_error(c->diag, pos.span, "I don't know anything called \"%s\".", name);
    size_t count;
    const Symbol *near = closest(c, name, &count);
    const Symbol *later = find(c->later.items, c->later.len, name);
    const Symbol *ended = find(c->ended.items, c->ended.len, name);
    const Symbol *outside = c->function ? find(c->made.items, c->floor, name) : NULL;
    const Constant *near_constant = checker_closest_constant(c, name);
    if (outside) {
        diag_note(c->diag, "A function only sees its own inputs and the names it makes. To use \"%s\" here, "
                           "pass it in as an input.", name);
    } else if (ended && ended->loop_number) {
        diag_note(c->diag, "\"%s\" is the number of the loop on line %zu, so it only exists inside that loop. "
                           "To keep it, make a variable before the loop and set it inside.", name, ended->line);
    } else if (ended) {
        diag_note(c->diag, "You made \"%s\" inside %s on line %zu, so it only exists inside that block. "
                           "To use it afterwards, make it before %s.", name, ended->where, ended->line, ended->where);
    } else if (near_constant && !near) {
        diag_note(c->diag, "Did you mean \"%s\"? It's a constant, kept on line %zu.", near_constant->name,
                  near_constant->definition->pos.line);
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
    const Constant *k = checker_find_constant(c, name->text);
    if (k) {
        diag_error(c->diag, name->pos.span, "\"%s\" is a constant, so it can't change.", name->text);
        diag_note(c->diag, "It's kept on line %zu. If it needs to change, make it with \"let %s be ...\" instead of "
                           "\"keep\".", k->definition->pos.line, name->text);
        return;
    }
    if (!checker_find_made(c, name->text)) checker_report_unknown(c, name->text, name->pos);
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


// "to area with width and height"
static const char *function_header(const Checker *c, const Function *f) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_appendf(&sb, "to %s", f->name);
    const Stmt *def = f->definition;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        sb_append(&sb, i == 0 ? " with " : " and ");
        sb_append(&sb, def->as.function.params.items[i].text);
    }
    return sb.data;
}

static void report_unknown_function(Checker *c, const char *name, SourcePos pos) {
    if (checker_find_constant(c, name)) {
        diag_error(c->diag, pos.span, "\"%s\" is a constant, not a function.", name);
        diag_note(c->diag, "Only functions made with \"to %s ...:\" can be called.", name);
        return;
    }
    if (checker_find_made(c, name)) {
        diag_error(c->diag, pos.span, "\"%s\" is a variable, not a function.", name);
        diag_note(c->diag, "Only functions made with \"to %s ...:\" can be called.", name);
        return;
    }
    diag_error(c->diag, pos.span, "I don't know a function called \"%s\".", name);
    size_t limit = strlen(name) <= 3 ? 1 : 2;
    const Function *best = NULL;
    size_t best_distance = limit + 1;
    for (size_t i = 0; i < c->functions.len; i++) {
        size_t d = edit_distance(c->arena, name, c->functions.items[i].name);
        if (d < best_distance) {
            best_distance = d;
            best = &c->functions.items[i];
        }
    }
    if (best) {
        diag_note(c->diag, "Did you mean \"%s\"? It's defined on line %zu.", best->name, best->definition->pos.line);
    } else {
        diag_note(c->diag, "Define it first, like \"to %s someone:\" with its sentences indented below.", name);
    }
}

static void report_argument_count(Checker *c, const Function *f, size_t given, SourcePos pos) {
    size_t wanted = param_count(f);
    if (wanted == 0) {
        diag_error(c->diag, pos.span, "\"%s\" doesn't take any values, but this gives it %zu.", f->name, given);
    } else {
        diag_error(c->diag, pos.span, "\"%s\" needs %zu value%s, but this gives it %zu.", f->name, wanted,
                   wanted == 1 ? "" : "s", given);
    }
    diag_note(c->diag, "It's defined on line %zu: \"%s\".", f->definition->pos.line, function_header(c, f));
}

static void check_call(Checker *c, const Expr *expr) {
    for (size_t i = 0; i < expr->as.call.args.len; i++) {
        checker_check_expr(c, expr->as.call.args.items[i]);
    }
    const Function *f = checker_find_function(c, expr->as.call.name);
    if (!f) {
        report_unknown_function(c, expr->as.call.name, expr->pos);
    } else if (param_count(f) != expr->as.call.args.len) {
        report_argument_count(c, f, expr->as.call.args.len, expr->pos);
    }
}

// A bare name: a variable, or a function that takes no values.
static void check_name_expr(Checker *c, const Expr *expr) {
    if (checker_find_made(c, expr->as.name) || checker_find_constant(c, expr->as.name)) return;
    const Function *f = checker_find_function(c, expr->as.name);
    if (!f) {
        checker_report_unknown(c, expr->as.name, expr->pos);
    } else if (param_count(f) > 0) {
        report_argument_count(c, f, 0, expr->pos);
    }
}

void checker_check_expr(Checker *c, const Expr *expr) {
    switch (expr->kind) {
    case EXPR_NAME: check_name_expr(c, expr); break;
    case EXPR_CALL: check_call(c, expr); break;
    case EXPR_IT: check_it(c, expr); break;
    case EXPR_LENGTH:
    case EXPR_FILE_CONTENTS: checker_check_expr(c, expr->as.operand); break;
    case EXPR_UNARY: checker_check_expr(c, expr->as.unary.operand); break;
    case EXPR_BINARY:
        checker_check_expr(c, expr->as.binary.left);
        checker_check_expr(c, expr->as.binary.right);
        break;
    case EXPR_CONVERT: checker_check_expr(c, expr->as.convert.operand); break;
    default: break;
    }
}

static void check_let(Checker *c, const Stmt *stmt) {
    const Name *name = &stmt->as.assign.name;
    checker_check_expr(c, stmt->as.assign.value);
    const Symbol *existing = checker_find_made(c, name->text);
    if (existing) {
        diag_error(c->diag, name->pos.span, "You already made \"%s\" on line %zu.", name->text, existing->line);
        diag_note(c->diag, "To change it, write \"set %s to ...\".", name->text);
        return;
    }
    make(c, name);
}

static void check_return(Checker *c, const Stmt *stmt) {
    if (stmt->as.returned) checker_check_expr(c, stmt->as.returned);
    if (c->function) return;
    diag_error(c->diag, stmt->pos.span, "\"give back\" only works inside a function.");
    diag_note(c->diag, "It ends a function and hands a value back to whoever called it. "
                       "To end the whole program, write \"stop the program\".");
}

static void check_stmt(Checker *c, const Stmt *stmt);

// A block's statements in a scope of their own: names made inside are
// forgotten (and remembered as "ended") when it ends.
// Ends a scope opened when made.len was `outer`: its names are forgotten,
// and remembered as "ended" in `where` on `line`.
static void end_scope(Checker *c, size_t outer, size_t line, const char *where) {
    for (size_t i = outer; where && i < c->made.len; i++) {
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
    c->blocks++;
    check_statements(c, block);
    c->blocks--;
    end_scope(c, outer, line, where);
}

static void check_params(Checker *c, const Stmt *stmt) {
    for (size_t i = 0; i < stmt->as.function.params.len; i++) {
        const Name *param = &stmt->as.function.params.items[i];
        if (checker_find_made(c, param->text)) {
            diag_error(c->diag, param->pos.span, "This function already has an input called \"%s\".", param->text);
            diag_note(c->diag, "Give each input its own name.");
        } else {
            make(c, param);
        }
    }
}

// A function's body sees only its inputs and the names it makes; loops and
// blocks outside it don't count.
static void check_function(Checker *c, const Stmt *stmt) {
    if (c->function || c->blocks > 0) {
        diag_error(c->diag, stmt->as.function.name.pos.span,
                   "Functions can only be defined at the top level, not inside %s.",
                   c->function ? "another function" : "an \"if\" or a loop");
        diag_note(c->diag, "Move \"to %s ...:\" to the start of a line, outside every block.",
                  stmt->as.function.name.text);
        return;
    }
    size_t saved_floor = c->floor;
    size_t saved_loops = c->loops.len;
    c->floor = c->made.len;
    c->loops.len = 0;
    c->function = stmt;
    check_params(c, stmt);
    check_statements(c, &stmt->as.function.body);
    end_scope(c, c->floor, stmt->pos.line, NULL);
    c->function = NULL;
    c->floor = saved_floor;
    c->loops.len = saved_loops;
}

static void check_if(Checker *c, const Stmt *stmt) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        if (branch->condition) checker_check_expr(c, branch->condition);
        check_block(c, &branch->body, stmt->pos.line, "the \"if\"");
    }
}

static void check_loop_variable(Checker *c, const Stmt *stmt) {
    const Name *var = &stmt->as.loop.var;
    const Symbol *existing = checker_find_made(c, var->text);
    if (!existing && checker_clashes(c, var)) return;
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
        if (values[i]) checker_check_expr(c, values[i]);
    }
    size_t outer = c->made.len;
    if (stmt->as.loop.kind == LOOP_COUNT) check_loop_variable(c, stmt);
    Loop loop = {stmt->as.loop.kind == LOOP_COUNT || stmt->as.loop.kind == LOOP_TIMES};
    vec_push(c->arena, &c->loops, loop);
    c->blocks++;
    check_statements(c, &stmt->as.loop.body);
    c->blocks--;
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
        checker_check_expr(c, stmt->as.assign.value);
        check_target(c, &stmt->as.assign.name);
        break;
    case STMT_CHANGE:
        checker_check_expr(c, stmt->as.change.amount);
        check_target(c, &stmt->as.change.target);
        break;
    case STMT_SAY: checker_check_expr(c, stmt->as.value); break;
    case STMT_ASK:
        checker_check_expr(c, stmt->as.ask.prompt);
        make(c, &stmt->as.ask.answer);
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE:
        checker_check_expr(c, stmt->as.file_write.text);
        checker_check_expr(c, stmt->as.file_write.path);
        break;
    case STMT_READ_FILE:
        checker_check_expr(c, stmt->as.read_file.path);
        make(c, &stmt->as.read_file.name);
        break;
    case STMT_STOP: break;
    case STMT_IF: check_if(c, stmt); break;
    case STMT_LOOP: check_loop(c, stmt); break;
    case STMT_BREAK:
    case STMT_CONTINUE: check_loop_control(c, stmt); break;
    case STMT_FUNCTION: check_function(c, stmt); break;
    case STMT_RETURN: check_return(c, stmt); break;
    case STMT_CALL: checker_check_expr(c, stmt->as.call); break;
    case STMT_CONSTANT: checker_check_constant(c, stmt); break;
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
        if (stmt->kind == STMT_FUNCTION) continue;  // its names are its own
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

// Every top-level function, so calls can come before definitions.
static void collect_functions(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        const Stmt *stmt = program->items[i];
        if (stmt->kind != STMT_FUNCTION) continue;
        const Name *name = &stmt->as.function.name;
        const Function *existing = checker_find_function(c, name->text);
        if (existing) {
            diag_error(c->diag, name->pos.span, "You already defined \"%s\" on line %zu.", name->text,
                       existing->definition->pos.line);
            diag_note(c->diag, "Each function needs its own name.");
            continue;
        }
        Function f = {name->text, stmt};
        vec_push(c->arena, &c->functions, f);
    }
}

void check_program(Arena *arena, Diag *diag, const Block *program) {
    Checker c = {0};
    c.arena = arena;
    c.diag = diag;
    collect_functions(&c, program);
    checker_collect_constants(&c, program);
    collect_names(&c, program);
    check_statements(&c, program);
}
