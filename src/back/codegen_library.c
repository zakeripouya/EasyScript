// The standard library: each value or sentence is a call into the runtime
// (es_math.h, es_time.h, es_files.h, es_csv.h), its parts worked out left to right.

#include <assert.h>
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
    case LIB_HALF: sb_appendf(out, "(%s * 0.5)", a); break;
    case LIB_TWICE: sb_appendf(out, "(%s * 2.0)", a); break;
    case LIB_PERCENT_OF: sb_appendf(out, "(%s * %s / 100.0)", a, b); break;
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

void cg_emit_library_stmt(Codegen *g, const Stmt *stmt) {
    if (stmt->as.library.op >= DO_COMPLAIN) {
        cg_emit_system_stmt(g, stmt);
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
    case DO_SAVE_CSV: {
        Type row = type_item(stmt->as.library.type);
        const char *kind = cg_value_suffix(ast_map_value(g->records, row));
        cg_indent(g);
        sb_appendf(&g->body, "es_save_csv(%zu, %s, %s, '%s')%s;\n", site, a, b, kind, close);
        break;
    }
    default: break;
    }
}
