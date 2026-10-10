// Loops, and the sentences that leave or skip a round of one.
//
//   count [down] from A to B [by S | in steps of S] [and call each number N | as N]:
//   go from A to B [by S | in steps of S] [and call each number N | as N]:
//   for each N from A to B [by S | in steps of S]:
//   do this N times:  /  repeat N times:
//   while C:  /  as long as C:  /  repeat while C:
//   keep doing this until C:  /  repeat until C:
//   forever:
//
//   stop the loop / stop / break          skip this one / skip / continue / move on
//   stop the program                      (ends the whole program)
//
// In a count header "as" names the loop variable instead of starting a
// conversion, so "as number" is the variable "number"; use parentheses to
// convert a bound: count from 1 to (limit as a number).

#include <string.h>
#include "front/parse_internal.h"

static Stmt *new_loop(Parser *p, LoopKind kind, const Token *verb) {
    Stmt *stmt = parser_new_stmt(p, STMT_LOOP, verb);
    stmt->as.loop.kind = kind;
    stmt->as.loop.var.text = "number";
    stmt->as.loop.var.pos = parser_token_pos(verb);
    return stmt;
}

// A value in a count header, where "as" ends it.
static Expr *count_value(Parser *p) {
    bool saved = p->as_ends_value;
    p->as_ends_value = true;
    Expr *expr = parse_expression(p);
    p->as_ends_value = saved;
    return expr;
}

// A to B [by S | in steps of S], after "from".
static bool parse_range(Parser *p, Stmt *stmt, const char *example) {
    static const char *const to[] = {"to", NULL};
    stmt->as.loop.from = count_value(p);
    if (p->sentence_failed || !parser_expect_word(p, to, example)) return false;
    stmt->as.loop.to = count_value(p);
    if (p->sentence_failed) return false;
    if (parser_at_word(p, 0, "by")) {
        parser_advance(p);
        stmt->as.loop.step = count_value(p);
    } else if (parser_match_words(p, "in steps of")) {
        parser_consume(p, 3);
        stmt->as.loop.step = count_value(p);
    }
    return !p->sentence_failed;
}

// [and call each number N | as N]
static bool parse_loop_name(Parser *p, Stmt *stmt) {
    if (parser_match_words(p, "and call each number")) {
        parser_consume(p, 4);
    } else if (parser_at_word(p, 0, "as")) {
        parser_advance(p);
    } else {
        return true;
    }
    stmt->as.loop.var_named = true;
    return parser_parse_name(p, &stmt->as.loop.var, NULL);
}

static bool report_count_as_variable(Parser *p) {
    if (parser_error(p, p->prev_span, "A sentence that starts with \"count\" is a counting loop, like "
                                      "\"count from 1 to 10:\".")) {
        diag_note(p->diag, "To make a variable called \"count\", write \"let count be ...\".");
    }
    return false;
}

// count [down] from ... / go from ...
static bool parse_count(Parser *p, Stmt *stmt, const Token *verb) {
    static const char *const from[] = {"from", NULL};
    bool is_count = strcmp(verb->text, "count") == 0;
    const char *example = is_count ? "count from 1 to 10:" : "go from 0 to 100 in steps of 10:";
    if (is_count && parser_looks_like_assignment(parser_peek(p, 0))) return report_count_as_variable(p);
    if (is_count && parser_at_word(p, 0, "down")) {
        parser_advance(p);
        stmt->as.loop.down = true;
        example = "count down from 10 to 1:";
    }
    return parser_expect_word(p, from, example) && parse_range(p, stmt, example) && parse_loop_name(p, stmt);
}

// for each N from A to B [by S]
static bool parse_for_each(Parser *p, Stmt *stmt) {
    static const char *const each[] = {"each", NULL};
    static const char *const from[] = {"from", NULL};
    static const char *const example = "for each n from 1 to 10:";
    stmt->as.loop.var_named = true;
    return parser_expect_word(p, each, example) && parser_parse_name(p, &stmt->as.loop.var, from) &&
           parser_expect_word(p, from, example) && parse_range(p, stmt, example);
}

// N times (after "do this" or "repeat")
static bool parse_times(Parser *p, Stmt *stmt, const char *example) {
    static const char *const times[] = {"times", NULL};
    stmt->as.loop.times = parse_expression(p);
    return !p->sentence_failed && parser_expect_word(p, times, example);
}

static bool parse_condition(Parser *p, Stmt *stmt) {
    stmt->as.loop.condition = parse_expression(p);
    return !p->sentence_failed;
}

// Everything between the first word and the colon.
static bool parse_header(Parser *p, Stmt *stmt, const Token *verb) {
    const char *word = verb->text;
    if (strcmp(word, "count") == 0 || strcmp(word, "go") == 0) return parse_count(p, stmt, verb);
    if (strcmp(word, "for") == 0) return parse_for_each(p, stmt);
    if (strcmp(word, "do") == 0) {
        static const char *const this_word[] = {"this", NULL};
        stmt->as.loop.kind = LOOP_TIMES;
        return parser_expect_word(p, this_word, "do this 3 times:") && parse_times(p, stmt, "do this 3 times:");
    }
    if (strcmp(word, "repeat") == 0) {
        if (parser_at_word(p, 0, "until") || parser_at_word(p, 0, "while")) {
            stmt->as.loop.kind = parser_at_word(p, 0, "until") ? LOOP_UNTIL : LOOP_WHILE;
            parser_advance(p);
            return parse_condition(p, stmt);
        }
        stmt->as.loop.kind = LOOP_TIMES;
        return parse_times(p, stmt, "repeat 3 times:");
    }
    if (strcmp(word, "while") == 0) {
        stmt->as.loop.kind = LOOP_WHILE;
        return parse_condition(p, stmt);
    }
    if (strcmp(word, "as") == 0) {
        stmt->as.loop.kind = LOOP_WHILE;
        return parser_expect_phrase(p, "long as", "as long as", "as long as x is less than 10:") &&
               parse_condition(p, stmt);
    }
    if (strcmp(word, "keep") == 0) {
        stmt->as.loop.kind = LOOP_UNTIL;
        return parser_expect_phrase(p, "doing this until", "keep doing this until", "keep doing this until done:") &&
               parse_condition(p, stmt);
    }
    stmt->as.loop.kind = LOOP_FOREVER;  // "forever"
    return true;
}

Stmt *parse_loop(Parser *p, const Token *verb) {
    Stmt *stmt = new_loop(p, LOOP_COUNT, verb);
    bool ok = parse_header(p, stmt, verb);
    if (!ok) {
        parser_skip_line_and_block(p);
    } else {
        SourcePos header = stmt->pos;
        ok = parser_parse_block(p, &stmt->as.loop.body, &header, NULL, "this loop");
    }
    p->ended_with_block = true;  // set last: statements inside the block reset it
    return ok ? stmt : NULL;
}

// --- stop / skip -------------------------------------------------------------

Stmt *parse_loop_control(Parser *p, const Token *verb) {
    static const char *const on[] = {"on", NULL};
    const char *word = verb->text;
    if (strcmp(word, "stop") == 0) {
        if (parser_match_words(p, "program")) {
            parser_advance(p);
            return parser_new_stmt(p, STMT_STOP, verb);
        }
        if (parser_at_word(p, 0, "loop")) parser_advance(p);
        return parser_new_stmt(p, STMT_BREAK, verb);
    }
    if (strcmp(word, "break") == 0) return parser_new_stmt(p, STMT_BREAK, verb);
    if (strcmp(word, "skip") == 0 && parser_match_words(p, "this one")) parser_consume(p, 2);
    if (strcmp(word, "move") == 0 && !parser_expect_word(p, on, "move on")) return NULL;
    return parser_new_stmt(p, STMT_CONTINUE, verb);  // skip, continue, move on
}
