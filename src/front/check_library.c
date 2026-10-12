// The kinds of the standard library's values and sentences: math on
// numbers, random numbers and items, dates, files and folders, and csv.

#include <string.h>
#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

// How messages name each value: "The square root needs a number, but this is text."
static const char *const op_display[] = {
    "\"the square root of\"", "\"to the power of\"", "\"the absolute value of\"", "\"rounded\"", "\"rounded up\"",
    "\"rounded down\"", "\"rounded to ... decimal places\"", "\"the bigger of\"", "\"the smaller of\"", "pi",
    "\"a random number from\"", "\"a random decimal between\"", "\"pick one of\"", "\"shuffled\"", "\"roll a die\"",
    "\"the time now\"", "today", "tomorrow", "yesterday", "\"days ago\"", "\"days from now\"", "a date's part",
    "\"is in\"", "\"the number of days between\"", "\"is before\"", "\"is after\"", "\"file ... exists\"",
    "\"folder ... exists\"", "\"the lines of file\"", "\"the files in\"", "\"the folders in\"",
    "\"the size of file\"", "\"the last change of file\"", "\"half of\"", "\"twice\"", "\"percent of\"",
    "\"is infinite\"", "\"the reason\"", "\"per\"",
};

static TypeVar require(Checker *c, const Expr *operand, Type type, const char *display, const char *needs) {
    TypeVar var = checker_check_expr(c, operand);
    checker_require(c, var, type, operand, arena_sprintf(c->arena, "%s needs %s", display, needs), NULL);
    return var;
}

static void number(Checker *c, const Expr *operand, const char *display) {
    require(c, operand, TYPE_NUMBER, display, "a number");
}

static void date(Checker *c, const Expr *operand, const char *display) {
    require(c, operand, TYPE_DATE, display, "a date");
}

static void text(Checker *c, const Expr *operand, const char *display) {
    require(c, operand, TYPE_TEXT, display, "text (a path, like \"notes.txt\")");
}

static TypeVar list_of_text(Checker *c, size_t line) {
    return types_new(c, type_list_of(TYPE_TEXT), line);
}

TypeVar checker_check_library(Checker *c, const Expr *expr) {
    if (expr->as.library.op >= LIB_PROGRAM_WORDS) return checker_check_system(c, expr);
    const char *display = op_display[expr->as.library.op];
    if (expr->as.library.op == LIB_DATE_PART) {
        display = arena_sprintf(c->arena, "\"the %s of\"", ast_date_part_name(expr->as.library.part));
    }
    const Expr *a = expr->as.library.a, *b = expr->as.library.b;
    size_t line = expr->pos.line;
    switch (expr->as.library.op) {
    case LIB_POWER:
    case LIB_ROUNDED_PLACES:
    case LIB_BIGGER:
    case LIB_SMALLER:
    case LIB_RANDOM_WHOLE:
    case LIB_RANDOM_DECIMAL: number(c, b, display); /* fall through */
    case LIB_SQUARE_ROOT:
    case LIB_ABSOLUTE:
    case LIB_ROUNDED:
    case LIB_ROUNDED_UP:
    case LIB_ROUNDED_DOWN:
        number(c, a, display);
        return types_new(c, TYPE_NUMBER, line);
    case LIB_PI:
    case LIB_DIE: return types_new(c, TYPE_NUMBER, line);
    case LIB_PICK: return checker_require_list(c, checker_check_expr(c, a), a, "I can only pick one of the items of a list");
    case LIB_SHUFFLED: {
        TypeVar list = checker_check_expr(c, a);
        checker_require_list(c, list, a, "I can only shuffle a list");
        return list;
    }
    case LIB_NOW:
    case LIB_TODAY:
    case LIB_TOMORROW:
    case LIB_YESTERDAY: return types_new(c, TYPE_DATE, line);
    case LIB_DAYS_AGO:
    case LIB_DAYS_FROM_NOW:
        number(c, a, display);
        return types_new(c, TYPE_DATE, line);
    case LIB_DATE_PART:
        date(c, a, display);
        return types_new(c, expr->as.library.part == DATE_WEEKDAY ? TYPE_TEXT : TYPE_NUMBER, line);
    case LIB_IN_PERIOD:
        date(c, a, display);
        return types_new(c, TYPE_YESNO, line);
    case LIB_BEFORE:
    case LIB_AFTER:
        date(c, a, display);
        date(c, b, display);
        return types_new(c, TYPE_YESNO, line);
    case LIB_DAYS_BETWEEN:
        date(c, a, display);
        date(c, b, display);
        return types_new(c, TYPE_NUMBER, line);
    case LIB_FILE_EXISTS:
    case LIB_FOLDER_EXISTS:
        text(c, a, display);
        return types_new(c, TYPE_YESNO, line);
    case LIB_FILE_LINES:
    case LIB_FILES_IN:
    case LIB_FOLDERS_IN:
        text(c, a, display);
        return list_of_text(c, line);
    case LIB_FILE_SIZE:
        text(c, a, display);
        return types_new(c, TYPE_NUMBER, line);
    case LIB_FILE_CHANGED:
        text(c, a, display);
        return types_new(c, TYPE_DATE, line);
    case LIB_PERCENT_OF:
    case LIB_HALF:
    case LIB_TWICE: {  // "10 percent of $19.99", "half of rent": money stays money
        const Expr *amount = expr->as.library.op == LIB_PERCENT_OF ? b : a;
        if (expr->as.library.op == LIB_PERCENT_OF) number(c, a, display);
        TypeVar var = checker_check_expr(c, amount);
        if (type_is_money(types_of(c, var))) return var;
        checker_require(c, var, TYPE_NUMBER, amount, arena_sprintf(c->arena, "%s needs a number", display), NULL);
        return types_new(c, TYPE_NUMBER, line);
    }
    case LIB_IS_INFINITE:
        number(c, a, display);
        return types_new(c, TYPE_YESNO, line);
    case LIB_REASON: return types_new(c, TYPE_TEXT, line);
    case LIB_PERIOD_KEY:  // "per year" groups by a number, the others by text ("October 2026")
        date(c, a, arena_sprintf(c->arena, "\"per %s\"", ast_date_part_name(expr->as.library.part)));
        return types_new(c, expr->as.library.part == DATE_YEAR ? TYPE_NUMBER : TYPE_TEXT, line);
    default: break;
    }
    return error_var(c);
}

bool checker_library_name(Checker *c, const Expr *expr, TypeVar *result) {
    static const struct {
        const char *name;
        LibraryOp op;
    } names[] = {{"pi", LIB_PI}, {"today", LIB_TODAY}, {"tomorrow", LIB_TOMORROW}, {"yesterday", LIB_YESTERDAY}};
    for (size_t i = 0; i < sizeof names / sizeof names[0]; i++) {
        if (strcmp(expr->as.name, names[i].name) != 0 || expr->module) continue;
        Expr *e = (Expr *)expr;
        Span span = e->pos.span;
        memset(&e->as, 0, sizeof e->as);
        e->kind = EXPR_LIBRARY;
        e->as.library.op = names[i].op;
        e->as.library.op_span = span;
        *result = checker_check_library(c, e);
        return true;
    }
    return false;
}

int checker_date_part(const char *name) {
    static const char *const parts[] = {"year", "month", "day", "weekday", "hour", "minute"};
    for (int i = 0; i < 6; i++) {
        if (strcmp(name, parts[i]) == 0) return i;
    }
    return -1;
}

TypeVar checker_date_field(Checker *c, const Expr *expr, TypeVar base) {
    int part = checker_date_part(expr->as.field.name.text);
    if (part < 0) {
        diag_error(c->diag, expr->as.field.name.pos.span, "A date has no \"%s\".", expr->as.field.name.display);
        diag_note(c->diag, "A date has a year, a month, a day, a weekday, an hour and a minute, like \"today's year\".");
        return error_var(c);
    }
    Expr *e = (Expr *)expr;
    Expr *record = e->as.field.record;
    Span span = e->as.field.name.pos.span;
    memset(&e->as, 0, sizeof e->as);
    e->kind = EXPR_LIBRARY;
    e->as.library.op = LIB_DATE_PART;
    e->as.library.op_span = span;
    e->as.library.a = record;
    e->as.library.part = part;
    (void)base;
    return types_new(c, part == DATE_WEEKDAY ? TYPE_TEXT : TYPE_NUMBER, expr->pos.line);
}

// --- Sentences ---------------------------------------------------------------

static void action_text(Checker *c, const Expr *operand, const char *what) {
    checker_require(c, checker_check_expr(c, operand), TYPE_TEXT, operand,
                    arena_sprintf(c->arena, "%s has to be text, like \"notes.txt\"", what), NULL);
}

// read csv file F and call it NAME: a list of maps from text (each column's name) to text.
static void check_read_csv(Checker *c, const Stmt *stmt) {
    action_text(c, stmt->as.library.a, "The name of a file");
    Type row = ast_map_type(c->arena, c->records, TYPE_TEXT, TYPE_TEXT);
    TypeVar rows = types_new(c, type_list_of(row), stmt->pos.line);
    checker_make_or_store(c, &stmt->as.library.name, rows);
    TypedSlot slot = {(Type *)&stmt->as.library.type, rows};
    vec_push(c->arena, &c->slots, slot);
}

// save L as csv file F: L is a list of maps from text to numbers, text or
// yes/no; or a list of records (a column per field); or a map (a "key" and a
// "value" column).
static void check_save_csv(Checker *c, const Stmt *stmt) {
    const Expr *list = stmt->as.library.a;
    TypeVar var = checker_check_expr(c, list);
    Type type = types_of(c, var);
    TypedSlot slot = {(Type *)&stmt->as.library.type, var};
    if (type_is_map(type)) {
        vec_push(c->arena, &c->slots, slot);
        types_defer_one_of(c, list, types_item(c, var), types_item(c, var), KIND_NUMBER | KIND_TEXT | KIND_YESNO | KIND_DATE | KIND_MONEY,
                           "The values saved in a csv file have to be numbers, text, yes/no values or dates");
        action_text(c, stmt->as.library.b, "The name of a file");
        return;
    }
    if (type_is_list(type) && type_is_record(type_item(type)) && !type_is_maybe(type_item(type))) {
        vec_push(c->arena, &c->slots, slot);
        action_text(c, stmt->as.library.b, "The name of a file");
        return;
    }
    TypeVar row = checker_require_list(c, var, list, "I can only save a list of maps or records, or a map, as csv");
    TypeVar key, value;
    if (checker_require_map(c, row, list, "Each row saved as csv has to be a map from column names to values", NULL,
                            &key, &value)) {
        checker_require(c, key, TYPE_TEXT, list, "The column names of a csv file have to be text", NULL);
        types_defer_one_of(c, list, value, value, KIND_NUMBER | KIND_TEXT | KIND_YESNO,
                           "The values saved in a csv file have to be numbers, text or yes/no values");
    }
    vec_push(c->arena, &c->slots, slot);
    action_text(c, stmt->as.library.b, "The name of a file");
}

static void check_chart(Checker *c, const Stmt *stmt);

void checker_check_library_stmt(Checker *c, const Stmt *stmt) {
    const Expr *a = stmt->as.library.a, *b = stmt->as.library.b;
    bool folder = stmt->as.library.folder;
    switch (stmt->as.library.op) {
    case DO_WAIT:
        checker_require(c, checker_check_expr(c, a), TYPE_NUMBER, a, "How long to wait has to be a number", NULL);
        break;
    case DO_SAME_RANDOM:
    case DO_CLEAR_SCREEN: break;
    case DO_SHUFFLE:
        checker_require_list(c, checker_check_expr(c, a), a, "I can only shuffle a list");
        checker_reject_fixed(c, a);
        break;
    case DO_MAKE_FILE:
    case DO_DELETE_FILE: action_text(c, a, "The name of a file"); break;
    case DO_MAKE_FOLDER:
    case DO_DELETE_FOLDER: action_text(c, a, "The name of a folder"); break;
    case DO_RENAME:
    case DO_COPY:
        action_text(c, a, folder ? "The name of a folder" : "The name of a file");
        action_text(c, b, "The new name");
        break;
    case DO_MOVE:
        action_text(c, a, folder ? "The name of a folder" : "The name of a file");
        action_text(c, b, "The name of a folder");
        break;
    case DO_READ_CSV: check_read_csv(c, stmt); break;
    case DO_SAVE_CSV: check_save_csv(c, stmt); break;
    case DO_SAVE_CHART: check_chart(c, stmt); break;
    case DO_READ_TABLE: {  // rows of the kind made for it; its fields come from use
        action_text(c, stmt->as.library.a, "The name of a file");
        checker_make_or_store(c, &stmt->as.library.name, types_new(c, stmt->as.library.type, stmt->pos.line));
        break;
    }
    case DO_COMPLAIN:
    case DO_RUN:
    case DO_WAIT_KEY: checker_check_system_stmt(c, stmt); break;
    }
}

// --- Charts ------------------------------------------------------------------------------

// A hidden name holding a value already checked (so it's checked once).
static Expr *hidden_name(Checker *c, const char *name, SourcePos pos, TypeVar var) {
    Symbol symbol = {name, "the chart's values", pos.line, NULL, false, var, false,
                     c->function ? c->function->definition : NULL, 0};
    vec_push(c->arena, &c->made, symbol);
    Expr *expr = ast_new_expr(c->arena, EXPR_NAME, pos);
    expr->as.name = name;
    expr->display = "the chart's values";
    return expr;
}

// "save a bar chart of X as F [, titled T]": X is a list of numbers (labelled
// 1, 2, 3, ...) or a map from text or numbers to numbers. Checks the call of
// lib/charts.es that draws it ("numbers chart", "text map chart" or "number
// map chart"), with X and T in hidden names; codegen writes what it gives
// back into F.
static void check_chart(Checker *c, const Stmt *stmt) {
    Stmt *s = (Stmt *)stmt;
    const Expr *values = stmt->as.library.a, *title = stmt->as.library.parts.items[0];
    TypeVar var = checker_check_expr(c, values);
    checker_require(c, checker_check_expr(c, stmt->as.library.b), TYPE_TEXT, stmt->as.library.b,
                    "The name of a file has to be text", NULL);
    TypeVar title_var = title ? checker_check_expr(c, title) : 0;
    if (title) checker_require(c, title_var, TYPE_TEXT, title, "A chart's title has to be text", NULL);
    Type shape = types_of(c, var);  // "the total amount per month": numbers, as nothing said they're money
    if ((type_is_list(shape) || type_is_map(shape)) && types_of(c, types_item(c, var)) == TYPE_UNKNOWN) {
        types_require(c, types_item(c, var), TYPE_NUMBER, stmt->pos.line);
    }
    Type type = types_of(c, var);
    if (type == TYPE_ERROR || (title && types_of(c, title_var) != TYPE_TEXT)) return;
    Type key = type_is_map(type) ? ast_map_key(c->records, type) : TYPE_UNKNOWN;
    const char *function = type == type_list_of(TYPE_NUMBER) ? "numbers chart"
                           : !type_is_map(type) || ast_map_value(c->records, type) != TYPE_NUMBER ? NULL
                           : key == TYPE_TEXT ? "text map chart"
                           : key == TYPE_NUMBER ? "number map chart" : NULL;
    if (!function) {
        diag_error(c->diag, values->pos.span, "A chart needs a list of numbers or a map to numbers, but this is %s.", checker_kind(c, var));
        diag_note(c->diag, "Write it like \"save a bar chart of the total amount of sales per month as \"sales.svg\"\".");
        diag_info(c->diag, DIAG_WRONG_KIND, NULL, "a list of numbers or a map to numbers");
        return;
    }
    const char *name = stmt->as.library.name.text;
    Expr *call = ast_new_expr(c->arena, EXPR_CALL, stmt->pos);
    call->as.call.name = function;
    call->display = function;
    call->module = "0charts";
    vec_push(c->arena, &call->as.call.args, hidden_name(c, name, values->pos, var));
    static const char *const kinds[] = {"bar", "line", "pie"};
    Expr *kind = ast_new_expr(c->arena, EXPR_TEXT, stmt->pos);
    kind->as.text.value = kinds[(int)stmt->as.library.scale];
    kind->as.text.len = strlen(kind->as.text.value);
    vec_push(c->arena, &call->as.call.args, kind);
    Expr *shown = ast_new_expr(c->arena, EXPR_TEXT, stmt->pos);  // no title: ""
    shown->as.text.value = "";
    if (title) shown = hidden_name(c, arena_sprintf(c->arena, "%st", name), title->pos, title_var);
    vec_push(c->arena, &call->as.call.args, shown);
    checker_check_expr(c, call);
    vec_push(c->arena, &s->as.library.parts, call);
}

// --- try to ... -------------------------------------------------------------------------

// The files and folders the sentence uses (a and b), or false if it's a
// sentence "try to" doesn't cover.
static bool tried_paths(const Stmt *stmt, const Expr **a, const Expr **b) {
    *a = *b = NULL;
    switch (stmt->kind) {
    case STMT_READ_FILE: *a = stmt->as.read_file.path; return true;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: *a = stmt->as.file_write.path; return true;
    case STMT_LIBRARY:
        switch (stmt->as.library.op) {
        case DO_READ_TABLE:
        case DO_READ_CSV:
        case DO_MAKE_FILE:
        case DO_MAKE_FOLDER:
        case DO_DELETE_FILE:
        case DO_DELETE_FOLDER: *a = stmt->as.library.a; return true;
        case DO_SAVE_CSV: *a = stmt->as.library.b; return true;
        case DO_RENAME:
        case DO_COPY:
        case DO_MOVE:
            *a = stmt->as.library.a;
            *b = stmt->as.library.b;
            return true;
        default: return false;
        }
    default: return false;
    }
}

// "try to S. if that fails, ...": S uses a file or a folder (written as text
// or a name, since it's looked at twice: once to check, once to use).
void checker_check_try(Checker *c, const Stmt *stmt) {
    const Stmt *attempt = stmt->as.attempt.attempt;
    const Expr *a, *b;
    if (!tried_paths(attempt, &a, &b)) {
        diag_error(c->diag, attempt->pos.span, "\"try to\" only works with a file or a folder.");
        diag_note(c->diag, "Use it to read, write, make or delete one, like \"try to read file \"notes.txt\" and call it "
                           "notes.\"");
        diag_info(c->diag, DIAG_NOT_ALLOWED, "a sentence that uses a file or a folder", NULL);
    } else {
        const Expr *paths[] = {a, b};
        for (size_t i = 0; i < 2; i++) {
            if (!paths[i] || paths[i]->kind == EXPR_TEXT || paths[i]->kind == EXPR_NAME) continue;
            diag_error(c->diag, paths[i]->pos.span, "In \"try to\", the file's name has to be text or a name.");
            diag_note(c->diag, "Keep it in a name first, like \"let path be ...\", and use the name here.");
            diag_info(c->diag, DIAG_NOT_ALLOWED, "a name worked out here", "text");
        }
    }
    checker_check_statements(c, &(Block){(Stmt **)&stmt->as.attempt.attempt, 1, 1});
    checker_check_block(c, &stmt->as.attempt.fallback, stmt->pos.line, "\"if that fails\"");
}
