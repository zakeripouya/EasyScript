// The checker: names (made before use, made once, in scope), functions and
// constants, and the kind of every value (with check_expr.c and
// check_types.c). Constants are worked out first, then every function body,
// then the main program, so a call's mistakes are reported at the call;
// errors are sorted into source order at the end.

#include <string.h>
#include "front/check_internal.h"

const char *checker_type_name(Checker *c, Type type) {
    return ast_type_name(c->arena, c->records, type);
}

const char *checker_kind(Checker *c, TypeVar var) {
    return checker_type_name(c, types_of(c, var));
}


// The variable a statement changes, or NULL (reported) if there isn't one.
static const Symbol *check_target(Checker *c, const Name *name) {
    const Constant *k = checker_find_constant(c, name->text);
    if (k) {
        diag_error(c->diag, name->pos.span, "\"%s\" is a constant, so it can't change.", k->display);
        diag_note(c->diag, "It's kept on line %zu. If it needs to change, make it with \"let %s be ...\" instead of "
                           "\"keep\".", k->definition->pos.line, k->display);
        return NULL;
    }
    const Symbol *symbol = checker_find_made(c, name->text);
    if (!symbol) checker_report_unknown(c, name->text, name->display, name->pos);
    if (symbol) checker_use(c, symbol, true);
    return symbol;
}

// A variable keeps the kind it was made with.
static void store(Checker *c, const Symbol *symbol, TypeVar value, const Expr *expr, Span span) {
    if (checker_fits(c, symbol->var, value, expr, diag_line(c->diag, span.offset))) return;
    diag_error(c->diag, span, "\"%s\" is %s (made on line %zu), but this is %s.", symbol->display,
               checker_kind(c, symbol->var), symbol->line, checker_kind(c, value));
    diag_note(c->diag, "A name keeps the kind of value it was made with. To keep %s, make a new name for it.",
              checker_kind(c, value));
}

// The field a sentence changes ("set rex's age to"): its kind, as defined.
static TypeVar check_field_target(Checker *c, const Expr *field) {
    const Expr *saved = c->unnarrowed;
    c->unnarrowed = field;
    c->setting_field = true;
    TypeVar var = checker_check_expr(c, field);
    c->setting_field = false;
    c->unnarrowed = saved;
    return var;
}

// "let s be a new circle ...": s is known to be a circle until it's set.
static void narrow_new(Checker *c, const char *name, const Expr *value) {
    if (value->kind == EXPR_NEW && type_is_sum(value->as.new_record.type)) {
        checker_add_narrowing(c, name, value->as.new_record.variant);
    }
}

static void check_set(Checker *c, const Stmt *stmt) {
    const Expr *value_expr = stmt->as.assign.value;
    TypeVar value = checker_check_value(c, value_expr);
    const Expr *field = stmt->as.assign.field;
    if (field && field->kind == EXPR_MAP_GET) {
        checker_check_map_set(c, field, value_expr, value);
        return;
    }
    if (field) {
        TypeVar var = check_field_target(c, field);
        c->storing_field = true;
        bool fits = checker_fits(c, var, value, value_expr, value_expr->pos.line);
        c->storing_field = false;
        if (!fits) {
            diag_error(c->diag, value_expr->pos.span, "The field \"%s\" is %s, but this is %s.",
                       field->as.field.name.display, checker_kind(c, var), checker_kind(c, value));
        }
        checker_kill_path(c, field, NULL);
        return;
    }
    const Symbol *symbol = check_target(c, &stmt->as.assign.name);
    if (symbol) {
        store(c, symbol, value, value_expr, value_expr->pos.span);
        TypedSlot slot = {(Type *)&stmt->as.assign.type, symbol->var};
        vec_push(c->arena, &c->slots, slot);
    }
    checker_kill_path(c, NULL, stmt->as.assign.name.text);
    narrow_new(c, stmt->as.assign.name.text, value_expr);
}

// add/subtract/increase/decrease/multiply/divide: the runtime's arithmetic
// rules, with the variable on the left (add E to X is X plus E).
static void check_change(Checker *c, const Stmt *stmt) {
    static const BinaryOp ops[] = {BINARY_ADD, BINARY_SUBTRACT, BINARY_MULTIPLY, BINARY_DIVIDE};
    TypeVar amount = checker_check_expr(c, stmt->as.change.amount);
    const Expr *field = stmt->as.change.field;
    TypeVar var;
    if (field) {
        var = field->kind == EXPR_MAP_GET ? checker_check_map_change(c, field) : check_field_target(c, field);
        if (types_of(c, var) == TYPE_ERROR) return;
        const Expr *start = stmt->as.change.start;
        TypeVar start_var = start ? checker_check_expr(c, start) : 0;
        if (start && !types_unify(c, var, start_var)) {
            diag_error(c->diag, start->pos.span, "The map's values are %s, so a value can't start at %s.",
                       checker_kind(c, var), checker_kind(c, start_var));
        }
    } else {
        const Symbol *symbol = check_target(c, &stmt->as.change.target);
        if (!symbol) return;
        var = symbol->var;
    }
    if (stmt->as.change.op == CHANGE_ADD && type_is_list(types_of(c, var))) {
        checker_add_to_list(c, (Stmt *)stmt, var, amount);
        return;
    }
    TypedSlot slot = {(Type *)&stmt->as.change.type, var};
    vec_push(c->arena, &c->slots, slot);
    size_t before = diag_count(c->diag);
    TypeVar result = checker_arithmetic(c, ops[stmt->as.change.op], var, amount, stmt->pos.span, stmt->pos.line);
    if (diag_count(c->diag) == before && !types_unify(c, var, result)) {  // "divide price by $2": a number
        diag_error(c->diag, stmt->pos.span, "This gives %s, so it can't be kept where %s is.", checker_kind(c, result),
                   checker_kind(c, var));
        diag_note(c->diag, "Keep it in a new name instead, like \"let share be price divided by $2\".");
        diag_info(c->diag, DIAG_WRONG_KIND, NULL, checker_kind(c, var));
    }
}

// "ask ... and call the answer X" and "read file ... and call it X" make X as
// text, or store text in it.
void checker_make_or_store(Checker *c, const Name *name, TypeVar value) {
    const Symbol *existing = checker_find_made(c, name->text);
    if (existing) {
        checker_use(c, existing, true);
        store(c, existing, value, NULL, name->pos.span);
    } else {
        checker_make(c, name, value);
    }
}

static void make_text(Checker *c, const Name *name) {
    checker_make_or_store(c, name, types_new(c, TYPE_TEXT, name->pos.line));
}

static void check_let(Checker *c, const Stmt *stmt) {
    const Name *name = &stmt->as.assign.name;
    TypeVar value = checker_check_expr(c, stmt->as.assign.value);
    const Symbol *existing = checker_find_made(c, name->text);
    if (existing) {
        diag_error(c->diag, name->pos.span, "You already made \"%s\" on line %zu.", existing->display, existing->line);
        diag_note(c->diag, "To change it, write \"set %s to ...\".", existing->display);
        return;
    }
    checker_make(c, name, value);
    TypedSlot slot = {(Type *)&stmt->as.assign.type, value};
    vec_push(c->arena, &c->slots, slot);
    narrow_new(c, name->text, stmt->as.assign.value);
}

// A function gives back one kind of value, or nothing, everywhere.
static void check_return(Checker *c, const Stmt *stmt) {
    TypeVar value = stmt->as.returned ? checker_check_value(c, stmt->as.returned) : 0;
    const Function *f = c->function;
    if (!f && stmt->stop_here) {
        diag_error(c->diag, stmt->pos.span, "\"stop here\" only works inside a definition.");
        diag_note(c->diag, "It ends the definition and goes back to whoever used it. "
                           "To end the whole program, write \"stop the program\".");
        diag_info(c->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        diag_choice(c->diag, "stop the program");
        return;
    }
    if (!f) {
        diag_error(c->diag, stmt->pos.span, "\"give back\" only works inside a definition.");
        diag_note(c->diag, "It ends a definition and hands a value back to whoever used it. "
                           "To end the whole program, write \"stop the program\".");
        return;
    }
    Type result = types_of(c, f->result);
    bool nothing = stmt->as.returned && stmt->as.returned->kind == EXPR_NOTHING;
    if (nothing && result == TYPE_NOTHING) return;  // "give back nothing" is "return"
    if (!stmt->as.returned) {
        if (result == TYPE_NOTHING || result == TYPE_ERROR) return;
        diag_error(c->diag, stmt->pos.span, "%s gives back a value, so it can't %s here.",
                   checker_capital(c, checker_function_label(c, f)),
                   stmt->stop_here ? "stop without one" : "give back nothing");
        diag_note(c->diag, "Give back a value here too, like \"give back 0\".");
    } else if (result == TYPE_NOTHING) {
        diag_error(c->diag, stmt->as.returned->pos.span, "%s gives back nothing, so it can't give back a value.",
                   checker_capital(c, checker_function_label(c, f)));
        diag_note(c->diag, "It's defined on line %zu: \"%s\".", f->definition->pos.line, checker_function_header(c, f));
    } else if (!checker_fits(c, f->result, value, stmt->as.returned, stmt->pos.line)) {
        diag_error(c->diag, stmt->as.returned->pos.span, "%s gives back %s (since line %zu), but this is %s.",
                   checker_capital(c, checker_function_label(c, f)), checker_type_name(c, result),
                   types_line(c, f->result), checker_kind(c, value));
        if (types_of(c, value) == TYPE_NOTHING && type_can_be_nothing(result)) {
            diag_note(c->diag, "To give back nothing too, say so in the definition, like \"giving back %s or "
                               "nothing\".", checker_type_name(c, result));
        } else {
            diag_note(c->diag, "A definition gives back the same kind of value every time.");
        }
    }
}

static void check_stmt(Checker *c, const Stmt *stmt);

// Ends a scope opened when made.len was `outer`: its names are forgotten,
// and remembered as "ended" in `where` on `line`.
void checker_end_scope(Checker *c, size_t outer, size_t line, const char *where) {
    for (size_t i = outer; where && i < c->made.len; i++) {
        Symbol ended = {c->made.items[i].name, c->made.items[i].display, line, where, c->made.items[i].loop_number, 0,
                        c->made.items[i].loop_item, NULL, 0};
        vec_push(c->arena, &c->ended, ended);
    }
    c->made.len = outer;
}

// Whether a block always leaves before its end (give back, stop, skip).
static bool always_leaves(const Block *block) {
    if (block->len == 0) return false;
    StmtKind last = block->items[block->len - 1]->kind;
    return last == STMT_RETURN || last == STMT_STOP || last == STMT_BREAK || last == STMT_CONTINUE;
}

// After "if node is nothing, give back 0." the rest of the block knows
// node isn't nothing.
void checker_check_statements(Checker *c, const Block *block) {
    for (size_t i = 0; i < block->len; i++) {
        const Stmt *stmt = block->items[i];
        check_stmt(c, stmt);
        if (stmt->kind == STMT_IF && stmt->as.if_stmt.branches.len == 1 &&
            always_leaves(&stmt->as.if_stmt.branches.items[0].body)) {
            checker_narrow_condition(c, stmt->as.if_stmt.branches.items[0].condition,
                                     stmt->as.if_stmt.branches.items[0].unless);
        }
    }
}

void checker_check_block(Checker *c, const Block *block, size_t line, const char *where) {
    size_t outer = c->made.len, checked = c->present.len;
    c->blocks++;
    checker_check_statements(c, block);
    c->blocks--;
    checker_end_scope(c, outer, line, where);
    c->present.len = checked;
}

static void check_params(Checker *c, const Function *f) {
    const Stmt *stmt = f->definition;
    for (size_t i = 0; i < stmt->as.function.params.len; i++) {
        const Name *param = &stmt->as.function.params.items[i];
        if (checker_find_made(c, param->text)) {
            diag_error(c->diag, param->pos.span, "This definition already has an input called \"%s\".", param->display);
            diag_note(c->diag, "Give each input its own name.");
        } else {
            checker_make(c, param, f->params.items[i]);
        }
    }
}

// A function's body sees only its inputs and the names it makes; loops and
// blocks outside it don't count.
static void check_function(Checker *c, const Stmt *stmt) {
    if (c->function || c->blocks > 0) {
        diag_error(c->diag, stmt->as.function.name.pos.span,
                   "Definitions with \"to\" can only be at the top level, not inside %s.",
                   c->function ? "another definition" : "an \"if\" or a loop");
        diag_note(c->diag, "Move \"to %s ...:\" to the start of a line, outside every block.",
                  stmt->as.function.name.display);
        if (c->function) {
            diag_note(c->diag, "To make one inside another, write \"let %s be a function taking ...:\" "
                               "with its sentences indented below.", stmt->as.function.name.display);
        }
        return;
    }
    const Function *f = NULL;
    for (size_t i = 0; i < c->functions.len && !f; i++) {
        if (c->functions.items[i].definition == stmt) f = &c->functions.items[i];
    }
    if (!f) return;  // a duplicate, already reported
    size_t saved_floor = c->floor;
    size_t saved_loops = c->loops.len;
    c->floor = c->made.len;
    c->loops.len = 0;
    c->function = f;
    check_params(c, f);
    if (stmt->as.function.defines) {  // "a customer is loyal if its orders ...": "it" is the customer
        Loop subject = {true, f->params.items[0], false, NO_VARIANT, false};
        vec_push(c->arena, &c->loops, subject);
    }
    checker_check_statements(c, &stmt->as.function.body);
    checker_end_scope(c, c->floor, stmt->pos.line, NULL);
    c->function = NULL;
    c->floor = saved_floor;
    c->loops.len = saved_loops;
}

// Each branch knows what its condition says isn't nothing, and what the
// conditions before it being false say.
static void check_if(Checker *c, const Stmt *stmt) {
    size_t outer = c->present.len;
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        if (branch->condition && branch->unless) {
            checker_require(c, checker_check_expr(c, branch->condition), TYPE_YESNO, branch->condition,
                            "An \"unless\" needs yes or no to decide",
                            "Compare it with something, like \"unless x is 5\".");
        } else if (branch->condition) {
            checker_require(c, checker_check_expr(c, branch->condition), TYPE_YESNO, branch->condition,
                            "An \"if\" needs yes or no to decide", "Compare it with something, like \"if x is 5\".");
        }
        size_t before = c->present.len;
        if (branch->condition) checker_narrow_condition(c, branch->condition, !branch->unless);
        checker_check_block(c, &branch->body, stmt->pos.line, branch->unless ? "the \"unless\"" : "the \"if\"");
        c->present.len = before;
        if (branch->condition) checker_narrow_condition(c, branch->condition, branch->unless);
    }
    c->present.len = outer;
}

void checker_make_item_name(Checker *c, const Name *name, TypeVar item) {
    const Symbol *existing = checker_find_made(c, name->text);
    if (existing) {
        diag_error(c->diag, name->pos.span, "You already made \"%s\" on line %zu.", existing->display, existing->line);
        diag_note(c->diag, "Give the items a name that isn't taken yet, like \"current %s\".", name->display);
        return;
    }
    if (checker_clashes(c, name)) return;
    Symbol symbol = {name->text, name->display, name->pos.line, NULL, true, item, true,
                     c->function ? c->function->definition : NULL, 0};
    vec_push(c->arena, &c->made, symbol);
}

static void check_loop_control(Checker *c, const Stmt *stmt) {
    for (size_t i = 0; i < c->loops.len; i++) {
        if (!c->loops.items[i].subject) return;  // a "when" of "check which kind" isn't a loop
    }
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
    case STMT_USE: checker_check_use(c, stmt); break;
    case STMT_LIBRARY: checker_check_library_stmt(c, stmt); break;
    case STMT_RULE: checker_check_rule(c, (Stmt *)stmt); break;
    case STMT_EVENT: checker_check_event(c, stmt); break;
    case STMT_TRY: checker_check_try(c, stmt); break;
    case STMT_EXAMPLE:
        checker_require(c, checker_check_expr(c, stmt->as.value), TYPE_YESNO, stmt->as.value,
                        "An example says something that's yes or no, like \"total is 12\"", NULL);
        break;
    case STMT_ASK:
        checker_check_expr(c, stmt->as.ask.prompt);
        if (stmt->as.ask.kind == TYPE_UNKNOWN) {  // a plain "ask": asked for as the kind its answer is used as
            Stmt *ask = (Stmt *)stmt;
            TypeVar answer = types_new(c, TYPE_UNKNOWN, stmt->pos.line);
            checker_make_or_store(c, &ask->as.ask.answer, answer);
            vec_push(c->arena, &c->slots, ((TypedSlot){&ask->as.ask.kind, answer}));
            vec_push(c->arena, &c->asks, ((AskAnswer){ask, answer}));
        } else {
            checker_make_or_store(c, &stmt->as.ask.answer, types_new(c, stmt->as.ask.kind, stmt->pos.line));
        }
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
        if (stmt->as.read_file.path->kind == EXPR_TEXT && stmt->as.read_file.path->as.text.len > 4 &&
            strcmp(stmt->as.read_file.path->as.text.value + stmt->as.read_file.path->as.text.len - 4, ".csv") == 0) {
            vec_push(c->arena, &c->csv_texts, stmt);  // going through its rows is "read the table in"
        }
        break;
    case STMT_STOP: checker_check_stop(c, stmt); break;
    case STMT_IF: check_if(c, stmt); break;
    case STMT_LOOP: checker_check_loop(c, stmt); break;
    case STMT_BREAK:
    case STMT_CONTINUE: check_loop_control(c, stmt); break;
    case STMT_FUNCTION: check_function(c, stmt); break;
    case STMT_RETURN: check_return(c, stmt); break;
    case STMT_CALL: checker_check_call(c, stmt->as.call, false); break;
    case STMT_CONSTANT: checker_check_constant(c, stmt); break;
    case STMT_LIST: checker_check_list_stmt(c, stmt); break;
    case STMT_RECORD: checker_check_record(c, (Stmt *)stmt); break;
    case STMT_SUM: checker_check_sum(c, stmt); break;
    case STMT_MATCH: checker_check_match(c, stmt); break;
    case STMT_INTERFACE: checker_check_interface(c, stmt); break;
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
        for (size_t b = 0; stmt->kind == STMT_MATCH && b < stmt->as.match.branches.len; b++) {
            collect_names(c, &stmt->as.match.branches.items[b].body);
        }
        if (stmt->kind == STMT_FUNCTION) continue;  // its names are its own
        const Name *name = stmt->kind == STMT_LET    ? &stmt->as.assign.name
                           : stmt->kind == STMT_ASK  ? &stmt->as.ask.answer
                           : stmt->kind == STMT_READ_FILE ? &stmt->as.read_file.name
                           : stmt->kind == STMT_LIBRARY && stmt->as.library.name.text ? &stmt->as.library.name
                                                          : NULL;
        if (name && !checker_find_later(c, name->text)) {
            Symbol symbol = {name->text, name->display, name->pos.line, NULL, false, 0, false, NULL, 0};
            vec_push(c->arena, &c->later, symbol);
        }
    }
}

// Whether a function's body gives back a value anywhere ("give back E").
bool checker_gives_value(const Block *block) {
    for (size_t i = 0; i < block->len; i++) {
        const Stmt *stmt = block->items[i];
        if (stmt->kind == STMT_RETURN && stmt->as.returned && stmt->as.returned->kind != EXPR_NOTHING) return true;
        if (stmt->kind == STMT_LOOP && checker_gives_value(&stmt->as.loop.body)) return true;
        for (size_t b = 0; stmt->kind == STMT_IF && b < stmt->as.if_stmt.branches.len; b++) {
            if (checker_gives_value(&stmt->as.if_stmt.branches.items[b].body)) return true;
        }
        for (size_t b = 0; stmt->kind == STMT_MATCH && b < stmt->as.match.branches.len; b++) {
            if (checker_gives_value(&stmt->as.match.branches.items[b].body)) return true;
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
    if (result == TYPE_UNKNOWN && !checker_gives_value(&def->as.function.body)) result = TYPE_NOTHING;
    f->result = types_new(c, result, result ? line : 0);
}

// The kind of record a method is for (0 for a function that isn't one).
static Type method_kind(const Stmt *def) {
    return def->as.function.method ? def->as.function.declared.items[0] : 0;
}

// A second function with a name already taken. Methods for different kinds
// of record can share a name ("speak" for a dog and for a cat); the call
// picks one by the kind of its record.
static bool reject_duplicate(Checker *c, Stmt *stmt) {
    const Name *name = &stmt->as.function.name;
    Type kind = method_kind(stmt);
    c->file = stmt->pos.file;
    for (size_t i = 0; i < c->functions.len; i++) {
        const Function *existing = &c->functions.items[i];
        if (strcmp(existing->name, name->text) != 0) continue;
        Type other = method_kind(existing->definition);
        if (kind && other && kind != other) continue;
        if (existing->file != c->file && !(kind && kind == other)) continue;  // each file has its own names
        if (kind && kind == other) {
            diag_error(c->diag, name->pos.span, "You already defined \"%s\" for %s on %s.", existing->display,
                       ast_type_name(c->arena, c->records, kind), checker_where(c, existing->definition->pos));
            diag_note(c->diag, "Each kind of record can have one method called \"%s\".", existing->display);
        } else {
            diag_error(c->diag, name->pos.span, "You already defined \"%s\" on line %zu.", existing->display,
                       existing->definition->pos.line);
            diag_note(c->diag, "Each definition needs its own name. Only methods for different kinds of record "
                               "can share one.");
        }
        return true;
    }
    return false;
}

// Methods that share a name get told apart in the C program by their kind.
static void mark_shared_methods(Checker *c) {
    for (size_t i = 0; i < c->functions.len; i++) {
        Stmt *def = c->functions.items[i].definition;
        if (checker_shared_method(c, def->as.function.name.text)) {
            def->as.function.overload = 1 + (method_kind(def) - TYPE_RECORD);
        }
    }
}

// Every top-level function, so calls can come before definitions.
static void collect_functions(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        Stmt *stmt = program->items[i];
        if (stmt->kind != STMT_FUNCTION) continue;
        const Name *name = &stmt->as.function.name;
        if (reject_duplicate(c, stmt)) continue;
        Function f = {name->text, name->display, stmt->pos.file, stmt->as.function.private, stmt, {0}, 0};
        function_types(c, &f);
        vec_push(c->arena, &c->functions, f);
    }
    mark_shared_methods(c);
}

// The kind each plain "ask" asks for: what its answer is used as (a number,
// an amount, yes or no), or text when nothing decides. Other kinds can't be
// typed in.
static void settle_answers(Checker *c) {
    for (size_t i = 0; i < c->asks.len; i++) {
        Stmt *ask = c->asks.items[i].ask;
        TypeVar var = c->asks.items[i].var;
        Type kind = types_of(c, var);
        if (kind == TYPE_UNKNOWN) {
            types_require(c, var, TYPE_TEXT, ask->pos.line);
        } else if (kind != TYPE_TEXT && kind != TYPE_NUMBER && kind != TYPE_YESNO && kind != TYPE_ERROR &&
                   !type_is_money(kind) && !type_is_textkind(kind)) {
            diag_error(c->diag, ask->as.ask.answer.pos.span,
                       "An answer to a question is text, a number, an amount or yes or no, but \"%s\" is %s.",
                       ask->as.ask.answer.display, checker_kind(c, var));
            diag_note(c->diag, "Ask for each part on its own, then make %s from the answers.", checker_kind(c, var));
            diag_info(c->diag, DIAG_WRONG_KIND, NULL, "text, a number, an amount or yes or no");
        }
    }
}

void check_program(Arena *arena, Diag *diag, Program *whole) {
    const Block *program = &whole->body;
    Checker c = {0};
    c.arena = arena;
    c.diag = diag;
    c.records = &whole->records;
    c.program = whole;
    c.main_file = whole->files.len ? (unsigned)whole->files.len - 1 : 0;
    size_t first_error = diag_count(diag);
    checker_collect_records(&c, program);
    checker_collect_sums(&c, program);
    checker_collect_interfaces(&c, program);
    collect_functions(&c, program);
    checker_collect_constants(&c, program);
    collect_names(&c, program);
    for (size_t pass = 0; pass < 5; pass++) {  // constants, kinds of record, functions, the rest, then events
        for (size_t i = 0; i < program->len; i++) {
            const Stmt *stmt = program->items[i];
            int kind_pass = stmt->kind == STMT_CONSTANT ? 0 : stmt->kind == STMT_RECORD || stmt->kind == STMT_SUM || stmt->kind == STMT_INTERFACE ? 1
                            : stmt->kind == STMT_FUNCTION ? 2 : stmt->kind == STMT_EVENT ? 4 : 3;
            c.file = stmt->pos.file;
            if (kind_pass == (int)pass && checker_check_module_stmt(&c, stmt)) check_stmt(&c, stmt);
        }
    }
    types_settle(&c);
    checker_judge_columns(&c);  // "the total amount of sales": amounts, if the file says so
    types_default_numbers(&c);  // otherwise a number
    for (size_t r = 0; r < c.records->len; r++) {  // a table's columns that nothing decided are text, as in the file
        if (!c.records->items[r].table) continue;
        Stmt *def = c.records->items[r].definition;
        for (size_t f = 0; f < def->as.record.fields.len; f++) {
            TypeVar column = checker_field_kind(&c, &def->as.record.fields.items[f], def->pos.line);
            if (types_of(&c, column) == TYPE_UNKNOWN) types_require(&c, column, TYPE_TEXT, def->pos.line);
        }
    }
    settle_answers(&c);  // a plain "ask" whose answer nothing decided is text
    checker_check_columns(&c);
    checker_finish_charts(&c);
    types_finish(&c, first_error);
    diag_sort(diag, first_error);
    if (diag_count(diag) == first_error) checker_rename_modules(&c);
}
