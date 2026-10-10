/* EasyScript runtime, part 2: what values can do (text, numbers, comparisons,
 * loops, functions, files, input). Part 1, es_memory.h, comes first.
 *
 * The compiler has already checked every kind, so nothing here checks kinds.
 * What's left are the mistakes only the values can show: dividing by zero,
 * counting by a step that isn't more than zero, text that isn't a number,
 * files that can't be read or written, and runaway recursion. */
#ifndef ES_RUNTIME_H
#define ES_RUNTIME_H

#ifndef ES_MEMORY_H
#include "es_memory.h"
#endif

#include <limits.h>

/* --- Text ----------------------------------------------------------------- */

static EsText es_lit_nan = ES_STATIC_TEXT("not a number");
static EsText es_lit_infinity = ES_STATIC_TEXT("infinity");
static EsText es_lit_minus_infinity = ES_STATIC_TEXT("-infinity");
static EsText es_lit_yes = ES_STATIC_TEXT("yes");
static EsText es_lit_no = ES_STATIC_TEXT("no");

/* Whole numbers print without decimals; others with up to 15 significant digits. */
static EsText *es_number_text(double x) {
    if (isnan(x)) return &es_lit_nan;
    if (isinf(x)) return x > 0 ? &es_lit_infinity : &es_lit_minus_infinity;
    if (x == 0) x = 0; /* no "-0" */
    const char *fmt = (x == floor(x) && fabs(x) < 1e15) ? "%.0f" : "%.15g";
    int n = snprintf(NULL, 0, fmt, x);
    char *p;
    EsText *t = es_text_new((size_t)n, &p);
    snprintf(p, (size_t)n + 1, fmt, x);
    return t;
}

static EsText *es_yesno_text(bool yes) {
    return yes ? &es_lit_yes : &es_lit_no;
}

/* "followed by", and "and" with text: joins two texts (consuming both).
 * Joining onto empty text gives the other side back without copying. */
static EsText *es_join(EsText *a, EsText *b) {
    if (a->len == 0) {
        es_release(a);
        return b;
    }
    if (b->len == 0) {
        es_release(b);
        return a;
    }
    char *p;
    EsText *joined = es_text_new(a->len + b->len, &p);
    memcpy(p, a->chars, a->len);
    memcpy(p + a->len, b->chars, b->len);
    es_release(a);
    es_release(b);
    return joined;
}

static void es_say(EsText *t) {
    es_out(t->chars, t->len);
    es_out("\n", 1);
    es_release(t);
}

/* Length in characters (UTF-8 aware). */
static double es_length(EsText *t) {
    size_t chars = 0;
    for (size_t i = 0; i < t->len; i++) {
        if (((unsigned char)t->chars[i] & 0xC0) != 0x80) chars++;
    }
    es_release(t);
    return (double)chars;
}

/* --- Numbers ---------------------------------------------------------------- */

static inline double es_div(int line, double a, double b) {
    if (b == 0) es_fail(line, NULL, "You divided by zero.");
    return a / b;
}

/* Whole numbers (the usual case) use the integer remainder, which gives the
 * same answer as fmod, much faster. */
static inline double es_mod(int line, double a, double b) {
    if (b == 0) es_fail(line, NULL, "You divided by zero.");
    if (fabs(a) < 9e15 && fabs(b) < 9e15 && a == (double)(long long)a && b == (double)(long long)b) {
        return (double)((long long)a % (long long)b);
    }
    return fmod(a, b);
}

static bool es_is_digit(char c) {
    return c >= '0' && c <= '9';
}

/* Spaces around it, an optional "-", digits, and optionally "." and digits. */
static bool es_parse_number(const char *s, size_t len, double *out) {
    size_t i = 0, end = len;
    while (i < end && s[i] == ' ') i++;
    while (end > i && s[end - 1] == ' ') end--;
    size_t start = i;
    if (i < end && s[i] == '-') i++;
    size_t digits = i;
    while (i < end && es_is_digit(s[i])) i++;
    if (i == digits) return false;
    if (i < end && s[i] == '.') {
        size_t frac = ++i;
        while (i < end && es_is_digit(s[i])) i++;
        if (i == frac) return false;
    }
    if (i != end) return false;
    *out = strtod(s + start, NULL);
    return true;
}

/* "as a number" on text (consuming it). */
static double es_text_number(int line, EsText *t) {
    double x;
    if (!es_parse_number(t->chars, t->len, &x)) {
        es_fail(line, NULL, "I can't turn \"%.*s\" into a number.", (int)(t->len > 60 ? 60 : t->len), t->chars);
    }
    es_release(t);
    return x;
}

/* --- Comparisons ---------------------------------------------------------- */

/* Numbers within a relative 1e-12 of each other count as equal, so
 * 0.1 plus 0.2 is 0.3. */
static inline bool es_close(double a, double b) {
    return a == b || fabs(a - b) <= 1e-12 * fmax(fabs(a), fabs(b));
}

/* -1, 0 or 1; numbers that count as equal give 0, so "is at most" agrees with "is". */
static inline int es_number_order(double a, double b) {
    if (es_close(a, b)) return 0;
    return a < b ? -1 : 1;
}

/* Texts in alphabetical order (by bytes): -1, 0 or 1, consuming both. */
static int es_text_order(EsText *a, EsText *b) {
    size_t n = a->len < b->len ? a->len : b->len;
    int c = memcmp(a->chars, b->chars, n);
    int order = c != 0 ? (c < 0 ? -1 : 1) : (a->len > b->len) - (a->len < b->len);
    es_release(a);
    es_release(b);
    return order;
}

static bool es_text_same(EsText *a, EsText *b) {
    bool same = a->len == b->len && memcmp(a->chars, b->chars, a->len) == 0;
    es_release(a);
    es_release(b);
    return same;
}

/* --- Functions ------------------------------------------------------------- */

/* Every function call goes one level deeper; endless recursion stops with a
 * friendly error instead of crashing. */
#define ES_MAX_DEPTH 10000
static int es_depth;

static inline void es_enter(int line) {
    if (++es_depth > ES_MAX_DEPTH) {
        es_fail(line, "Check that the function stops calling itself at some point.",
                "Functions are calling each other too deeply (more than %d calls inside each other).", ES_MAX_DEPTH);
    }
}

static inline void es_leave(void) {
    es_depth--;
}

/* A function that gives back a value reached its end without "give back". */
static _Noreturn void es_no_result(int line, const char *name) {
    es_fail(line, "Make sure every way through it ends with \"give back ...\".",
            "The function \"%s\" ended without giving anything back.", name);
}

/* --- Loops ----------------------------------------------------------------- */

/* "count from A to B by S": counts toward B, up or down, by S each round.
 * "count down" never counts up (it runs zero times if A is below B).
 *
 * Round i's number is A + direction * i * S, worked out from the start each
 * time (no drifting). The count goes on while that number isn't past B, and
 * a number equal to B (see es_close) lands exactly on it. The last round,
 * and the first one whose number is equal to B, are worked out once, at the
 * start, so each round is cheap. */
typedef struct {
    double from, to, step;
    int direction;      /* 1 or -1 */
    long long last;     /* the last round's index; -1 if there are none */
    long long at_end;   /* rounds from here to last are equal to B, so they're B */
} EsCount;

static inline double es_count_raw(const EsCount *c, long long i) {
    return c->from + c->direction * (double)i * c->step;
}

/* Whether round i is still in the count: not past `to`, or equal to it. */
static bool es_count_in(const EsCount *c, long long i, double to) {
    double v = es_count_raw(c, i);
    return es_close(v, to) || (c->direction > 0 ? v <= to : v >= to);
}

static EsCount es_count_start(int line, double from, double to, double step, bool has_step, bool down) {
    EsCount c = {from, to, 1, 1, -1, LLONG_MAX};
    if (has_step) {
        if (!(step > 0)) {
            es_fail(line, "Counting goes up or down by itself; the step only says how far to go each time.",
                    "The step of a count has to be more than zero, but it's %s.", es_number_text(step)->chars);
        }
        c.step = step;
    }
    if (down) {
        c.direction = -1;
        if (from < to && !es_close(from, to)) return c; /* empty */
    } else {
        c.direction = from <= to || es_close(from, to) ? 1 : -1;
    }
    double rounds = floor((to - from) * c.direction / c.step);
    if (!(rounds < 9e18)) { /* endless in practice (or not a number): count until stopped */
        c.last = LLONG_MAX;
        return c;
    }
    long long last = rounds < 0 ? 0 : (long long)rounds;
    while (es_count_in(&c, last + 1, to)) last++; /* rounding can put the end one step either way */
    while (last >= 0 && !es_count_in(&c, last, to)) last--;
    c.last = last;
    /* Numbers equal to B are a run at the end (almost always just the last
     * one): start from an estimate of where it begins, then adjust. */
    double near = (fabs(to - from) - 1e-12 * fmax(fabs(from), fabs(to))) / c.step;
    long long at_end = near <= 0 ? 0 : near >= (double)last ? last : (long long)near;
    while (at_end > 0 && es_close(es_count_raw(&c, at_end - 1), to)) at_end--;
    while (at_end <= last && !es_close(es_count_raw(&c, at_end), to)) at_end++;
    c.at_end = at_end;
    return c;
}

/* Round i's number (i from 0 to c->last). */
static inline double es_count_value(const EsCount *c, long long i) {
    return i >= c->at_end ? c->to : es_count_raw(c, i);
}

/* "do this N times": N must be a whole number, zero or more. */
static long long es_times(int line, double n) {
    if (n < 0) es_fail(line, NULL, "The number of times can't be negative, but it's %s.", es_number_text(n)->chars);
    if (n != floor(n)) {
        es_fail(line, NULL, "The number of times has to be a whole number, but it's %s.", es_number_text(n)->chars);
    }
    if (n > 9e18) es_fail(line, NULL, "That's too many times to repeat.");
    return (long long)n;
}

/* --- Files and input ------------------------------------------------------ */

/* The whole file as new text, without its final new line. Consumes path. */
static EsText *es_read_file(int line, EsText *path) {
    FILE *f = fopen(path->chars, "rb");
    if (!f) es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", path->chars, es_reason(errno));
    if (fseek(f, 0, SEEK_END) != 0) es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", path->chars, es_reason(errno));
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
        es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", path->chars, es_reason(errno));
    }
    char *p = malloc((size_t)size + 1);
    if (!p) es_out_of_memory();
    size_t got = fread(p, 1, (size_t)size, f);
    fclose(f);
    if (got > 0 && p[got - 1] == '\n') got--;
    if (got > 0 && p[got - 1] == '\r') got--;
    EsText *text = es_text_copy(p, got);
    free(p);
    es_release(path);
    return text;
}

/* Writes the text and a new line, replacing or adding to the file. Consumes both. */
static void es_write_file(int line, EsText *text, EsText *path, bool append) {
    FILE *f = fopen(path->chars, append ? "ab" : "wb");
    if (!f) {
        es_fail(line, NULL, "I couldn't write to the file \"%s\": %s.", path->chars,
                errno == ENOENT ? "the folder it should go in doesn't exist" : es_reason(errno));
    }
    fwrite(text->chars, 1, text->len, f);
    fputc('\n', f);
    if (fclose(f) != 0) es_fail(line, NULL, "I couldn't write to the file \"%s\": %s.", path->chars, es_reason(errno));
    es_release(text);
    es_release(path);
}

/* Reads one line: a replayed answer (in the shell) if any are left,
 * otherwise from the keyboard, remembering it for the shell's next run. */
static ssize_t es_read_answer(char **buffer, size_t *cap) {
    if (es_answers && !es_answers_used_up) {
        ssize_t n = getline(buffer, cap, es_answers);
        if (n > 0) return n;
        es_answers_used_up = true;
    }
    ssize_t n = getline(buffer, cap, stdin);
    if (es_answers) {
        fseek(es_answers, 0, SEEK_END);
        if (n > 0) fwrite(*buffer, 1, (size_t)n, es_answers);
        if (n <= 0 || (*buffer)[n - 1] != '\n') fputc('\n', es_answers);
        fflush(es_answers);
    }
    return n;
}

/* Prints the question (consuming it), then reads one line as new text. At
 * the end of input the answer is empty text. */
static EsText *es_ask(EsText *question) {
    es_out(question->chars, question->len);
    es_release(question);
    fflush(stdout);
    char *buffer = NULL;
    size_t cap = 0;
    ssize_t n = es_read_answer(&buffer, &cap);
    size_t len = n > 0 ? (size_t)n : 0;
    if (len > 0 && buffer[len - 1] == '\n') len--;
    if (len > 0 && buffer[len - 1] == '\r') len--;
    EsText *answer = len > 0 ? es_text_copy(buffer, len) : &es_empty_text;
    free(buffer);
    return answer;
}

/* "stop the program" inside a function, once the function has released its
 * own names (in main() it jumps to the end instead). See es_finish. */
static _Noreturn void es_stop(void) {
    es_finish(false);
    exit(0);
}

#endif
