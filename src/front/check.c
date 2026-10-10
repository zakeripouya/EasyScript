// The checker: names (made before use, made once, in scope), functions and
// constants, and the kind of every value (with check_expr.c and
// check_types.c). Constants are worked out first, then every function body,
// then the main program, so a call's mistakes are reported at the call;
// errors are sorted into source order at the end.

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

const Symbol *checker_find_later(const Checker *c, const char *name) {
    return c->later.len ? find(c->later.items, c->later.len, name) : NULL;
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

static void make(Checker *c, const Name *name, TypeVar var) {
    if (checker_find_made(c, name->text) || checker_clashes(c, name)) return;
    Symbol symbol = {name->text, name->pos.line, NULL, false, var};
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
    const Symbol *outside = c->function ? find(c->later.items, c->later.len, name) : NULL;
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

// The variable a statement changes, or NULL (reported) if there isn't one.
static const Symbol *check_target(Checker *c, const Name *name) {
    const Constant *k = checker_find_constant(c, name->text);
    if (k) {
        diag_error(c->diag, name->pos.span, "\"%s\" is a constant, so it can't change.", name->text);
        diag_note(c->diag, "It's kept on line %zu. If it needs to change, make it with \"let %s be ...\" instead of "
                           "\"keep\".", k->definition->pos.line, name->text);
        return NULL;
    }
    const Symbol *symbol = checker_find_made(c, name->text);
    if (!symbol) checker_report_unknown(c, name->text, name->pos);
    return symbol;
}

// A variable keeps the kind it was made with.
static void store(Checker *c, const Symbol *symbol, TypeVar value, Span span) {
    if (types_unify(c, symbol->var, value)) return;
    diag_error(c->diag, span, "\"%s\" is %s (made on line %zu), but this is %s.", symbol->name,
               ast_type_name(types_of(c, symbol->var)), symbol->line, ast_type_name(types_of(c, value)));
    diag_note(c->diag, "A variable keeps the kind of value it was made with. To keep %s, make a new variable for it.",
              ast_type_name(types_of(c, value)));
}

static void check_set(Checker *c, const Stmt *stmt) {
    TypeVar value = checker_check_expr(c, stmt->as.assign.value);
    const Symbol *symbol = check_target(c, &stmt->as.assign.name);
    if (symbol) store(c, symbol, value, stmt->as.assign.value->pos.span);
}

// add/subtract/increase/decrease/multiply/divide: the runtime's arithmetic
// rules, with the variable on the left (add E to X is X plus E).
static void check_change(Checker *c, const Stmt *stmt) {
    static const BinaryOp ops[] = {BINARY_ADD, BINARY_SUBTRACT, BINARY_MULTIPLY, BINARY_DIVIDE};
    TypeVar amount = checker_check_expr(c, stmt->as.change.amount);
    const Symbol *symbol = check_target(c, &stmt->as.change.target);
    if (symbol) checker_arithmetic(c, ops[stmt->as.change.op], symbol->var, amount, stmt->pos.span, stmt->pos.line);
}

// "ask ... and call the answer X" and "read file ... and call it X" make X as
// text, or store text in it.
static void make_text(Checker *c, const Name *name) {
    TypeVar text = types_new(c, TYPE_TEXT, name->pos.line);
    const Symbol *existing = checker_find_made(c, name->text);
    if (existing) {
        store(c, existing, text, name->pos.span);
    } else {
        make(c, name, text);
    }
}

static void check_let(Checker *c, const Stmt *stmt) {
    const Name *name = &stmt->as.assign.name;
    TypeVar value = checker_check_expr(c, stmt->as.assign.value);
    const Symbol *existing = checker_find_made(c, name->text);
    if (existing) {
        diag_error(c->diag, name->pos.span, "You already made \"%s\" on line %zu.", name->text, existing->line);
        diag_note(c->diag, "To change it, write \"set %s to ...\".", name->text);
        return;
    }
    make(c, name, value);
}

// A function gives back one kind of value, or nothing, everywhere.
static void check_return(Checker *c, const Stmt *stmt) {
    TypeVar value = stmt->as.returned ? checker_check_expr(c, stmt->as.returned) : 0;
    const Function *f = c->function;
    if (!f) {
        diag_error(c->diag, stmt->pos.span, "\"give back\" only works inside a function.");
        diag_note(c->diag, "It ends a function and hands a value back to whoever called it. "
                           "To end the whole program, write \"stop the program\".");
        return;
    }
    Type result = types_of(c, f->result);
    if (!stmt->as.returned) {
        if (result == TYPE_NOTHING || result == TYPE_ERROR) return;
        diag_error(c->diag, stmt->pos.span, "\"%s\" gives back a value, so it can't give back nothing here.", f->name);
        diag_note(c->diag, "Give back a value here too, like \"give back 0\".");
    } else if (result == TYPE_NOTHING) {
        diag_error(c->diag, stmt->as.returned->pos.span, "\"%s\" gives back nothing, so it can't give back a value.",
                   f->name);
        diag_note(c->diag, "It's defined on line %zu: \"%s\".", f->definition->pos.line, checker_function_header(c, f));
    } else if (!types_unify(c, f->result, value)) {
        diag_error(c->diag, stmt->as.returned->pos.span, "\"%s\" gives back %s (since line %zu), but this is %s.",
                   f->name, ast_type_name(result), types_line(c, f->result), ast_type_name(types_of(c, value)));
        diag_note(c->diag, "A function gives back the same kind of value every time.");
    }
}

static void check_stmt(Checker *c, const Stmt *stmt);

// A block's statements in a scope of their own: names made inside are
// forgotten (and remembered as "ended") when it ends.
// Ends a scope opened when made.len was `outer`: its names are forgotten,
// and remembered as "ended" in `where` on `line`.
static void end_scope(Checker *c, size_t outer, size_t line, const char *where) {
    for (size_t i = outer; where && i < c->made.len; i++) {
        Symbol ended = {c->made.items[i].name, line, where, c->made.items[i].loop_number, 0};
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

static void check_params(Checker *c, const Function *f) {
    const Stmt *stmt = f->definition;
    for (size_t i = 0; i < stmt->as.function.params.len; i++) {
        const Name *param = &stmt->as.function.params.items[i];
        if (checker_find_made(c, param->text)) {
            diag_error(c->diag, param->pos.span, "This function already has an input called \"%s\".", param->text);
            diag_note(c->diag, "Give each input its own name.");
        } else {
            make(c, param, f->params.items[i]);
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
    const Function *f = checker_find_function(c, stmt->as.function.name.text);
    if (!f || f->definition != stmt) return;  // a duplicate, already reported
    size_t saved_floor = c->floor;
    size_t saved_loops = c->loops.len;
    c->floor = c->made.len;
    c->loops.len = 0;
    c->function = f;
    check_params(c, f);
    check_statements(c, &stmt->as.function.body);
    end_scope(c, c->floor, stmt->pos.line, NULL);
    c->function = NULL;
    c->floor = saved_floor;
    c->loops.len = saved_loops;
}

static void check_if(Checker *c, const Stmt *stmt) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        if (branch->condition) {
            checker_require(c, checker_check_expr(c, branch->condition), TYPE_YESNO, branch->condition,
                            "An \"if\" needs yes or no to decide", "Compare it with something, like \"if x is 5\".");
        }
        check_block(c, &branch->body, stmt->pos.line, "the \"if\"");
    }
}

static void check_loop_variable(Checker *c, const Stmt *stmt) {
    const Name *var = &stmt->as.loop.var;
    const Symbol *existing = checker_find_made(c, var->text);
    if (!existing && checker_clashes(c, var)) return;
    if (!existing) {
        Symbol symbol = {var->text, var->pos.line, NULL, true, types_new(c, TYPE_NUMBER, var->pos.line)};
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
static void check_loop_value(Checker *c, const Expr *value, Type type, const char *message, const char *hint) {
    if (value) checker_require(c, checker_check_expr(c, value), type, value, message, hint);
}

static void check_loop(Checker *c, const Stmt *stmt) {
    check_loop_value(c, stmt->as.loop.from, TYPE_NUMBER, "A count has to start at a number", NULL);
    check_loop_value(c, stmt->as.loop.to, TYPE_NUMBER, "A count has to end at a number", NULL);
    check_loop_value(c, stmt->as.loop.step, TYPE_NUMBER, "The step of a count has to be a number", NULL);
    check_loop_value(c, stmt->as.loop.times, TYPE_NUMBER, "The number of times has to be a number", NULL);
    check_loop_value(c, stmt->as.loop.condition, TYPE_YESNO,
                     "A loop needs yes or no to decide whether to keep going",
                     "Compare it with something, like \"while count is less than 10\".");
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
    case STMT_SET: check_set(c, stmt); break;
    case STMT_CHANGE: check_change(c, stmt); break;
    case STMT_SAY: checker_check_expr(c, stmt->as.value); break;
    case STMT_ASK:
        checker_check_expr(c, stmt->as.ask.prompt);
        make_text(c, &stmt->as.ask.answer);
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE:
        checker_check_expr(c, stmt->as.file_write.text);
        checker_require(c, checker_check_expr(c, stmt->as.file_write.path), TYPE_TEXT, stmt->as.file_write.path,
                        "The name of a file has to be text", NULL);
        break;
    case STMT_READ_FILE:
        checker_require(c, checker_check_expr(c, stmt->as.read_file.path), TYPE_TEXT, stmt->as.read_file.path,
                        "The name of a file has to be text", NULL);
        make_text(c, &stmt->as.read_file.name);
        break;
    case STMT_STOP: break;
    case STMT_IF: check_if(c, stmt); break;
    case STMT_LOOP: check_loop(c, stmt); break;
    case STMT_BREAK:
    case STMT_CONTINUE: check_loop_control(c, stmt); break;
    case STMT_FUNCTION: check_function(c, stmt); break;
    case STMT_RETURN: check_return(c, stmt); break;
    case STMT_CALL: checker_check_call(c, stmt->as.call, false); break;
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
            Symbol symbol = {name->text, name->pos.line, NULL, false, 0};
            vec_push(c->arena, &c->later, symbol);
        }
    }
}

// Whether a function's body gives back a value anywhere ("give back E").
static bool gives_value(const Block *block) {
    for (size_t i = 0; i < block->len; i++) {
        const Stmt *stmt = block->items[i];
        if (stmt->kind == STMT_RETURN && stmt->as.returned) return true;
        if (stmt->kind == STMT_LOOP && gives_value(&stmt->as.loop.body)) return true;
        for (size_t b = 0; stmt->kind == STMT_IF && b < stmt->as.if_stmt.branches.len; b++) {
            if (gives_value(&stmt->as.if_stmt.branches.items[b].body)) return true;
        }
    }
    return false;
}

// A function's kinds: written in its definition, or left for inference. One
// that never gives back a value gives back nothing.
static void function_types(Checker *c, Function *f) {
    const Stmt *def = f->definition;
    size_t line = def->pos.line;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        Type declared = def->as.function.declared.items[i];
        vec_push(c->arena, &f->params, types_new(c, declared, declared ? line : 0));
    }
    Type result = def->as.function.declared_result;
    if (result == TYPE_UNKNOWN && !gives_value(&def->as.function.body)) result = TYPE_NOTHING;
    f->result = types_new(c, result, result ? line : 0);
}

// Every top-level function, so calls can come before definitions.
static void collect_functions(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        Stmt *stmt = program->items[i];
        if (stmt->kind != STMT_FUNCTION) continue;
        const Name *name = &stmt->as.function.name;
        const Function *existing = checker_find_function(c, name->text);
        if (existing) {
            diag_error(c->diag, name->pos.span, "You already defined \"%s\" on line %zu.", name->text,
                       existing->definition->pos.line);
            diag_note(c->diag, "Each function needs its own name.");
            continue;
        }
        Function f = {name->text, stmt, {0}, 0};
        function_types(c, &f);
        vec_push(c->arena, &c->functions, f);
    }
}

void check_program(Arena *arena, Diag *diag, const Block *program) {
    Checker c = {0};
    c.arena = arena;
    c.diag = diag;
    size_t first_error = diag_count(diag);
    collect_functions(&c, program);
    checker_collect_constants(&c, program);
    collect_names(&c, program);
    for (size_t pass = 0; pass < 3; pass++) {  // constants, then functions, then the rest
        for (size_t i = 0; i < program->len; i++) {
            const Stmt *stmt = program->items[i];
            int kind_pass = stmt->kind == STMT_CONSTANT ? 0 : stmt->kind == STMT_FUNCTION ? 1 : 2;
            if (kind_pass == (int)pass) check_stmt(&c, stmt);
        }
    }
    types_finish(&c, first_error);
    diag_sort(diag, first_error);
}
