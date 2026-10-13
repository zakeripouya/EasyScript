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

/* `prefix`, then t's characters with a comma every three digits before the
 * point ("-1234567.5" is "-1,234,567.5"), then `suffix`. Consumes t. */
static EsText *es_with_commas(const char *prefix, EsText *t, const char *suffix) {
    size_t start = t->len && t->chars[0] == '-' ? 1 : 0, digits = start;
    while (digits < t->len && t->chars[digits] >= '0' && t->chars[digits] <= '9') digits++;
    size_t whole = digits - start, commas = whole > 0 ? (whole - 1) / 3 : 0;
    size_t p = strlen(prefix), q = strlen(suffix), n = p + t->len + commas + q;
    char *chars;
    EsText *out = es_text_new(n, &chars);
    size_t k = 0;
    if (start) chars[k++] = '-';
    memcpy(chars + k, prefix, p);
    k += p;
    for (size_t i = start; i < t->len; i++) {
        chars[k++] = t->chars[i];
        size_t left = digits - 1 - i; /* digits still to come before the point */
        if (i < digits - 1 && left % 3 == 0) chars[k++] = ',';
    }
    memcpy(chars + k, suffix, q);
    es_release(t);
    return out;
}

/* "with commas": "1,234,567.5". */
static EsText *es_commas_text(double x) {
    if (!isfinite(x) || fabs(x) >= 1e15) return es_number_text(x);
    return es_with_commas("", es_number_text(x), "");
}

/* "as money": a dollar sign, commas and two places: "$1,234.50", "-$5.00". */
static EsText *es_money_text(double x) {
    if (!isfinite(x) || fabs(x) >= 1e15) return es_number_text(x);
    double cents = round(fabs(x) * 100);
    int n = snprintf(NULL, 0, "%s%.2f", x < 0 && cents > 0 ? "-" : "", cents / 100);
    char *chars;
    EsText *t = es_text_new((size_t)n, &chars);
    snprintf(chars, (size_t)n + 1, "%s%.2f", x < 0 && cents > 0 ? "-" : "", cents / 100);
    return es_with_commas("$", t, "");
}

/* "as a percent": 0.25 is "25%" (rounded like any number). */
static EsText *es_percent_text(double x) {
    if (!isfinite(x)) return es_number_text(x);
    return es_with_commas("", es_number_text(x * 100), "%");
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

/* --- Exact money ------------------------------------------------------------ */

/* An amount of money is whole cents in a long long. Amounts stay below 2^53
 * cents (90 trillion), so a list, map or record that keeps one as a number
 * keeps it exactly. The currency is part of the kind: 0 dollars, 1 euros,
 * 2 pounds. Multiplying or dividing rounds to the cent, half away from zero. */
#define ES_MONEY_LIMIT 9007199254740992.0

static long long es_money_fits(int site, double cents) {
    if (!(fabs(cents) < ES_MONEY_LIMIT)) {
        es_fail(site, "Amounts can go up to 90 trillion.", isnan(cents)
                    ? "This amount of money isn't a number."
                    : "This amount of money is too big for me to keep exactly.");
    }
    return (long long)cents;
}

static inline long long es_money_add(int site, long long a, long long b) {
    return es_money_fits(site, (double)(a + b));
}

static inline long long es_money_subtract(int site, long long a, long long b) {
    return es_money_fits(site, (double)(a - b));
}

static inline long long es_money_times(int site, long long a, double n) {
    return es_money_fits(site, round((double)a * n));  /* round() goes half away from zero */
}

static inline long long es_money_divide(int site, long long a, double n) {
    if (n == 0) es_fail(site, NULL, "You divided by zero.");
    return es_money_fits(site, round((double)a / n));
}

/* "10 percent of $19.99": $2.00. */
static inline long long es_money_percent(int site, double percent, long long a) {
    return es_money_fits(site, round((double)a * percent / 100.0));
}

/* An average or a middle value of amounts: rounded to the cent. */
static inline long long es_money_round(int site, double cents) {
    return es_money_fits(site, round(cents));
}

/* An amount divided by another of the same currency: a plain number. */
static inline double es_money_ratio(int site, long long a, long long b) {
    if (b == 0) es_fail(site, NULL, "You divided by zero.");
    return (double)a / (double)b;
}

/* Whether s (n bytes) is `word`, capitals or not. */
static bool es_word_is(const char *s, size_t n, const char *word) {
    if (n != strlen(word)) return false;
    for (size_t i = 0; i < n; i++) {
        if ((s[i] | 0x20) != word[i]) return false;
    }
    return true;
}

/* "$1,200.50", "1200.5", "-€3", "12 dollars": an amount typed or read from a
 * file, in its currency (0 dollars, 1 euros, 2 pounds): its own sign or
 * none, commas between thousands, at most two decimals, and maybe the
 * currency's word after it. */
static bool es_parse_money(const char *s, size_t len, int currency, long long *cents) {
    static const char *const signs[] = {"$", "\xE2\x82\xAC", "\xC2\xA3"};
    static const char *const words[][2] = {{"dollars", "dollar"}, {"euros", "euro"}, {"pounds", "pound"}};
    const char *end = s + len;
    while (s < end && *s == ' ') s++;
    while (end > s && end[-1] == ' ') end--;
    bool minus = s < end && *s == '-';
    if (minus) s++;
    size_t n = strlen(signs[currency]);
    if ((size_t)(end - s) >= n && memcmp(s, signs[currency], n) == 0) s += n;
    double whole = 0, part = 0;
    int digits = 0, decimals = 0;
    for (; s < end && ((*s >= '0' && *s <= '9') || *s == ','); s++) {
        if (*s != ',') whole = whole * 10 + (*s - '0'), digits++;
    }
    if (s < end && *s == '.') {
        for (s++; s < end && *s >= '0' && *s <= '9' && decimals < 2; s++, decimals++) part = part * 10 + (*s - '0');
    }
    const char *word = s;
    while (word < end && *word == ' ') word++;
    if (word > s && (es_word_is(word, (size_t)(end - word), words[currency][0]) ||
                     es_word_is(word, (size_t)(end - word), words[currency][1]))) {
        s = end;
    }
    if (s != end || digits == 0) return false;
    double value = whole * 100 + (decimals == 1 ? part * 10 : part);
    if (value >= ES_MONEY_LIMIT) return false;
    *cents = (long long)(minus ? -value : value);
    return true;
}

/* "$1,200.00", "-€4.50", "£3.00". */
static EsText *es_cents_text(long long cents, int currency) {
    static const char *const signs[] = {"$", "\xE2\x82\xAC", "\xC2\xA3"};
    long long whole = cents < 0 ? -cents : cents;
    int n = snprintf(NULL, 0, "%s%lld.%02lld", cents < 0 ? "-" : "", whole / 100, whole % 100);
    char *chars;
    EsText *t = es_text_new((size_t)n, &chars);
    snprintf(chars, (size_t)n + 1, "%s%lld.%02lld", cents < 0 ? "-" : "", whole / 100, whole % 100);
    return es_with_commas(signs[currency], t, "");
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

static bool es_input_ended; /* the last answer read was the end of the input */

static EsText *es_ask(EsText *question) {
    es_terminal_normal();
    es_out(question->chars, question->len);
    es_release(question);
    fflush(stdout);
    char *buffer = NULL;
    size_t cap = 0;
    ssize_t n = es_read_answer(&buffer, &cap);
    es_input_ended = n < 0;
    size_t len = n > 0 ? (size_t)n : 0;
    if (len > 0 && buffer[len - 1] == '\n') len--;
    if (len > 0 && buffer[len - 1] == '\r') len--;
    EsText *answer = len > 0 ? es_text_copy(buffer, len) : &es_empty_text;
    free(buffer);
    return answer;
}

/* Whether an answer is the word, capitals or not. Only looks. */
static bool es_answer_is(const EsText *answer, const char *word) {
    size_t n = strlen(word);
    if (answer->len != n) return false;
    for (size_t i = 0; i < n; i++) {
        char c = answer->chars[i];
        if ((c >= 'A' && c <= 'Z' ? c + 32 : c) != word[i]) return false;
    }
    return true;
}

/* "ask for a number": asks again until the answer is a number. Consumes question. */
static double es_ask_number(int site, EsText *question) {
    for (;;) {
        EsText *answer = es_ask(es_retain(question));
        double x;
        bool ok = es_parse_number(answer->chars, answer->len, &x);
        es_release(answer);
        if (ok) {
            es_release(question);
            return x;
        }
        if (es_input_ended) {
            es_release(question);
            es_fail(site, NULL, "There's no answer to read: the input has run out.");
        }
        es_say(es_text_copy("Please type a number, like 42.", 30));
    }
}

/* "ask for an amount": asks again until the answer is an amount (with its
 * currency's sign or none). Consumes question. */
static long long es_ask_money(int site, EsText *question, int currency) {
    static const char *const examples[] = {"Please type an amount, like $12.50.", "Please type an amount, like \xE2\x82\xAC" "12.50.",
                                           "Please type an amount, like \xC2\xA3" "12.50."};
    for (;;) {
        EsText *answer = es_ask(es_retain(question));
        long long cents = 0;
        bool ok = es_parse_money(answer->chars, answer->len, currency, &cents);
        es_release(answer);
        if (ok) {
            es_release(question);
            return cents;
        }
        if (es_input_ended) {
            es_release(question);
            es_fail(site, NULL, "There's no answer to read: the input has run out.");
        }
        es_say(es_text_copy(examples[currency], strlen(examples[currency])));
    }
}

/* "ask yes or no": asks again until the answer is yes or no (or y, n, true, false). Consumes question. */
static bool es_ask_yesno(int site, EsText *question) {
    for (;;) {
        EsText *answer = es_ask(es_retain(question));
        static const char *const yes_words[] = {"yes", "y", "true"}, *const no_words[] = {"no", "n", "false"};
        bool yes = false, ok = false;
        for (size_t i = 0; i < 3; i++) {
            if (es_answer_is(answer, yes_words[i])) yes = ok = true;
            if (es_answer_is(answer, no_words[i])) ok = true;
        }
        es_release(answer);
        if (ok) {
            es_release(question);
            return yes;
        }
        if (es_input_ended) {
            es_release(question);
            es_fail(site, NULL, "There's no answer to read: the input has run out.");
        }
        es_say(es_text_copy("Please answer yes or no.", 24));
    }
}

/* "stop the program" inside a function, once the function has released its
 * own names (in main() it jumps to the end instead). See es_finish. */
static _Noreturn void es_stop(void) {
    es_finish(false);
    exit(es_status);
}

#endif
