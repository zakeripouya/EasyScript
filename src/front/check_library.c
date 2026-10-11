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
    "\"the size of file\"", "\"the last change of file\"",
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

// save L as csv file F: L is a list of maps from text to numbers, text or yes/no.
static void check_save_csv(Checker *c, const Stmt *stmt) {
    const Expr *list = stmt->as.library.a;
    TypeVar var = checker_check_expr(c, list);
    TypeVar row = checker_require_list(c, var, list, "I can only save a list of maps as csv");
    TypeVar key, value;
    if (checker_require_map(c, row, list, "Each row saved as csv has to be a map from column names to values", NULL,
                            &key, &value)) {
        checker_require(c, key, TYPE_TEXT, list, "The column names of a csv file have to be text", NULL);
        types_defer_one_of(c, list, value, value, KIND_NUMBER | KIND_TEXT | KIND_YESNO,
                           "The values saved in a csv file have to be numbers, text or yes/no values");
    }
    TypedSlot slot = {(Type *)&stmt->as.library.type, var};
    vec_push(c->arena, &c->slots, slot);
    action_text(c, stmt->as.library.b, "The name of a file");
}

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
    }
}

// --- Where dates can't go yet ---------------------------------------------------

static bool holds_date(const Records *records, Type type) {
    if (type_is_list(type)) return holds_date(records, type_base(type));
    if (type_is_map(type)) {
        return ast_map_key(records, type) == TYPE_DATE || holds_date(records, ast_map_value(records, type));
    }
    return type_present(type) == TYPE_DATE;
}

static bool report_date(Checker *c, Span span, const char *where) {
    diag_error(c->diag, span, "Dates can't be kept in %s yet.", where);
    diag_note(c->diag, "Keep the date as text instead, like \"today as text\", or keep its parts, like \"today's year\".");
    return true;
}

void checker_reject_dates(Checker *c) {
    for (size_t i = 0; i < c->typed.len; i++) {
        const Expr *expr = c->typed.items[i].expr;
        if ((type_is_list(expr->type) || type_is_map(expr->type)) && holds_date(c->records, expr->type)) {
            report_date(c, expr->pos.span, type_is_list(expr->type) ? "a list" : "a map");
            return;
        }
    }
    for (size_t i = 0; i < c->records->len; i++) {
        const Stmt *def = c->records->items[i].definition;
        for (size_t f = 0; def && def->kind == STMT_RECORD && f < def->as.record.fields.len; f++) {
            const Field *field = &def->as.record.fields.items[f];
            if (holds_date(c->records, field->type)) {
                report_date(c, field->pos.span, "a record");
                return;
            }
        }
    }
    for (size_t i = 0; i < c->records->sums.len; i++) {
        const Stmt *def = c->records->sums.items[i].definition;
        for (size_t v = 0; def && v < def->as.sum.variants.len; v++) {
            const VariantDef *variant = &def->as.sum.variants.items[v];
            for (size_t f = 0; f < variant->fields.len; f++) {
                if (holds_date(c->records, variant->fields.items[f].type)) {
                    report_date(c, variant->fields.items[f].pos.span, "a kind with variants");
                    return;
                }
            }
        }
    }
}
