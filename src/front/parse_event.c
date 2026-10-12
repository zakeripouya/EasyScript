// Events, at the top level. They run once the program's other sentences have:
//
//   every 2 seconds: / every second: / every 100 milliseconds:   (also minutes and hours)
//   every day at 9pm: / every Monday at 9:30am: / every Friday at 17:00:
//   whenever C: / whenever C, S          each time C becomes yes
//   when C: / when C, S                  the first time C is yes
//   whenever a key is pressed:           "it" is the key ("left", "enter", "a", ...)

#include <stdlib.h>
#include <string.h>
#include "front/parse_internal.h"

static const char *const weekdays[] = {"sunday", "monday", "tuesday", "wednesday", "thursday", "friday", "saturday"};

static int weekday_at(const Parser *p, size_t ahead) {
    for (int i = 0; i < 7; i++) {
        if (parser_at_word(p, ahead, weekdays[i])) return i;
    }
    return -1;
}

static const struct {
    const char *word;
    double seconds;
} units[] = {
    {"seconds", 1},     {"second", 1},          {"minutes", 60},       {"minute", 60},
    {"hours", 3600},    {"hour", 3600},         {"milliseconds", 0.001}, {"millisecond", 0.001},
};

static int unit_at(const Parser *p, size_t ahead) {
    for (size_t i = 0; i < sizeof units / sizeof units[0]; i++) {
        if (parser_at_word(p, ahead, units[i].word)) return (int)i;
    }
    return -1;
}

bool parser_at_event(const Parser *p) {
    if (parser_at_word(p, 0, "whenever") || parser_at_word(p, 0, "when")) return true;
    if (!parser_at_word(p, 0, "every")) return false;
    if (parser_at_word(p, 1, "day") || weekday_at(p, 1) >= 0) return parser_at_word(p, 2, "at");
    if (unit_at(p, 1) >= 0) return true;  // every second:
    size_t n = parser_value_length(p, 1);
    return n > 0 && unit_at(p, 1 + n) >= 0;
}

// The time of day after "at": 9am, 9:30pm, 12am (midnight), 17:00; in seconds after midnight.
static bool parse_time(Parser *p, double *seconds) {
    static const char *const example = "every Monday at 9am:";
    const Token *hour = parser_peek(p, 0);
    double h = hour->kind == TOK_NUMBER ? strtod(hour->text, NULL) : -1, m = 0;
    size_t n = 1;
    bool minutes = parser_at(p, 1, TOK_COLON) && parser_at(p, 2, TOK_NUMBER);
    if (minutes) {
        m = strtod(parser_peek(p, 2)->text, NULL);
        n = 3;
    }
    bool am = parser_at_word(p, n, "am"), pm = parser_at_word(p, n, "pm");
    bool whole = h == (int)h && m == (int)m;
    bool ok = h >= 0 && whole && m >= 0 && m < 60 && ((am || pm) ? h >= 1 && h <= 12 : minutes && h < 24);
    if (!ok) {
        Span span = hour->span;
        if (parser_error(p, span, "I expected a time of day here, like \"9am\", \"9:30pm\" or \"17:00\".")) {
            diag_note(p->diag, "Write it like \"%s\". Hours go from 1 to 12 with am or pm, or from 0 to 23 with "
                               "minutes (17:00).", example);
            diag_info(p->diag, DIAG_MISSING, "a time of day", NULL);
        }
        return false;
    }
    if (am && h == 12) h = 0;
    if (pm && h != 12) h += 12;
    parser_consume(p, n + (am || pm));
    *seconds = h * 3600 + m * 60;
    return true;
}

// ":" and a block, or ", S" (one sentence, for "whenever" and "when").
static bool parse_body(Parser *p, Stmt *stmt, bool one_line) {
    if (one_line && parser_at(p, 0, TOK_COMMA)) {
        Span comma = parser_advance(p)->span;
        if (parser_at_sentence_end(p)) {
            if (parser_error(p, comma, "Something is missing after \",\".")) {
                diag_note(p->diag, "Say what to do, like \"whenever score is over 100, say \"Level up!\"\".");
                diag_info(p->diag, DIAG_MISSING, "what to do", NULL);
            }
            return false;
        }
        Stmt *first = parser_parse_actions(p, &stmt->as.event.body);
        return first && !p->sentence_failed;
    }
    SourcePos header = stmt->pos;
    bool ok = parser_parse_block(p, &stmt->as.event.body, &header, NULL, "this event");
    stmt->pos = header;
    p->ended_with_block = true;
    return ok;
}

// After a mistake in the header: skip it and its block (which ends the sentence).
static Stmt *skip(Parser *p) {
    parser_skip_line_and_block(p);
    p->ended_with_block = true;
    return NULL;
}

Stmt *parser_parse_event(Parser *p) {
    const Token *verb = parser_advance(p);
    Stmt *stmt = parser_new_stmt(p, STMT_EVENT, verb);
    bool every = strcmp(verb->text, "every") == 0;
    if (every && (parser_at_word(p, 0, "day") || weekday_at(p, 0) >= 0)) {  // every Monday at 9am
        stmt->as.event.kind = EVENT_AT;
        stmt->as.event.weekday = weekday_at(p, 0);
        parser_consume(p, 2);  // the day, "at"
        if (!parse_time(p, &stmt->as.event.time)) {
            return skip(p);
        }
    } else if (every) {  // every 2 seconds
        stmt->as.event.kind = EVENT_EVERY;
        if (unit_at(p, 0) < 0) stmt->as.event.every = parse_unary(p);
        int unit = unit_at(p, 0);
        if (p->sentence_failed || unit < 0) {
            return skip(p);
        }
        stmt->as.event.scale = units[unit].seconds;
        parser_advance(p);
    } else if (parser_match_words(p, "key is pressed")) {  // whenever a key is pressed
        if (strcmp(verb->text, "whenever") != 0) {
            if (parser_error(p, verb->span, "\"when a key is pressed\" happens more than once, so it's \"whenever\".")) {
                diag_note(p->diag, "Write it like \"whenever a key is pressed:\".");
                diag_info(p->diag, DIAG_NOT_ALLOWED, "\"when\" with keys", NULL);
            }
            return skip(p);
        }
        stmt->as.event.kind = EVENT_KEY;
        parser_consume(p, 3);
    } else {  // whenever C / when C
        stmt->as.event.kind = strcmp(verb->text, "when") == 0 ? EVENT_WHEN : EVENT_WHENEVER;
        if (parser_at_sentence_end(p) || parser_at(p, 0, TOK_COLON)) {
            if (parser_error(p, verb->span, "Something is missing after \"%s\".", verb->text)) {
                diag_note(p->diag, "Say when, like \"whenever score is over 100:\".");
                diag_info(p->diag, DIAG_MISSING, "a condition", "yes or no");
            }
            return skip(p);
        }
        stmt->as.event.condition = parse_expression(p);
        if (p->sentence_failed) {
            return skip(p);
        }
    }
    bool one_line = stmt->as.event.kind == EVENT_WHENEVER || stmt->as.event.kind == EVENT_WHEN;
    return parse_body(p, stmt, one_line) ? stmt : NULL;
}
