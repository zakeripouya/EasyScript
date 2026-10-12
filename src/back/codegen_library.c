// The standard library: each value or sentence is a call into the runtime
// (es_math.h, es_time.h, es_files.h, es_csv.h), its parts worked out left to right.

#include <assert.h>
#include <string.h>
#include "back/codegen_internal.h"

// The parts a and b (either can be NULL), in order; returns whether a "(" was opened.
static bool parts(Codegen *g, const Expr *expr, const char **a, const char **b, StrBuf *out) {
    Expr *exprs[] = {expr->as.library.a, expr->as.library.b};
    const char *values[2] = {NULL, NULL};
    bool opened = cg_ordered(g, exprs, 2, values, out);
    *a = values[0];
    *b = values[1];
    return opened;
}

// "pick one of L": item N of L for a random N, L held in a temporary so it's worked out once.
static void emit_pick(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *list = expr->as.library.a;
    size_t t = cg_new_temp(g, list->type);
    size_t site = cg_site(g, expr->pos);
    sb_appendf(out, "(es_t%zu = %s, es_get_%s(%zu, es_t%zu, ES_AT, es_random_position(%zu, es_t%zu)))", t,
               cg_operand(g, list, false), cg_item_suffix(list->type), site, t, site, t);
}

void cg_emit_library(Codegen *g, const Expr *expr, StrBuf *out) {
    LibraryOp op = expr->as.library.op;
    if (op >= LIB_PROGRAM_WORDS) {
        cg_emit_system(g, expr, out);
        return;
    }
    if (op == LIB_PICK) {
        emit_pick(g, expr, out);
        return;
    }
    const char *a, *b;
    bool opened = parts(g, expr, &a, &b, out);
    size_t site = cg_site(g, expr->pos);
    switch (op) {
    case LIB_SQUARE_ROOT: sb_appendf(out, "es_square_root(%zu, %s)", site, a); break;
    case LIB_POWER: sb_appendf(out, "es_power(%zu, %s, %s)", site, a, b); break;
    case LIB_ABSOLUTE: sb_appendf(out, "fabs(%s)", a); break;
    case LIB_ROUNDED: sb_appendf(out, "es_round_half(%s)", a); break;
    case LIB_ROUNDED_UP: sb_appendf(out, "ceil(%s)", a); break;
    case LIB_ROUNDED_DOWN: sb_appendf(out, "floor(%s)", a); break;
    case LIB_ROUNDED_PLACES: sb_appendf(out, "es_round_places(%zu, %s, %s)", site, a, b); break;
    case LIB_BIGGER: sb_appendf(out, "es_bigger(%s, %s)", a, b); break;
    case LIB_SMALLER: sb_appendf(out, "es_smaller(%s, %s)", a, b); break;
    case LIB_PI: sb_append(out, "3.14159265358979323846"); break;
    case LIB_RANDOM_WHOLE: sb_appendf(out, "es_random_whole(%zu, %s, %s)", site, a, b); break;
    case LIB_RANDOM_DECIMAL: sb_appendf(out, "es_random_decimal(%zu, %s, %s)", site, a, b); break;
    case LIB_SHUFFLED: sb_appendf(out, "es_list_shuffled(%s)", a); break;
    case LIB_DIE: sb_append(out, "es_random_whole(0, 1, 6)"); break;
    case LIB_NOW: sb_append(out, "es_now()"); break;
    case LIB_TODAY: sb_append(out, "es_today(0)"); break;
    case LIB_TOMORROW: sb_append(out, "es_today(1)"); break;
    case LIB_YESTERDAY: sb_append(out, "es_today(-1)"); break;
    case LIB_DAYS_AGO: sb_appendf(out, "es_days_away(%zu, %s, -1)", site, a); break;
    case LIB_DAYS_FROM_NOW: sb_appendf(out, "es_days_away(%zu, %s, 1)", site, a); break;
    case LIB_DATE_PART:
        if (expr->as.library.part == DATE_WEEKDAY) {
            sb_appendf(out, "es_weekday_text(%s)", a);
        } else {
            sb_appendf(out, "es_date_part(%s, %d)", a, expr->as.library.part);
        }
        break;
    case LIB_IN_PERIOD: sb_appendf(out, "es_in_period(%s, %d)", a, expr->as.library.part); break;
    case LIB_DAYS_BETWEEN: sb_appendf(out, "es_days_between(%s, %s)", a, b); break;
    case LIB_BEFORE: sb_appendf(out, "(%s < %s)", a, b); break;
    case LIB_AFTER: sb_appendf(out, "(%s > %s)", a, b); break;
    case LIB_FILE_EXISTS: sb_appendf(out, "es_exists(%s, false)", a); break;
    case LIB_FOLDER_EXISTS: sb_appendf(out, "es_exists(%s, true)", a); break;
    case LIB_FILE_LINES: sb_appendf(out, "es_text_lines(es_read_file(%zu, %s))", site, a); break;
    case LIB_FILES_IN: sb_appendf(out, "es_folder_list(%zu, %s, false)", site, a); break;
    case LIB_FOLDERS_IN: sb_appendf(out, "es_folder_list(%zu, %s, true)", site, a); break;
    case LIB_FILE_SIZE: sb_appendf(out, "es_file_size(%zu, %s)", site, a); break;
    case LIB_FILE_CHANGED: sb_appendf(out, "es_file_changed(%zu, %s)", site, a); break;
    case LIB_HALF:
        if (type_is_money(expr->type)) sb_appendf(out, "es_money_divide(%zu, %s, 2.0)", cg_site(g, expr->pos), a);
        else sb_appendf(out, "(%s * 0.5)", a);
        break;
    case LIB_TWICE:
        if (type_is_money(expr->type)) sb_appendf(out, "es_money_times(%zu, %s, 2.0)", cg_site(g, expr->pos), a);
        else sb_appendf(out, "(%s * 2.0)", a);
        break;
    case LIB_PERCENT_OF:  // money: rounded to the cent
        if (type_is_money(expr->type)) {
            cg_plan(g, expr->pos, "a percent of money: whole cents, rounded to the cent (half away from zero).");
            sb_appendf(out, "es_money_percent(%zu, %s, %s)", cg_site(g, expr->pos), a, b);
        }
        else sb_appendf(out, "(%s * %s / 100.0)", a, b);
        break;
    case LIB_REASON: cg_emit_var(g, out, "0reason", TYPE_TEXT); break;
    case LIB_PERIOD_KEY:
        if (expr->as.library.part == DATE_YEAR) {
            sb_appendf(out, "es_date_part(%s, %d)", a, DATE_YEAR);
        } else {
            sb_appendf(out, "es_period_text(%s, %d)", a, expr->as.library.part);
        }
        break;
    case LIB_IS_INFINITE: sb_appendf(out, "%sisinf(%s)", expr->as.library.part ? "!" : "", a); break;
    default: break;
    }
    if (opened) sb_append_char(out, ')');
}

// Whether working out the value has effects (reads the clock or a file, can fail, or is random).
bool cg_library_effects(const Codegen *g, const Expr *expr) {
    switch (expr->as.library.op) {
    case LIB_PI: return false;
    case LIB_WORKED: return cg_has_effects(g, expr->as.library.a);
    case LIB_ABSOLUTE:
    case LIB_HALF:
    case LIB_TWICE:
    case LIB_PERCENT_OF:
    case LIB_IS_INFINITE:
    case LIB_ROUNDED:
    case LIB_ROUNDED_UP:
    case LIB_ROUNDED_DOWN:
    case LIB_BIGGER:
    case LIB_SMALLER:
    case LIB_DATE_PART:
    case LIB_DAYS_BETWEEN:
    case LIB_BEFORE:
    case LIB_AFTER:
        return cg_has_effects(g, expr->as.library.a) || (expr->as.library.b && cg_has_effects(g, expr->as.library.b));
    default: return true;
    }
}

// format_two is "es_rename(%zu, %s, %s, ...)": the site, then the two parts.
static void action(Codegen *g, const char *format_two, const char *a, const char *b, size_t site, const char *close) {
    cg_indent(g);
    sb_appendf(&g->body, format_two, site, a, b);
    sb_appendf(&g->body, "%s;\n", close);
}

// "save a bar chart of X as F": X as plain numbers (amounts in units, not
// cents) and the title kept in the hidden names the checker's call reads,
// then that call's picture written into F.
static void emit_chart(Codegen *g, const Stmt *stmt) {
    const char *name = stmt->as.library.name.text;
    const Expr *values = stmt->as.library.a, *title = stmt->as.library.parts.items[0];
    const Expr *call = stmt->as.library.parts.items[1];
    Type shown = call->as.call.args.items[0]->type;
    StrBuf code;
    sb_init(&code, g->arena);
    cg_emit_expr(g, values, &code);
    if (shown != values->type) {  // amounts: their numbers of units
        code.data = arena_sprintf(g->arena, "%s(%s)", type_is_map(shown) ? "es_map_units" : "es_list_units", code.data);
    }
    cg_declare_name(g, name, shown);
    cg_emit_store(g, name, shown, code.data);
    if (title) {
        const char *title_name = arena_sprintf(g->arena, "%st", name);
        cg_declare_name(g, title_name, TYPE_TEXT);
        cg_emit_store(g, title_name, TYPE_TEXT, cg_operand(g, title, false));
    }
    Stmt write = *stmt;
    write.kind = STMT_WRITE_FILE;
    memset(&write.as, 0, sizeof write.as);
    write.as.file_write.text = stmt->as.library.parts.items[1];
    write.as.file_write.path = stmt->as.library.b;
    cg_emit_stmt(g, &write);
}

void cg_emit_library_stmt(Codegen *g, const Stmt *stmt) {
    if (stmt->as.library.op >= DO_COMPLAIN) {
        cg_emit_system_stmt(g, stmt);
        return;
    }
    if (stmt->as.library.op == DO_SAVE_CHART) {
        emit_chart(g, stmt);
        return;
    }
    Expr *exprs[] = {stmt->as.library.a, stmt->as.library.b};
    const char *v[2] = {NULL, NULL};
    StrBuf head;
    sb_init(&head, g->arena);
    bool opened = cg_ordered(g, exprs, 2, v, &head);
    const char *a = opened ? arena_sprintf(g->arena, "%s%s", head.data, v[0]) : v[0];  // the temporaries come first
    const char *b = v[1];
    const char *close = opened ? ")" : "";
    size_t site = stmt->as.library.a ? cg_site(g, stmt->as.library.a->pos) : 0;
    bool folder = stmt->as.library.folder;
    switch (stmt->as.library.op) {
    case DO_WAIT:
        cg_indent(g);
        sb_appendf(&g->body, "es_wait(%zu, (%s) * %.17g)%s;\n", site, a, stmt->as.library.scale, close);
        break;
    case DO_SAME_RANDOM: cg_indent(g), sb_append(&g->body, "es_random_fix();\n"); break;
    case DO_CLEAR_SCREEN: cg_indent(g), sb_append(&g->body, "es_clear_screen();\n"); break;
    case DO_SHUFFLE: cg_indent(g), sb_appendf(&g->body, "es_shuffle(%s)%s;\n", a, close); break;
    case DO_MAKE_FILE:
    case DO_MAKE_FOLDER:
        cg_indent(g);
        sb_appendf(&g->body, "es_make(%zu, %s, %s)%s;\n", site, a, stmt->as.library.op == DO_MAKE_FOLDER ? "true" : "false",
                   close);
        break;
    case DO_RENAME:
        action(g, folder ? "es_rename(%zu, %s, %s, true)" : "es_rename(%zu, %s, %s, false)", a, b, site, close);
        break;
    case DO_COPY: action(g, "es_copy_file(%zu, %s, %s)", a, b, site, close); break;
    case DO_MOVE: action(g, folder ? "es_move(%zu, %s, %s, true)" : "es_move(%zu, %s, %s, false)", a, b, site, close); break;
    case DO_DELETE_FILE:
    case DO_DELETE_FOLDER:
        cg_indent(g);
        sb_appendf(&g->body, "es_delete(%zu, %s, %s, %s)%s;\n", site, a,
                   stmt->as.library.op == DO_DELETE_FOLDER ? "true" : "false",
                   stmt->as.library.everything ? "true" : "false", close);
        break;
    case DO_READ_CSV: {
        const char *name = stmt->as.library.name.text;
        Type type = stmt->as.library.type;
        cg_declare_name(g, name, type);
        cg_emit_new_cell(g, name, type);
        cg_emit_store(g, name, type, arena_sprintf(g->arena, "es_read_csv(%zu, %s)", site, a));
        break;
    }
    case DO_READ_TABLE: {  // the rows, as records of the kind made for this sentence
        const char *name = stmt->as.library.name.text;
        Type type = stmt->as.library.type;
        cg_declare_name(g, name, type);
        cg_emit_new_cell(g, name, type);
        cg_emit_store(g, name, type, arena_sprintf(g->arena, "es_read_table(%zu, %s, &es_rt%zu)", site, a,
                                                   type_record_index(type_item(type))));
        break;
    }
    case DO_SAVE_CSV: {
        Type saved = stmt->as.library.type;
        cg_indent(g);
        if (type_is_map(saved)) {  // a "key" and a "value" column
            Type key = ast_map_key(g->records, saved), value = ast_map_value(g->records, saved);
            sb_appendf(&g->body, "es_save_map(%zu, %s, %s, '%c', '%c')%s;\n", site, a, b, cg_field_kind(key),
                       cg_field_kind(value), close);
        } else if (type_is_record(type_item(saved))) {  // a column per field
            sb_appendf(&g->body, "es_save_records(%zu, %s, %s, &es_rt%zu)%s;\n", site, a, b,
                       type_record_index(type_item(saved)), close);
        } else {  // a list of maps from column names to values
            const char *kind = cg_value_suffix(ast_map_value(g->records, type_item(saved)));
            sb_appendf(&g->body, "es_save_csv(%zu, %s, %s, '%s')%s;\n", site, a, b, kind, close);
        }
        break;
    }
    default: break;
    }
}

// "try to S. if that fails, ...": the check for S's file or folder first
// (NULL if it can go ahead, otherwise the reason), then S, or the fallback
// with "the reason" (kept in a hidden name).
void cg_emit_try(Codegen *g, const Stmt *stmt) {
    const Stmt *attempt = stmt->as.attempt.attempt;
    const char *check = NULL;
    if (attempt->kind == STMT_READ_FILE) {
        check = arena_sprintf(g->arena, "es_try_read(%s)", cg_operand(g, attempt->as.read_file.path, false));
    } else if (attempt->kind == STMT_WRITE_FILE || attempt->kind == STMT_APPEND_FILE) {
        check = arena_sprintf(g->arena, "es_try_write(%s)", cg_operand(g, attempt->as.file_write.path, false));
    } else {
        const Expr *a = attempt->as.library.a, *b = attempt->as.library.b;
        bool folder = attempt->as.library.folder;
        switch (attempt->as.library.op) {
        case DO_READ_TABLE:
        case DO_READ_CSV: check = arena_sprintf(g->arena, "es_try_read(%s)", cg_operand(g, a, false)); break;
        case DO_SAVE_CSV: check = arena_sprintf(g->arena, "es_try_write(%s)", cg_operand(g, b, false)); break;
        case DO_MAKE_FILE:
        case DO_MAKE_FOLDER:
            check = arena_sprintf(g->arena, "es_try_make(%s, %s)", cg_operand(g, a, false),
                                  attempt->as.library.op == DO_MAKE_FOLDER ? "true" : "false");
            break;
        case DO_DELETE_FILE:
        case DO_DELETE_FOLDER:
            check = arena_sprintf(g->arena, "es_try_find(%s, %s)", cg_operand(g, a, false),
                                  attempt->as.library.op == DO_DELETE_FOLDER ? "true" : "false");
            break;
        default: {  // rename, copy, move
            const char *from = cg_operand(g, a, false);
            check = arena_sprintf(g->arena, "es_try_two(%s, %s, %s, %s)", from, cg_operand(g, b, false),
                                  folder ? "true" : "false", attempt->as.library.op == DO_MOVE ? "true" : "false");
            break;
        }
        }
    }
    cg_declare_name(g, "0reason", TYPE_TEXT);
    StrBuf reason;
    sb_init(&reason, g->arena);
    cg_append_var(g, &reason, "0reason", TYPE_TEXT);
    size_t id = ++g->loop_ids;
    cg_indent(g);
    sb_appendf(&g->body, "{\n");
    g->depth++;
    cg_indent(g);
    sb_appendf(&g->body, "EsText *es_why%zu = %s;\n", id, check);
    cg_indent(g);
    sb_appendf(&g->body, "if (!es_why%zu) {\n", id);
    g->depth++;
    cg_emit_stmt(g, attempt);  // in the scope around it: the names it makes are used after it
    g->depth--;
    cg_indent(g);
    sb_appendf(&g->body, "} else {\n");
    g->depth++;
    cg_indent(g);
    sb_appendf(&g->body, "es_set(&%s, es_why%zu);\n", reason.data, id);
    g->depth--;
    cg_emit_block(g, &stmt->as.attempt.fallback);
    cg_indent(g);
    sb_append(&g->body, "}\n");
    g->depth--;
    cg_indent(g);
    sb_append(&g->body, "}\n");
}

// "for example, X is Y.": es_example(site, right, what X is) where it's
// written. For "X is Y" (and "is not"), X is worked out once, into a hidden
// name, so it can be shown when the example is wrong.
void cg_emit_example(Codegen *g, const Stmt *stmt) {
    const Expr *condition = stmt->as.value;
    size_t site = cg_site(g, condition->pos);
    bool equal = condition->kind == EXPR_BINARY &&
                 (condition->as.binary.op == BINARY_EQUAL || condition->as.binary.op == BINARY_NOT_EQUAL) &&
                 !type_is_func(condition->as.binary.left->type);
    if (!equal) {
        cg_indent(g);
        sb_appendf(&g->body, "es_example(%zu, %s, NULL, false);\n", site, cg_operand(g, condition, false));
        return;
    }
    const Expr *left = condition->as.binary.left;
    const char *hidden = arena_sprintf(g->arena, "0example%zu", ++g->loop_ids);
    cg_declare_name(g, hidden, left->type);
    StrBuf value;
    sb_init(&value, g->arena);
    cg_emit_expr(g, left, &value);
    cg_emit_store(g, hidden, left->type, value.data);
    Expr *name = ast_new_expr(g->arena, EXPR_NAME, left->pos);
    name->as.name = hidden;
    name->type = left->type;
    Expr *test = ast_new_expr(g->arena, EXPR_BINARY, condition->pos);
    *test = *condition;
    test->as.binary.left = name;
    const char *right = cg_operand(g, test, false);  // in order: test, then the name shown
    const char *shown = cg_operand(g, name, true);
    cg_indent(g);
    sb_appendf(&g->body, "es_example(%zu, %s, %s, %s);\n", site, right, shown,
               left->type == TYPE_TEXT ? "true" : "false");
}
