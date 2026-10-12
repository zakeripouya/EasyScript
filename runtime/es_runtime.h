/* EasyScript runtime, part 3: what values can do (text, numbers, comparisons,
 * loops, functions, files, input). Parts 1 and 2 come first.
 *
 * The compiler has already checked every kind, so nothing here checks kinds.
 * What's left are the mistakes only the values can show: dividing by zero,
 * counting by a step that isn't more than zero, text that isn't a number,
 * files that can't be read or written, and runaway recursion. */
#ifndef ES_RUNTIME_H
#define ES_RUNTIME_H

#ifndef ES_PROGRAM_H
#include "es_program.h"
#endif

#include <limits.h>

/* --- Text ----------------------------------------------------------------- */

static EsText es_lit_nan = ES_STATIC_TEXT("not a number");
static EsText es_lit_infinity = ES_STATIC_TEXT("infinity");
static EsText es_lit_minus_infinity = ES_STATIC_TEXT("-infinity");
static EsText es_lit_yes = ES_STATIC_TEXT("yes");
static EsText es_lit_no = ES_STATIC_TEXT("no");

/* Whole numbers up to 2^53 (where a double still holds every whole number)
 * print exactly, without decimals or an exponent. */
#define ES_EXACT_WHOLE 9007199254740992.0

/* Whole numbers print without decimals; others with at most 10 significant
 * digits, so tiny rounding errors don't show (0.1 plus 0.2 prints 0.3). A
 * number that rounds to a whole number prints as one (0.99999999999 is 1).
 * consteval.c's const_number_text does exactly the same. */
static EsText *es_number_text(double x) {
    if (isnan(x)) return &es_lit_nan;
    if (isinf(x)) return x > 0 ? &es_lit_infinity : &es_lit_minus_infinity;
    char digits[32]; /* "%.10g" takes at most 17 characters, "%.0f" up to 2^53 at most 17 */
    if (x != floor(x) || fabs(x) > ES_EXACT_WHOLE) {
        snprintf(digits, sizeof digits, "%.10g", x);
        x = strtod(digits, NULL);
    }
    if (x == 0) x = 0; /* no "-0" */
    if (x == floor(x) && fabs(x) <= ES_EXACT_WHOLE) snprintf(digits, sizeof digits, "%.0f", x);
    return es_text_copy(digits, strlen(digits));
}

/* "as exact text": the shortest digits that read back as exactly the same
 * number (whole numbers up to 2^53 as they are), with an exponent as
 * "1e20" or "1e-7" when printf picks one. consteval.c's const_exact_text
 * does exactly the same. */
static EsText *es_number_exact_text(double x) {
    if (isnan(x)) return &es_lit_nan;
    if (isinf(x)) return x > 0 ? &es_lit_infinity : &es_lit_minus_infinity;
    if (x == 0) x = 0; /* no "-0" */
    char digits[40]; /* "%.17g" takes at most 24 characters */
    if (x == floor(x) && fabs(x) <= ES_EXACT_WHOLE) {
        snprintf(digits, sizeof digits, "%.0f", x);
    } else {
        for (int precision = 15; precision <= 17; precision++) {
            snprintf(digits, sizeof digits, "%.*g", precision, x);
            if (strtod(digits, NULL) == x) break;
        }
        char *e = strchr(digits, 'e');
        if (e) { /* "1e+20" is "1e20", "1e-07" is "1e-7" */
            char *from = e + 1, *to = e + 1;
            if (*from == '+') from++;
            else if (*from == '-') *to++ = *from++;
            while (*from == '0' && from[1]) from++;
            memmove(to, from, strlen(from) + 1);
        }
    }
    return es_text_copy(digits, strlen(digits));
}

/* A number as text for an error message. The program stops right after,
 * so the text is never released; every one made is kept in a static list,
 * so it stays reachable and leak checkers (LeakSanitizer on Linux checks
 * error programs too) don't report it, even when one message shows two. */
static EsText **es_error_numbers;
static size_t es_error_number_count;

static const char *es_shown(double x) {
    EsText **kept = realloc(es_error_numbers, (es_error_number_count + 1) * sizeof *kept);
    if (!kept) es_out_of_memory();
    es_error_numbers = kept;
    EsText *text = es_number_text(x);
    es_error_numbers[es_error_number_count++] = text;
    return text->chars;
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

/* No space goes after a piece ending in one of these (or a space or new line). */
static bool es_opens(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '(' || c == '[' || c == '$';
}

/* No space goes before a piece starting with one of these (or a space or new line). */
static bool es_closes(char c) {
    return c == ' ' || c == '\n' || c == '\t' || c == '.' || c == ',' || c == '!' || c == '?' || c == ';' ||
           c == ':' || c == ')' || c == ']' || c == '%';
}

/* "say A, B": the two pieces with a space between them, unless either is
 * empty, A ends with a space, a new line, "(", "[" or "$", or B starts with a
 * space, a new line or ". , ! ? ; : ) ] %". Consumes both. */
static EsText *es_join_spaced(EsText *a, EsText *b) {
    if (a->len == 0 || b->len == 0 || es_opens(a->chars[a->len - 1]) || es_closes(b->chars[0])) return es_join(a, b);
    char *p;
    EsText *joined = es_text_new(a->len + 1 + b->len, &p);
    memcpy(p, a->chars, a->len);
    p[a->len] = ' ';
    memcpy(p + a->len + 1, b->chars, b->len);
    es_release(a);
    es_release(b);
    return joined;
}

/* "set T to T followed by X": adds b to the text in *slot. Text that only
 * this variable holds grows in place (doubling its room), so building text
 * piece by piece takes time in proportion to its length; shared text is
 * copied as es_join does. Consumes b. */
static void es_append(EsText **slot, EsText *b) {
    EsText *a = *slot;
    if (a->header.refs != 1) {
        es_set(slot, es_join(es_retain(a), b));
        return;
    }
    size_t len = a->len + b->len;
    if (len > a->cap) {
        size_t cap = a->cap * 2 > len ? a->cap * 2 : len;
        if (cap < 16) cap = 16;
        if (cap > SIZE_MAX - sizeof(EsText) - 1) es_out_of_memory();
        es_memory_take(cap - a->cap);
        EsText *grown = (EsText *)realloc(a, sizeof(EsText) + cap + 1);
        if (!grown) es_out_of_memory();
        grown->chars = (const char *)(grown + 1);
        grown->cap = cap;
        *slot = a = grown;
    }
    char *chars = (char *)(a + 1);
    memcpy(chars + a->len, b->chars, b->len);
    chars[len] = '\0';
    a->count = a->count > 0 && b->count > 0 ? a->count + b->count : 0; /* 0: count again when needed */
    a->len = len;
    es_release(b);
}

static void es_say(EsText *t) {
    es_out(t->chars, t->len);
    es_out("\n", 1);
    es_release(t);
}

/* Length in characters: a character is a Unicode code point, stored as
 * UTF-8 (one to four bytes), so it's every byte that doesn't continue the
 * one before. Counted once and kept in the text (text never changes, except
 * while es_append grows text nobody else holds); text whose count equals its
 * length is plain ASCII, so its characters are its bytes. */
static size_t es_text_chars(EsText *t) {
    if (t->count == 0 && t->len > 0) {
        size_t chars = 0;
        for (size_t i = 0; i < t->len; i++) chars += !es_continuation_byte(t->chars[i]);
        t->count = chars;
    }
    return t->count;
}

static double es_length(EsText *t) {
    size_t chars = es_text_chars(t);
    es_release(t);
    return (double)chars;
}

static bool es_text_is_empty(EsText *t) {
    bool empty = t->len == 0;
    es_release(t);
    return empty;
}

/* --- Numbers ---------------------------------------------------------------- */

static inline double es_div(int site, double a, double b) {
    if (b == 0) es_fail(site, NULL, "You divided by zero.");
    return a / b;
}

/* Whole numbers (the usual case) use the integer remainder, which gives the
 * same answer as fmod, much faster. */
static inline double es_mod(int site, double a, double b) {
    if (b == 0) es_fail(site, NULL, "You divided by zero.");
    if (fabs(a) < 9e15 && fabs(b) < 9e15 && a == (double)(long long)a && b == (double)(long long)b) {
        return (double)((long long)a % (long long)b);
    }
    return fmod(a, b);
}

static bool es_is_digit(char c) {
    return c >= '0' && c <= '9';
}

/* Spaces around it, an optional "-", digits, optionally "." and digits, and
 * optionally "e" (or "E"), a sign and digits for a power of ten (1e5). */
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
    if (i < end && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        if (i < end && (s[i] == '-' || s[i] == '+')) i++;
        size_t power = i;
        while (i < end && es_is_digit(s[i])) i++;
        if (i == power) return false;
    }
    if (i != end) return false;
    *out = strtod(s + start, NULL);
    return true;
}

/* "as a number" on text (consuming it). The text is shown up to its 60th
 * character. */
static double es_text_number(int site, EsText *t) {
    double x;
    if (es_parse_number(t->chars, t->len, &x)) {
        es_release(t);
        return x;
    }
    static const char *const hint = "A number is digits, with a \"-\" before them if it's negative, a \".\" for "
                                    "decimals and an \"e\" for powers of ten, like 42, -7, 3.5 or 1e5.";
    if (t->len == 0) es_fail(site, hint, "I can't turn empty text into a number.");
    size_t shown = 0;
    for (size_t chars = 0; shown < t->len && chars < 60; chars++) {
        shown++;
        while (shown < t->len && es_continuation_byte(t->chars[shown])) shown++;
    }
    es_fail(site, hint, "I can't turn \"%.*s%s\" into a number.", (int)shown, t->chars, shown < t->len ? "..." : "");
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

/* Every function call pushes its frame (see EsFrame); endless recursion
 * stops with a friendly error instead of crashing. The check comes before
 * the push, so the error is about the call, made while working out the
 * caller. */
static inline EsFrame *es_push(const EsFunction *fn, int call) {
    if (es_depth == ES_MAX_DEPTH) {
        es_fail(call, "Check that it stops using itself at some point.",
                "Definitions are using each other too deeply (more than %d inside each other).", ES_MAX_DEPTH);
    }
    EsFrame *frame = &es_stack[es_depth++];
    frame->fn = fn;
    frame->call = call;
    return frame;
}

/* A function with no inputs, one, or more. */
static inline void es_enter(const EsFunction *fn, int call) {
    es_push(fn, call);
}

static inline void es_enter_one(const EsFunction *fn, int call, EsArg input) {
    es_push(fn, call)->in.one = input;
}

static void es_enter_many(const EsFunction *fn, int call, const EsArg *inputs) {
    size_t n = (size_t)fn->inputs;
    if (es_inputs_len + n > es_inputs_cap) {
        size_t cap = es_inputs_cap ? es_inputs_cap * 2 : 64;
        while (cap < es_inputs_len + n) cap *= 2;
        EsArg *grown = realloc(es_inputs, cap * sizeof *grown);
        if (!grown) es_out_of_memory();
        es_inputs = grown;
        es_inputs_cap = cap;
    }
    es_push(fn, call)->in.many = es_inputs_len;
    memcpy(es_inputs + es_inputs_len, inputs, n * sizeof *inputs);
    es_inputs_len += n;
}

static inline void es_leave(void) {
    es_depth--;
}

/* Leaving a function with several inputs: their values come off es_inputs too. */
static inline void es_leave_many(int inputs) {
    es_inputs_len -= (size_t)inputs;
    es_depth--;
}

/* Input i of the function being worked out (to release text it was given). */
static inline EsArg es_input(int i) {
    return es_frame_input(&es_stack[es_depth - 1], i);
}

/* A function that gives back a value reached its end without "give back". */
static _Noreturn void es_no_result(int site, const char *name) {
    es_fail(site, "Make sure every way through it ends with \"give back ...\".",
            "\"%s\" ended without giving anything back.", name);
}

/* --- Loops ----------------------------------------------------------------- */

/* "count from A to B by S": counts up to B by S each round, and runs zero
 * times if A is above B. "count down" counts down, and runs zero times if A
 * is below B.
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

static EsCount es_count_start(int site, double from, double to, double step, bool has_step, bool down) {
    EsCount c = {from, to, 1, 1, -1, LLONG_MAX};
    if (has_step) {
        if (!(step > 0)) {
            es_fail(site, "Counting goes up by itself (or down, with \"count down\"); the step only says how far to go each time.",
                    "The step of a count has to be more than zero, but it's %s.", es_shown(step));
        }
        c.step = step;
    }
    if (down) {
        c.direction = -1;
        if (from < to && !es_close(from, to)) return c; /* empty */
    } else if (from > to && !es_close(from, to)) {
        return c; /* counting goes up only: empty */
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
static long long es_times(int site, double n) {
    if (n < 0) es_fail(site, NULL, "The number of times can't be negative, but it's %s.", es_shown(n));
    if (n != floor(n)) {
        es_fail(site, NULL, "The number of times has to be a whole number, but it's %s.", es_shown(n));
    }
    if (n > 9e18) es_fail(site, NULL, "That's too many times to repeat.");
    return (long long)n;
}

/* --- Files and input ------------------------------------------------------ */

/* The whole file as new text, without its final new line. Consumes path. */
static EsText *es_read_file(int site, EsText *path) {
    FILE *f = fopen(path->chars, "rb");
    if (!f) es_fail(site, NULL, "I couldn't read the file \"%s\": %s.", path->chars, es_reason(errno));
    if (fseek(f, 0, SEEK_END) != 0) es_fail(site, NULL, "I couldn't read the file \"%s\": %s.", path->chars, es_reason(errno));
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
        es_fail(site, NULL, "I couldn't read the file \"%s\": %s.", path->chars, es_reason(errno));
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
static void es_write_file(int site, EsText *text, EsText *path, bool append) {
    FILE *f = fopen(path->chars, append ? "ab" : "wb");
    if (!f) {
        es_fail(site, NULL, "I couldn't write to the file \"%s\": %s.", path->chars,
                errno == ENOENT ? "the folder it should go in doesn't exist" : es_reason(errno));
    }
    fwrite(text->chars, 1, text->len, f);
    fputc('\n', f);
    if (fclose(f) != 0) es_fail(site, NULL, "I couldn't write to the file \"%s\": %s.", path->chars, es_reason(errno));
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
static void es_terminal_normal(void); /* es_keys.h: lines are typed with the terminal as it was */

static EsText *es_ask(EsText *question) {
    es_terminal_normal();
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
    exit(es_status);
}

#endif
