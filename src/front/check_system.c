// The kinds of the system tools: the words given to the program, the text
// sent to it, settings, the keyboard, running other programs, complaining and
// stopping with a code.

#include <string.h>
#include "front/check_internal.h"

static const char *const run_example = "run \"git\" with \"status\" and call the result r";

// The kind "program result", or false (with an error at `at`) when the
// program defines its own kind with that name.
static bool program_result(Checker *c, Span at, Type *type) {
    if (c->records->program_result) {
        *type = type_record(c->records->program_result - 1);
        return true;
    }
    diag_error(c->diag, at, "This program defines its own kind called \"program result\", so running a program "
                            "can't make one.");
    diag_note(c->diag, "Give your kind another name, like \"build result\".");
    return false;
}

TypeVar checker_check_system(Checker *c, const Expr *expr) {
    const Expr *a = expr->as.library.a;
    size_t line = expr->pos.line;
    switch (expr->as.library.op) {
    case LIB_PROGRAM_WORDS:
    case LIB_SENT_LINES: return types_new(c, type_list_of(TYPE_TEXT), line);
    case LIB_PROGRAM_COUNT: return types_new(c, TYPE_NUMBER, line);
    case LIB_SENT_TEXT: return types_new(c, TYPE_TEXT, line);
    case LIB_PROGRAM_WORD:
        checker_require(c, checker_check_expr(c, a), TYPE_NUMBER, a, "The position of a word has to be a number", NULL);
        return types_new(c, TYPE_TEXT, line);
    case LIB_SETTING:
        checker_require(c, checker_check_expr(c, a), TYPE_TEXT, a, "The name of a setting has to be text, like \"HOME\"",
                        NULL);
        return types_new(c, TYPE_MAYBE + TYPE_TEXT, line);  // nothing when it isn't set
    case LIB_KEY_PRESSED: return types_new(c, TYPE_MAYBE + TYPE_TEXT, line);
    case LIB_WORKED: {
        TypeVar var = checker_check_expr(c, a);
        Type type;
        if (program_result(c, expr->as.library.op_span, &type)) {
            checker_require(c, var, type, a, "Only a program result can have worked",
                            arena_sprintf(c->arena, "Run a program first, like \"%s\", then check \"if r worked:\".",
                                          run_example));
        }
        return types_new(c, TYPE_YESNO, line);
    }
    default: return types_new(c, TYPE_ERROR, line);
    }
}

// Each word after "with": text, a number, or a list of text. One whose kind
// isn't known yet has to be text.
static void check_part(Checker *c, const Expr *part) {
    static const char *const message = "Each word given to a program has to be text, a number or a list of text";
    TypeVar var = checker_check_expr(c, part);
    Type type = types_of(c, var);
    if (type == TYPE_NUMBER || type == TYPE_ERROR) return;
    if (type_is_list(type)) {  // its items have to be text
        if (!types_require(c, types_item(c, var), TYPE_TEXT, part->pos.line)) {
            diag_error(c->diag, part->pos.span, "%s, but this is %s.", message, checker_kind(c, var));
        }
        return;
    }
    checker_require(c, var, TYPE_TEXT, part, message, NULL);
}

static void check_run(Checker *c, const Stmt *stmt) {
    const Expr *program = stmt->as.library.a;
    checker_require(c, checker_check_expr(c, program), TYPE_TEXT, program,
                    "The program to run has to be text, like \"git\"", NULL);
    for (size_t i = 0; i < stmt->as.library.parts.len; i++) check_part(c, stmt->as.library.parts.items[i]);
    if (!stmt->as.library.name.text) return;
    Type type;
    if (program_result(c, stmt->as.library.name.pos.span, &type)) {
        checker_make_or_store(c, &stmt->as.library.name, types_new(c, type, stmt->pos.line));
    }
}

void checker_check_system_stmt(Checker *c, const Stmt *stmt) {
    switch (stmt->as.library.op) {
    case DO_COMPLAIN: checker_check_expr(c, stmt->as.library.a); break;
    case DO_RUN: check_run(c, stmt); break;
    case DO_WAIT_KEY:
        checker_make_or_store(c, &stmt->as.library.name, types_new(c, TYPE_TEXT, stmt->pos.line));
        break;
    default: break;
    }
}

// stop the program with code N (or "saying it failed", code 1).
void checker_check_stop(Checker *c, const Stmt *stmt) {
    const Expr *code = stmt->as.stop_code;
    if (code) checker_require(c, checker_check_expr(c, code), TYPE_NUMBER, code, "The code has to be a number", NULL);
}
