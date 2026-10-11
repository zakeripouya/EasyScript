// C code generation. The checker has decided the kind of every value, so the
// program uses plain C types: a number is a double, yes/no a bool, text an
// EsText * and a list an EsList * (runtime/es_memory.h). There is no tagged
// value. Runtime calls that can fail get a site (cg_site: where they are),
// so their errors can show the line and carets. Lists are in codegen_list.c;
// constants, functions and main() in codegen_program.c.
//
// Every function pushes a frame onto the runtime's call stack (EsFrame: the
// function, the values it was given, where it was called), so a runtime error
// can say which calls led to it.
//
// Memory (docs/memory.md), for text and lists: every text or list expression
// produces an owned value, and runtime calls and EasyScript functions consume
// their text and list arguments, so nested calls need no bookkeeping. Reading
// a variable retains it; storing goes through es_set/es_list_set, which
// releases the old value after the new one is worked out. A block releases
// the variables first made in it when it ends; stop the loop, skip this one
// and give back release the ones of every block they leave, and main()'s are
// released when the program ends. Numbers and yes/no values own nothing and
// are never retained or released.
//
// Variables are C locals of main() or of their function, so their addresses
// never escape and the C compiler can keep numbers in registers.

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "back/codegen_internal.h"

// Starts a line of the body at the current nesting depth.
void cg_indent(Codegen *g) {
    sb_append_repeat(&g->body, ' ', 4 * (g->depth + 1));
}

// --- Releasing text and list variables ------------------------------------------

static void emit_drop(Codegen *g, const char *name, Type type) {
    cg_indent(g);
    if (cg_is_cell(g, name)) {  // the cell, which closures made here may share
        sb_append(&g->body, "es_record_drop(&");
        cg_append_cell(&g->body, name, type);
    } else {
        sb_appendf(&g->body, "%s(&", cg_drop(type));
        cg_append_var(g, &g->body, name, type);
    }
    sb_append(&g->body, ");\n");
}

// Releases the variables of scopes[from] and every scope inside it.
static void emit_drop_scopes(Codegen *g, size_t from) {
    for (size_t i = g->scopes.len; i > from; i--) {
        const NameScope *scope = &g->scopes.items[i - 1];
        for (size_t j = 0; j < scope->names.len; j++) emit_drop(g, scope->names.items[j].name, scope->names.items[j].type);
    }
}

void cg_push_scope(Codegen *g) {
    NameScope scope = {0};
    vec_push(g->arena, &g->scopes, scope);
}

// Releases the innermost scope's variables, then leaves it.
void cg_pop_scope(Codegen *g) {
    emit_drop_scopes(g, g->scopes.len - 1);
    g->scopes.len--;
}

// Leaving a function (its text and list inputs, every such variable it
// makes, and the text its frame holds) or the program (every text and list
// variable of main()).
void cg_emit_drop_all(Codegen *g) {
    if (g->function) {
        for (size_t i = 0; i < g->function->as.function.params.len; i++) {
            Type type = g->function->as.function.param_types.items[i];
            const char *param = g->function->as.function.params.items[i].text;
            if (cg_is_cell(g, param)) {  // its cell, and then the value it was given
                cg_indent(g);
                sb_append(&g->body, "es_record_drop(&");
                cg_append_cell(&g->body, param, type);
                sb_append(&g->body, ");\n");
            }
            if (type_counted(type)) {
                cg_indent(g);
                sb_appendf(&g->body, "%s(&", cg_drop(type));
                cg_append_raw_var(&g->body, param, type);
                sb_append(&g->body, ");\n");
            }
            if (type == TYPE_TEXT || type_is_object(type)) {  // the frame's reference to the value it was given
                cg_indent(g);
                sb_appendf(&g->body, "%s(es_input(%zu).%c);\n", cg_release(type), i, type == TYPE_TEXT ? 't' : 'r');
            }
        }
    }
    for (size_t i = 0; i < g->vars.len; i++) {
        const VarCode *var = &g->vars.items[i];
        if (type_counted(var->type) || cg_is_cell(g, var->name)) emit_drop(g, var->name, var->type);
    }
}

// --- Statements ----------------------------------------------------------------

// A variable gets a value: "es_vn_x = VALUE;", or "es_set(&es_vt_x, VALUE);"
// for text (es_list_set for lists), which releases what it held.
void cg_emit_store(Codegen *g, const char *name, Type type, const char *value) {
    cg_indent(g);
    cg_append_assign(g, &g->body, name, type, value, false);
}

static const char *expr_code(Codegen *g, const Expr *expr) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    cg_emit_expr(g, expr, &sb);
    return sb.data;
}

static const char *text_code(Codegen *g, const Expr *expr) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    cg_emit_text(g, expr, &sb);
    return sb.data;
}

// add/subtract/increase/decrease/multiply/divide: x = x + amount, or es_div.
// "add E to L" where L is a list adds E to its end.
static void emit_change(Codegen *g, const Stmt *stmt) {
    if (stmt->as.change.field) {
        const char *code = cg_field_stmt(g, stmt);
        cg_indent(g);
        sb_appendf(&g->body, "%s\n", code);
        return;
    }
    const char *name = stmt->as.change.target.text;
    if (stmt->as.change.append) {
        Type list = type_list_of(stmt->as.change.amount->type);
        const char *amount = expr_code(g, stmt->as.change.amount);
        cg_indent(g);
        sb_appendf(&g->body, "es_add_%s(es_list_retain(", cg_item_suffix(list));
        cg_append_var(g, &g->body, name, list);
        sb_appendf(&g->body, "), %s);\n", amount);
        return;
    }
    StrBuf var;
    sb_init(&var, g->arena);
    cg_append_var(g, &var, name, TYPE_NUMBER);
    const char *amount = expr_code(g, stmt->as.change.amount);
    const char *value;
    switch (stmt->as.change.op) {
    case CHANGE_ADD: value = arena_sprintf(g->arena, "%s + %s", var.data, amount); break;
    case CHANGE_SUBTRACT: value = arena_sprintf(g->arena, "%s - %s", var.data, amount); break;
    case CHANGE_MULTIPLY: value = arena_sprintf(g->arena, "%s * %s", var.data, amount); break;
    default: value = arena_sprintf(g->arena, "es_div(%zu, %s, %s)", cg_site(g, stmt->pos), var.data, amount); break;
    }
    cg_emit_store(g, name, TYPE_NUMBER, value);
}

// write/append: the text first, then the file name (left to right).
static void emit_file_write(Codegen *g, const Stmt *stmt) {
    const Expr *value = stmt->as.file_write.text;
    const char *text = text_code(g, value);
    bool value_effects = cg_has_effects(g, value) || type_is_list(value->type);  // turning a list into text reads it
    if (value_effects && cg_has_effects(g, stmt->as.file_write.path)) {
        size_t t = cg_new_temp(g, TYPE_TEXT);
        cg_indent(g);
        sb_appendf(&g->body, "es_t%zu = %s;\n", t, text);
        text = arena_sprintf(g->arena, "es_t%zu", t);
    }
    cg_indent(g);
    sb_appendf(&g->body, "es_write_file(%zu, %s, %s, %d);\n", cg_site(g, stmt->as.file_write.path->pos), text,
               expr_code(g, stmt->as.file_write.path), stmt->kind == STMT_APPEND_FILE);
}


void cg_emit_block(Codegen *g, const Block *block) {
    g->depth++;
    cg_push_scope(g);
    for (size_t i = 0; i < block->len; i++) {
        cg_emit_stmt(g, block->items[i]);
    }
    cg_pop_scope(g);
    g->depth--;
}

// if (C1) { ... } else if (C2) { ... } else { ... }
static void emit_if(Codegen *g, const Stmt *stmt) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        if (i == 0) {
            cg_indent(g);
        } else {
            sb_append(&g->body, " else ");
        }
        if (branch->condition) sb_appendf(&g->body, "if (%s) ", expr_code(g, branch->condition));
        sb_append(&g->body, "{\n");
        cg_emit_block(g, &branch->body);
        cg_indent(g);
        sb_append_char(&g->body, '}');
    }
    sb_append_char(&g->body, '\n');
}

// es_tN = NUMBER; (so a count's values are worked out once, left to right)
static size_t emit_temp(Codegen *g, const Expr *expr) {
    size_t t = cg_new_temp(g, TYPE_NUMBER);
    cg_indent(g);
    sb_appendf(&g->body, "es_t%zu = %s;\n", t, expr_code(g, expr));
    return t;
}

// The line that opens the C loop, for each kind of loop.
static void emit_loop_head(Codegen *g, const Stmt *stmt, size_t id) {
    switch (stmt->as.loop.kind) {
    case LOOP_COUNT: {
        size_t from = emit_temp(g, stmt->as.loop.from);
        size_t to = emit_temp(g, stmt->as.loop.to);
        size_t step = stmt->as.loop.step ? emit_temp(g, stmt->as.loop.step) : 0;
        cg_indent(g);
        size_t site = cg_site(g, stmt->as.loop.step ? stmt->as.loop.step->pos : stmt->pos);  // only the step can fail
        sb_appendf(&g->body, "EsCount es_c%zu = es_count_start(%zu, es_t%zu, es_t%zu, ", id, site, from, to);
        if (step) {
            sb_appendf(&g->body, "es_t%zu, 1, %d);\n", step, stmt->as.loop.down);
        } else {
            sb_appendf(&g->body, "1.0, 0, %d);\n", stmt->as.loop.down);
        }
        cg_indent(g);
        sb_appendf(&g->body, "for (long long es_i%zu = 0; es_i%zu <= es_c%zu.last; es_i%zu++) {\n", id, id, id, id);
        cg_indent(g);
        sb_append(&g->body, "    ");
        cg_append_assign(g, &g->body, stmt->as.loop.var.text, TYPE_NUMBER,
                         arena_sprintf(g->arena, "es_count_value(&es_c%zu, es_i%zu)", id, id), true);
        break;
    }
    case LOOP_TIMES:
        cg_indent(g);
        sb_appendf(&g->body, "long long es_n%zu = es_times(%zu, %s);\n", id, cg_site(g, stmt->as.loop.times->pos),
                   expr_code(g, stmt->as.loop.times));
        cg_indent(g);
        sb_appendf(&g->body, "for (long long es_i%zu = 1; es_i%zu <= es_n%zu; es_i%zu++) {\n", id, id, id, id);
        break;
    case LOOP_WHILE:
    case LOOP_UNTIL:
        cg_indent(g);
        sb_appendf(&g->body, "while (%s%s) {\n", stmt->as.loop.kind == LOOP_UNTIL ? "!" : "",
                   expr_code(g, stmt->as.loop.condition));
        break;
    case LOOP_FOREVER:
        cg_indent(g);
        sb_append(&g->body, "for (;;) {\n");
        break;
    case LOOP_EACH:
        if (stmt->as.loop.pair) {
            cg_emit_each_map_head(g, stmt, id, &g->body, g->depth);
        } else {
            cg_emit_each_head(g, stmt, id, &g->body, g->depth);
        }
        break;
    }
}

// { <setup> for/while (...) { body } }
static void emit_loop(Codegen *g, const Stmt *stmt) {
    size_t id = ++g->loop_ids;
    cg_indent(g);
    sb_append(&g->body, "{\n");
    g->depth++;
    cg_push_scope(g);
    if (stmt->as.loop.kind == LOOP_COUNT) cg_declare_name(g, stmt->as.loop.var.text, TYPE_NUMBER);
    LoopCode loop = {stmt, id, 0, NULL, TYPE_UNKNOWN};
    if (stmt->as.loop.kind == LOOP_EACH && stmt->as.loop.pair) {  // a map: "it" is the key
        Type map = stmt->as.loop.list->type;
        loop.item = stmt->as.loop.var.text;
        loop.item_type = ast_map_key(g->records, map);
        cg_declare_name(g, loop.item, loop.item_type);
        cg_declare_name(g, stmt->as.loop.value_name.text, ast_map_value(g->records, map));
    } else if (stmt->as.loop.kind == LOOP_EACH) {
        loop.item = stmt->as.loop.var.text;
        loop.item_type = type_item(stmt->as.loop.list->type);
        cg_declare_name(g, loop.item, loop.item_type);
    }
    emit_loop_head(g, stmt, id);
    loop.scope = g->scopes.len;
    vec_push(g->arena, &g->loops, loop);
    cg_emit_block(g, &stmt->as.loop.body);
    g->loops.len--;
    cg_indent(g);
    sb_append(&g->body, "}\n");
    cg_pop_scope(g);
    g->depth--;
    cg_indent(g);
    sb_append(&g->body, "}\n");
}

const LoopCode *cg_innermost_loop(const Codegen *g) {
    for (size_t i = g->loops.len; i > 0; i--) {
        if (g->loops.items[i - 1].stmt && g->loops.items[i - 1].stmt->kind == STMT_LOOP) return &g->loops.items[i - 1];
    }
    assert(!"\"stop the loop\" outside a loop reached codegen");
    return NULL;
}

// give back E / return: the value is worked out before the function's text
// is released.
// Popping the function's frame: "es_leave();", or "es_leave_many(N);" for a
// function with several inputs (their values come off the runtime's array too).
const char *cg_leave_code(const Codegen *g) {
    size_t n = g->function->as.function.params.len;
    return n > 1 ? arena_sprintf(g->arena, "es_leave_many(%zu);\n", n) : "es_leave();\n";
}

static void emit_return(Codegen *g, const Stmt *stmt) {
    size_t t = 0;
    Type result = g->function->as.function.result;
    if (stmt->as.returned && result != TYPE_NOTHING) {
        t = cg_new_temp(g, result);
        cg_indent(g);
        sb_appendf(&g->body, "es_t%zu = %s;\n", t, expr_code(g, stmt->as.returned));
    }
    cg_emit_drop_all(g);
    cg_indent(g);
    if (t) {
        sb_append(&g->body, cg_leave_code(g));
        cg_indent(g);
        sb_appendf(&g->body, "return es_t%zu;\n", t);
    } else {
        sb_append(&g->body, cg_leave_code(g));
        cg_indent(g);
        sb_append(&g->body, "return;\n");
    }
}

// A call written as a sentence: text it gives back is released, other values ignored.
static void emit_call_statement(Codegen *g, const Stmt *stmt) {
    const Expr *call_expr = stmt->as.call;
    Type result = call_expr->as.call.callee ? ast_func_kind(g->records, call_expr->as.call.callee->type)->result
                                            : call_expr->as.call.target->as.function.result;
    const char *call = expr_code(g, stmt->as.call);
    cg_indent(g);
    if (type_counted(result)) {
        sb_appendf(&g->body, "%s(%s);\n", cg_release(result), call);
    } else if (result == TYPE_NOTHING) {
        sb_appendf(&g->body, "%s;\n", call);
    } else {
        sb_appendf(&g->body, "(void)%s;\n", call);
    }
}

static void emit_stop(Codegen *g) {
    cg_indent(g);
    if (!g->function) {  // the end of main() releases everything
        sb_append(&g->body, "goto es_end;\n");
        g->stops = true;
        return;
    }
    sb_append(&g->body, "{\n");
    g->depth++;
    cg_emit_drop_all(g);
    cg_indent(g);
    sb_append(&g->body, "es_stop();\n");
    g->depth--;
    cg_indent(g);
    sb_append(&g->body, "}\n");
}

void cg_emit_stmt(Codegen *g, const Stmt *stmt) {
    cg_indent(g);
    sb_appendf(&g->body, "/* line %zu */\n", stmt->pos.line);
    switch (stmt->kind) {
    case STMT_LET:
    case STMT_SET: {
        const Expr *value = stmt->as.assign.value;
        if (stmt->kind == STMT_SET && stmt->as.assign.field) {
            const char *code = cg_field_stmt(g, stmt);
            cg_indent(g);
            sb_appendf(&g->body, "%s\n", code);
            break;
        }
        if (stmt->kind == STMT_SET && cg_emit_append(g, stmt)) break;
        if (stmt->kind == STMT_LET) cg_declare_name(g, stmt->as.assign.name.text, stmt->as.assign.type);
        cg_indent(g);
        cg_append_assign(g, &g->body, stmt->as.assign.name.text, stmt->as.assign.type, expr_code(g, value),
                         stmt->kind == STMT_LET);
        break;
    }
    case STMT_CHANGE: emit_change(g, stmt); break;
    case STMT_SAY:
        cg_indent(g);
        sb_appendf(&g->body, "es_say(%s);\n", text_code(g, stmt->as.value));
        break;
    case STMT_ASK:
        cg_declare_name(g, stmt->as.ask.answer.text, TYPE_TEXT);
        cg_emit_new_cell(g, stmt->as.ask.answer.text, TYPE_TEXT);
        cg_emit_store(g, stmt->as.ask.answer.text, TYPE_TEXT,
                   arena_sprintf(g->arena, "es_ask(%s)", text_code(g, stmt->as.ask.prompt)));
        break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: emit_file_write(g, stmt); break;
    case STMT_READ_FILE:
        cg_declare_name(g, stmt->as.read_file.name.text, TYPE_TEXT);
        cg_emit_new_cell(g, stmt->as.read_file.name.text, TYPE_TEXT);
        cg_emit_store(g, stmt->as.read_file.name.text, TYPE_TEXT,
                   arena_sprintf(g->arena, "es_read_file(%zu, %s)", cg_site(g, stmt->as.read_file.path->pos),
                                 expr_code(g, stmt->as.read_file.path)));
        break;
    case STMT_STOP: emit_stop(g); break;
    case STMT_IF: emit_if(g, stmt); break;
    case STMT_LOOP: emit_loop(g, stmt); break;
    case STMT_FUNCTION: break;  // generated separately, before main()
    case STMT_CONSTANT: break;  // a static initializer, worked out by the compiler
    case STMT_RECORD: break;    // a C struct, before the functions
    case STMT_SUM: break;       // a C struct per variant, likewise
    case STMT_MATCH: cg_emit_match(g, stmt); break;
    case STMT_RETURN: emit_return(g, stmt); break;
    case STMT_CALL: emit_call_statement(g, stmt); break;
    case STMT_LIST: {
        const char *code = cg_list_stmt(g, stmt);
        cg_indent(g);
        sb_appendf(&g->body, "%s;\n", code);
        break;
    }
    case STMT_BREAK:
    case STMT_CONTINUE:
        emit_drop_scopes(g, cg_innermost_loop(g)->scope);
        cg_indent(g);
        sb_append(&g->body, stmt->kind == STMT_BREAK ? "break;\n" : "continue;\n");
        break;
    }
}
