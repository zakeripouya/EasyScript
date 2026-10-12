// Values of the standard library. Each starts with its own words, so they
// never overlap other forms, and none is a name a program could clash with
// (pi, today, tomorrow and yesterday are names, worked out by the checker).
//
//   the square root of X        X to the power of Y      the absolute value of X
//   X rounded [up | down | to N decimal places]          the bigger/smaller of X and Y
//   a random number from A to B     a random decimal between A and B
//   pick one of L / a random item of L     L shuffled     roll a die
//   the time now     N days ago     N days from now     the year of D (or D's year)
//   D is before E    D is after E   D is in this month   the number of days between D and E
//   file F exists    folder F exists    the lines of file F    the files in D
//   the folders in D    the size of file F    the last change of file F

#include <string.h>
#include "front/parse_internal.h"

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

static Expr *new_library(Parser *p, LibraryOp op, SourcePos start, Span op_span, Expr *a, Expr *b) {
    Expr *expr = ast_new_expr(p->arena, EXPR_LIBRARY, start);
    expr->as.library.op = op;
    expr->as.library.op_span = op_span;
    expr->as.library.a = a;
    expr->as.library.b = b;
    Expr *last = b ? b : a;
    expr->pos = ast_pos_join(start, last ? last->pos : (SourcePos){op_span, 0, 0, 0});
    return expr;
}

// The words, then a single value: "the square root of X".
static Expr *prefixed(Parser *p, LibraryOp op, size_t words) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op_span = parser_consume(p, words);
    if ((op == LIB_FILES_IN || op == LIB_FOLDERS_IN) && parser_at_word(p, 0, "folder") &&
        parser_starts_operand(parser_peek(p, 1))) {
        op_span = ast_pos_join((SourcePos){op_span, 0, 0, 0}, parser_token_pos(parser_advance(p))).span;  // "in the folder D"
    }
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, op_span, "Something is missing after \"%s\".", parser_text(p, op_span))) {
            diag_note(p->diag, "Write a value after it, like \"%s 9\".", parser_text(p, op_span));
        }
        return parser_error_expr(p);
    }
    Expr *operand = parser_at(p, 0, TOK_MINUS) ? parse_unary(p) : parse_primary(p);  // "the absolute value of -3"
    return new_library(p, op, start, op_span, operand, NULL);
}

// "WORDS A CONNECTOR B": the bigger of A and B, a random number from A to B, ...
static Expr *two_values(Parser *p, LibraryOp op, size_t words, const char *connector, const char *example) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op_span = parser_consume(p, words);
    Expr *a = parse_unary(p);
    if (p->sentence_failed) return parser_error_expr(p);
    static const char *const ands[] = {"and", NULL}, *const tos[] = {"to", NULL};
    if (!parser_expect_word(p, strcmp(connector, "and") == 0 ? ands : tos, example)) return parser_error_expr(p);
    Expr *b = parse_unary(p);
    return new_library(p, op, start, op_span, a, b);
}

static const struct {
    const char *words;
    DatePart part;
} date_parts[] = {
    {"year of", DATE_YEAR}, {"month of", DATE_MONTH}, {"day of", DATE_DAY},
    {"weekday of", DATE_WEEKDAY}, {"hour of", DATE_HOUR}, {"minute of", DATE_MINUTE},
};

// "file" or "folder" starts "file F exists" when a value follows it (not "of": "contents of file").
static bool at_exists(const Parser *p) {
    const Token *next = parser_peek(p, 1);
    return (parser_at_word(p, 0, "file") || parser_at_word(p, 0, "folder")) && parser_starts_operand(next) &&
           !(next->kind == TOK_WORD && strcmp(next->text, "of") == 0);
}

static Expr *parse_exists(Parser *p) {
    bool folder = parser_at_word(p, 0, "folder");
    SourcePos start = parser_token_pos(parser_advance(p));
    Expr *path = parse_primary(p);
    if (p->sentence_failed) return parser_error_expr(p);
    if (!parser_at_word(p, 0, "exists")) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected \"exists\" here, but found %s.", parser_describe(p, token))) {
            diag_note(p->diag, "Write it like \"if %s \"notes.txt\" exists:\".", folder ? "folder" : "file");
        }
        return parser_error_expr(p);
    }
    Span op_span = parser_advance(p)->span;
    Expr *expr = new_library(p, folder ? LIB_FOLDER_EXISTS : LIB_FILE_EXISTS, start, op_span, path, NULL);
    expr->pos = ast_pos_join(start, (SourcePos){op_span, 0, 0, 0});
    return expr;
}

Expr *parser_parse_library_word(Parser *p) {
    if (parser_is_function(p, parser_peek(p, 0)->text)) return NULL;  // the program's own "to bigger of ..."
    Expr *system = parser_parse_system_word(p);
    if (system) return system;
    static const struct {
        const char *words;
        LibraryOp op;
    } prefixes[] = {
        {"square root of", LIB_SQUARE_ROOT}, {"absolute value of", LIB_ABSOLUTE}, {"pick one of", LIB_PICK},
        {"random item of", LIB_PICK}, {"random item in", LIB_PICK}, {"lines of file", LIB_FILE_LINES},
        {"files in", LIB_FILES_IN}, {"folders in", LIB_FOLDERS_IN}, {"size of file", LIB_FILE_SIZE},
        {"last change of file", LIB_FILE_CHANGED},
    };
    for (size_t i = 0; i < COUNT(prefixes); i++) {
        size_t n = parser_match_words(p, prefixes[i].words);
        if (n) return prefixed(p, prefixes[i].op, n);
    }
    if (parser_match_words(p, "bigger of")) return two_values(p, LIB_BIGGER, 2, "and", "the bigger of 3 and 4");
    if (parser_match_words(p, "smaller of")) return two_values(p, LIB_SMALLER, 2, "and", "the smaller of 3 and 4");
    if (parser_match_words(p, "random number from")) {
        return two_values(p, LIB_RANDOM_WHOLE, 3, "to", "a random number from 1 to 6");
    }
    if (parser_match_words(p, "random decimal between")) {
        return two_values(p, LIB_RANDOM_DECIMAL, 3, "and", "a random decimal between 0 and 1");
    }
    if (parser_match_words(p, "number of days between")) {
        return two_values(p, LIB_DAYS_BETWEEN, 4, "and", "the number of days between today and holiday");
    }
    if (parser_match_words(p, "roll die") || parser_match_words(p, "time now")) {
        LibraryOp op = parser_at_word(p, 0, "roll") ? LIB_DIE : LIB_NOW;
        SourcePos start = parser_token_pos(parser_peek(p, 0));
        Span span = parser_consume(p, 2);
        return new_library(p, op, start, span, NULL, NULL);
    }
    if (at_exists(p)) return parse_exists(p);
    for (size_t i = 0; i < COUNT(date_parts); i++) {
        if (!parser_match_words(p, date_parts[i].words) || parser_is_function(p, parser_peek(p, 0)->text)) continue;
        Expr *expr = prefixed(p, LIB_DATE_PART, 2);
        if (expr->kind == EXPR_LIBRARY) expr->as.library.part = (int)date_parts[i].part;
        return expr;
    }
    return NULL;
}

// "rounded to 2 decimal places" (or "1 decimal place"), after "rounded to".
static Expr *rounded_to(Parser *p, Expr *expr, Span start) {
    Expr *places = parse_primary(p);
    if (p->sentence_failed) return parser_error_expr(p);
    size_t n = parser_match_words(p, "decimal places");
    if (!n) n = parser_match_words(p, "decimal place");
    if (!n) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected \"decimal places\" here, but found %s.", parser_describe(p, token))) {
            diag_note(p->diag, "Write it like \"price rounded to 2 decimal places\".");
        }
        return parser_error_expr(p);
    }
    Span end = parser_consume(p, n);
    Span op_span = {start.offset, end.offset + end.length - start.offset};
    Expr *result = new_library(p, LIB_ROUNDED_PLACES, expr->pos, op_span, expr, places);
    result->pos = ast_pos_join(expr->pos, (SourcePos){end, 0, 0, 0});
    return result;
}

Expr *parser_parse_library_postfix(Parser *p, Expr *expr) {
    static const struct {
        const char *words;
        LibraryOp op;
    } postfixes[] = {
        {"rounded up", LIB_ROUNDED_UP}, {"rounded down", LIB_ROUNDED_DOWN}, {"shuffled", LIB_SHUFFLED},
        {"days ago", LIB_DAYS_AGO}, {"day ago", LIB_DAYS_AGO}, {"days from now", LIB_DAYS_FROM_NOW},
        {"day from now", LIB_DAYS_FROM_NOW},
    };
    for (size_t i = 0; i < COUNT(postfixes); i++) {
        size_t n = parser_match_words(p, postfixes[i].words);
        if (!n) continue;
        Span op_span = parser_consume(p, n);
        Expr *result = new_library(p, postfixes[i].op, expr->pos, op_span, expr, NULL);
        result->pos = ast_pos_join(expr->pos, (SourcePos){op_span, 0, 0, 0});
        return result;
    }
    if (parser_match_words(p, "rounded to")) return rounded_to(p, expr, parser_consume(p, 2));
    if (parser_at_word(p, 0, "rounded")) {
        Span op_span = parser_advance(p)->span;
        Expr *result = new_library(p, LIB_ROUNDED, expr->pos, op_span, expr, NULL);
        result->pos = ast_pos_join(expr->pos, (SourcePos){op_span, 0, 0, 0});
        return result;
    }
    size_t n = parser_match_words(p, "to power of");
    if (n) {
        Span op_span = parser_consume(p, n);
        Expr *power = parser_at(p, 0, TOK_MINUS) ? parse_unary(p) : parse_primary(p);  // so a second one is seen
        if (!p->sentence_failed && parser_match_words(p, "to power of")) {
            Span again = parser_consume(p, 3);
            if (parser_error(p, again, "\"to the power of\" twice could mean two things.")) {
                diag_note(p->diag, "Use parentheses to say which: \"(2 to the power of 3) to the power of 2\" or "
                                   "\"2 to the power of (3 to the power of 2)\".");
            }
            return parser_error_expr(p);
        }
        return new_library(p, LIB_POWER, expr->pos, op_span, expr, power);
    }
    return NULL;
}

Expr *parser_parse_library_test(Parser *p, Expr *left) {
    Expr *system = parser_parse_system_test(p, left);
    if (system) return system;
    if (parser_match_words(p, "is before") || parser_match_words(p, "is after")) {
        LibraryOp op = parser_at_word(p, 1, "before") ? LIB_BEFORE : LIB_AFTER;
        Span op_span = parser_consume(p, 2);
        return new_library(p, op, left->pos, op_span, left, parse_join(p));
    }
    static const struct {
        const char *words;
        Period period;
    } periods[] = {
        {"is in this month", PERIOD_THIS_MONTH}, {"is in last month", PERIOD_LAST_MONTH},
        {"is in next month", PERIOD_NEXT_MONTH}, {"is in this year", PERIOD_THIS_YEAR},
        {"is in last year", PERIOD_LAST_YEAR}, {"is in next year", PERIOD_NEXT_YEAR},
    };
    for (size_t i = 0; i < COUNT(periods); i++) {
        size_t n = parser_match_words(p, periods[i].words);
        if (!n) continue;
        Span op_span = parser_consume(p, n);
        Expr *result = new_library(p, LIB_IN_PERIOD, left->pos, op_span, left, NULL);
        result->as.library.part = (int)periods[i].period;
        result->pos = ast_pos_join(left->pos, (SourcePos){op_span, 0, 0, 0});
        return result;
    }
    return NULL;
}
