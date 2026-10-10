/* EasyScript runtime, part 2: what values can do (text, arithmetic, logic,
 * comparisons, loops, functions, files). Part 1, es_value.h, comes first. */
#ifndef ES_RUNTIME_H
#define ES_RUNTIME_H

#ifndef ES_VALUE_H
#include "es_value.h"
#endif

/* --- Text ----------------------------------------------------------------- */

static EsTextObject es_lit_nan = ES_STATIC_TEXT("not a number");
static EsTextObject es_lit_infinity = ES_STATIC_TEXT("infinity");
static EsTextObject es_lit_minus_infinity = ES_STATIC_TEXT("-infinity");
static EsTextObject es_lit_yes = ES_STATIC_TEXT("yes");
static EsTextObject es_lit_no = ES_STATIC_TEXT("no");
static EsTextObject es_lit_nothing = ES_STATIC_TEXT("nothing");

/* Whole numbers print without decimals; others with up to 15 significant digits. */
static EsValue es_number_text(double x) {
    if (isnan(x)) return es_text(&es_lit_nan);
    if (isinf(x)) return x > 0 ? es_text(&es_lit_infinity) : es_text(&es_lit_minus_infinity);
    if (x == 0) x = 0; /* no "-0" */
    const char *fmt = (x == floor(x) && fabs(x) < 1e15) ? "%.0f" : "%.15g";
    int n = snprintf(NULL, 0, fmt, x);
    char *p;
    EsValue v = es_text_new((size_t)n, &p);
    snprintf(p, (size_t)n + 1, fmt, x);
    return v;
}

/* Consumes v; text is handed straight back. */
static EsValue es_to_text(EsValue v) {
    switch (v.kind) {
    case ES_NUMBER: return es_number_text(v.as.number);
    case ES_TEXT: return v;
    case ES_YESNO: return v.as.yes ? es_text(&es_lit_yes) : es_text(&es_lit_no);
    default: return es_text(&es_lit_nothing);
    }
}

/* "followed by": both sides as text, joined. Joining onto empty text gives
 * the other side back without copying. */
static EsValue es_join(EsValue a, EsValue b) {
    EsValue ta = es_to_text(a);
    EsValue tb = es_to_text(b);
    if (es_len(ta) == 0) {
        es_release(ta);
        return tb;
    }
    if (es_len(tb) == 0) {
        es_release(tb);
        return ta;
    }
    char *p;
    EsValue joined = es_text_new(es_len(ta) + es_len(tb), &p);
    memcpy(p, es_chars(ta), es_len(ta));
    memcpy(p + es_len(ta), es_chars(tb), es_len(tb));
    es_release(ta);
    es_release(tb);
    return joined;
}

static void es_say(EsValue v) {
    EsValue t = es_to_text(v);
    es_out(es_chars(t), es_len(t));
    es_out("\n", 1);
    es_release(t);
}

/* --- Arithmetic ---------------------------------------------------------
 *
 * These only succeed on numbers, which own nothing, so there's nothing to
 * release; anything else stops the program. */

static const char *const es_join_hint = "To join text, use \"and\" or \"followed by\".";

static EsValue es_add(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) return es_num(a.as.number + b.as.number);
    es_fail(line, (a.kind == ES_TEXT || b.kind == ES_TEXT) ? es_join_hint : NULL, "I can't add %s to %s.",
            es_kind_name(b), es_kind_name(a));
    return es_nothing();
}

static EsValue es_sub(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) return es_num(a.as.number - b.as.number);
    es_fail(line, NULL, "I can't subtract %s from %s.", es_kind_name(b), es_kind_name(a));
    return es_nothing();
}

static EsValue es_mul(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) return es_num(a.as.number * b.as.number);
    es_fail(line, NULL, "I can't multiply %s by %s.", es_kind_name(a), es_kind_name(b));
    return es_nothing();
}

static EsValue es_div(int line, EsValue a, EsValue b) {
    if (a.kind != ES_NUMBER || b.kind != ES_NUMBER) {
        es_fail(line, NULL, "I can't divide %s by %s.", es_kind_name(a), es_kind_name(b));
    }
    if (b.as.number == 0) es_fail(line, NULL, "You divided by zero.");
    return es_num(a.as.number / b.as.number);
}

static EsValue es_mod(int line, EsValue a, EsValue b) {
    if (a.kind != ES_NUMBER || b.kind != ES_NUMBER) {
        es_fail(line, NULL, "I can't find the remainder of %s divided by %s.", es_kind_name(a), es_kind_name(b));
    }
    if (b.as.number == 0) es_fail(line, NULL, "You divided by zero.");
    return es_num(fmod(a.as.number, b.as.number));
}

static EsValue es_neg(int line, EsValue a) {
    if (a.kind != ES_NUMBER) es_fail(line, NULL, "I can't make %s negative.", es_kind_name(a));
    return es_num(-a.as.number);
}

/* --- Logic ---------------------------------------------------------------- */

static bool es_is_yes(EsValue v) {
    return v.kind == ES_YESNO && v.as.yes;
}

static bool es_is_no(EsValue v) {
    return v.kind == ES_YESNO && !v.as.yes;
}

/* "and" once the left side is known not to be "no" (that case short-circuits
 * in the generated code). Two yes/no values: logical and. Text on either side
 * (with a number, nothing, or text): joins them (consuming both). */
static EsValue es_and(int line, EsValue a, EsValue b) {
    if (a.kind == ES_YESNO && b.kind == ES_YESNO) return b;
    if (a.kind == ES_YESNO || b.kind == ES_YESNO) {
        if (a.kind == ES_TEXT || b.kind == ES_TEXT) {
            es_fail(line, "To join them, turn the yes/no value into text first with \"as text\".",
                    "I can't use \"and\" between %s and %s.", es_kind_name(a), es_kind_name(b));
        }
        es_fail(line, NULL, "I can't use \"and\" between %s and %s.", es_kind_name(a), es_kind_name(b));
    }
    if (a.kind == ES_TEXT || b.kind == ES_TEXT) return es_join(a, b);
    es_fail(line, (a.kind == ES_NUMBER && b.kind == ES_NUMBER) ? "To add numbers, use \"plus\"." : NULL,
            "I can't use \"and\" between %s and %s.", es_kind_name(a), es_kind_name(b));
    return es_nothing();
}

/* "or" once the left side is known not to be "yes". Like "not", an "if" and a
 * loop condition, it only succeeds on yes/no values, which own nothing. */
static EsValue es_or(int line, EsValue a, EsValue b) {
    if (a.kind != ES_YESNO) es_fail(line, NULL, "\"or\" needs yes or no on both sides, but the left side is %s.", es_kind_name(a));
    if (b.kind != ES_YESNO) es_fail(line, NULL, "\"or\" needs yes or no on both sides, but the right side is %s.", es_kind_name(b));
    return b;
}

static EsValue es_not(int line, EsValue a) {
    if (a.kind != ES_YESNO) es_fail(line, NULL, "\"not\" needs a yes/no value, but this is %s.", es_kind_name(a));
    return es_yesno(!a.as.yes);
}

/* The condition of an if or "otherwise if" must be yes or no. */
static bool es_if(int line, EsValue v) {
    if (v.kind != ES_YESNO) {
        es_fail(line, "Compare it with something, like \"if x is 5\".",
                "An \"if\" needs yes or no to decide, but this is %s.", es_kind_name(v));
    }
    return v.as.yes;
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

static inline EsValue es_leave(EsValue result) {
    es_depth--;
    return result;
}

/* --- Loops ----------------------------------------------------------------- */

/* "count from A to B by S": counts toward B, up or down, by S each round.
 * "count down" never counts up (it runs zero times if A is below B). */
typedef struct {
    double from, to, step;
    int direction; /* 1 or -1 */
    bool empty;
} EsCount;

static bool es_close(double a, double b);

static EsCount es_count_start(int line, EsValue from, EsValue to, EsValue step, bool has_step, bool down) {
    EsCount c = {0, 0, 1, 1, false};
    if (from.kind != ES_NUMBER) es_fail(line, NULL, "A count has to start at a number, but this is %s.", es_kind_name(from));
    if (to.kind != ES_NUMBER) es_fail(line, NULL, "A count has to end at a number, but this is %s.", es_kind_name(to));
    if (has_step) {
        if (step.kind != ES_NUMBER) {
            es_fail(line, NULL, "The step of a count has to be a number, but this is %s.", es_kind_name(step));
        }
        if (!(step.as.number > 0)) {
            es_fail(line, "Counting goes up or down by itself; the step only says how far to go each time.",
                    "The step of a count has to be more than zero, but it's %s.", es_chars(es_number_text(step.as.number)));
        }
        c.step = step.as.number;
    }
    c.from = from.as.number;
    c.to = to.as.number;
    if (down) {
        c.direction = -1;
        c.empty = c.from < c.to && !es_close(c.from, c.to);
    } else {
        c.direction = c.from <= c.to || es_close(c.from, c.to) ? 1 : -1;
    }
    return c;
}

/* Sets *var to round i's number (releasing what it held), or returns false
 * when the count is past its end.
 * Numbers are worked out from the start each time (no drifting), and a number
 * that's equal to the end (see es_close) lands exactly on it. */
static bool es_count_next(const EsCount *c, long long i, EsValue *var) {
    if (c->empty) return false;
    double v = c->from + c->direction * (double)i * c->step;
    if (es_close(v, c->to)) {
        v = c->to;
    } else if (c->direction > 0 ? v > c->to : v < c->to) {
        return false;
    }
    es_set(var, es_num(v));
    return true;
}

/* "do this N times": N must be a whole number, zero or more. */
static long long es_times(int line, EsValue n) {
    if (n.kind != ES_NUMBER) es_fail(line, NULL, "The number of times has to be a number, but this is %s.", es_kind_name(n));
    if (n.as.number < 0) es_fail(line, NULL, "The number of times can't be negative, but it's %s.", es_chars(es_number_text(n.as.number)));
    if (n.as.number != floor(n.as.number)) {
        es_fail(line, NULL, "The number of times has to be a whole number, but it's %s.", es_chars(es_number_text(n.as.number)));
    }
    if (n.as.number > 9e18) es_fail(line, NULL, "That's too many times to repeat.");
    return (long long)n.as.number;
}

/* The condition of a while or until loop must be yes or no. */
static bool es_loop_condition(int line, EsValue v) {
    if (v.kind != ES_YESNO) {
        es_fail(line, "Compare it with something, like \"while count is less than 10\".",
                "A loop needs yes or no to decide whether to keep going, but this is %s.", es_kind_name(v));
    }
    return v.as.yes;
}

/* --- Comparisons ---------------------------------------------------------- */

/* Numbers within a relative 1e-12 of each other count as equal, so
 * 0.1 plus 0.2 is 0.3. */
static bool es_close(double a, double b) {
    if (a == b) return true;
    return fabs(a - b) <= 1e-12 * fmax(fabs(a), fabs(b));
}

static bool es_same(EsValue a, EsValue b) {
    if (a.kind != b.kind) return false;
    switch (a.kind) {
    case ES_NUMBER: return es_close(a.as.number, b.as.number);
    case ES_TEXT: return es_len(a) == es_len(b) && memcmp(es_chars(a), es_chars(b), es_len(a)) == 0;
    case ES_YESNO: return a.as.yes == b.as.yes;
    default: return true;
    }
}

static EsValue es_eq(EsValue a, EsValue b) {
    bool same = es_same(a, b);
    es_release(a);
    es_release(b);
    return es_yesno(same);
}

static EsValue es_ne(EsValue a, EsValue b) {
    bool same = es_same(a, b);
    es_release(a);
    es_release(b);
    return es_yesno(!same);
}

/* -1, 0 or 1. Numbers compare by value (numbers that count as equal give 0,
 * so "is at most" agrees with "is") and text alphabetically (by bytes). */
static int es_order(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) {
        if (es_close(a.as.number, b.as.number)) return 0;
        return a.as.number < b.as.number ? -1 : 1;
    }
    if (a.kind == ES_TEXT && b.kind == ES_TEXT) {
        size_t n = es_len(a) < es_len(b) ? es_len(a) : es_len(b);
        int c = memcmp(es_chars(a), es_chars(b), n);
        if (c != 0) return c < 0 ? -1 : 1;
        return (es_len(a) > es_len(b)) - (es_len(a) < es_len(b));
    }
    bool mixed = (a.kind == ES_TEXT && b.kind == ES_NUMBER) || (a.kind == ES_NUMBER && b.kind == ES_TEXT);
    es_fail(line, mixed ? "Turn the text into a number first with \"as a number\"." : NULL, "I can't compare %s with %s.",
            es_kind_name(a), es_kind_name(b));
    return 0;
}

/* es_order, consuming both sides. */
static int es_compare(int line, EsValue a, EsValue b) {
    int order = es_order(line, a, b);
    es_release(a);
    es_release(b);
    return order;
}

static EsValue es_lt(int line, EsValue a, EsValue b) { return es_yesno(es_compare(line, a, b) < 0); }
static EsValue es_le(int line, EsValue a, EsValue b) { return es_yesno(es_compare(line, a, b) <= 0); }
static EsValue es_gt(int line, EsValue a, EsValue b) { return es_yesno(es_compare(line, a, b) > 0); }
static EsValue es_ge(int line, EsValue a, EsValue b) { return es_yesno(es_compare(line, a, b) >= 0); }

/* --- Conversions and built-ins -------------------------------------------- */

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

static EsValue es_as_number(int line, EsValue v) {
    double x;
    switch (v.kind) {
    case ES_NUMBER: return v;
    case ES_TEXT:
        if (es_parse_number(es_chars(v), es_len(v), &x)) {
            es_release(v);
            return es_num(x);
        }
        es_fail(line, NULL, "I can't turn \"%.*s\" into a number.", (int)(es_len(v) > 60 ? 60 : es_len(v)), es_chars(v));
        return es_nothing();
    default:
        es_fail(line, NULL, "I can't turn %s into a number.", v.kind == ES_YESNO ? (v.as.yes ? "yes" : "no") : "nothing");
        return es_nothing();
    }
}

/* Length in characters (UTF-8 aware). */
static EsValue es_length(int line, EsValue v) {
    if (v.kind != ES_TEXT) es_fail(line, NULL, "I can only find the length of text, but this is %s.", es_kind_name(v));
    size_t chars = 0;
    for (size_t i = 0; i < es_len(v); i++) {
        if (((unsigned char)es_chars(v)[i] & 0xC0) != 0x80) chars++;
    }
    es_release(v);
    return es_num((double)chars);
}

/* --- Files and input ------------------------------------------------------ */

static void es_require_file_name(int line, EsValue path) {
    if (path.kind != ES_TEXT) es_fail(line, NULL, "The name of a file has to be text, but this is %s.", es_kind_name(path));
}

/* The whole file as new text, without its final new line. Consumes path. */
static EsValue es_read_file(int line, EsValue path) {
    es_require_file_name(line, path);
    FILE *f = fopen(es_chars(path), "rb");
    if (!f) es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", es_chars(path), es_reason(errno));
    if (fseek(f, 0, SEEK_END) != 0) es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", es_chars(path), es_reason(errno));
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
        es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", es_chars(path), es_reason(errno));
    }
    char *p = malloc((size_t)size + 1);
    if (!p) es_out_of_memory();
    size_t got = fread(p, 1, (size_t)size, f);
    fclose(f);
    if (got > 0 && p[got - 1] == '\n') got--;
    if (got > 0 && p[got - 1] == '\r') got--;
    EsValue text = es_text_copy(p, got);
    free(p);
    es_release(path);
    return text;
}

/* Writes the value's text and a new line, replacing or adding to the file.
 * Consumes both. */
static void es_write_file(int line, EsValue value, EsValue path, bool append) {
    es_require_file_name(line, path);
    EsValue text = es_to_text(value);
    FILE *f = fopen(es_chars(path), append ? "ab" : "wb");
    if (!f) {
        es_fail(line, NULL, "I couldn't write to the file \"%s\": %s.", es_chars(path),
                errno == ENOENT ? "the folder it should go in doesn't exist" : es_reason(errno));
    }
    fwrite(es_chars(text), 1, es_len(text), f);
    fputc('\n', f);
    if (fclose(f) != 0) es_fail(line, NULL, "I couldn't write to the file \"%s\": %s.", es_chars(path), es_reason(errno));
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
static EsValue es_ask(int line, EsValue prompt) {
    (void)line;
    EsValue question = es_to_text(prompt);
    es_out(es_chars(question), es_len(question));
    es_release(question);
    fflush(stdout);
    char *buffer = NULL;
    size_t cap = 0;
    ssize_t n = es_read_answer(&buffer, &cap);
    size_t len = n > 0 ? (size_t)n : 0;
    if (len > 0 && buffer[len - 1] == '\n') len--;
    if (len > 0 && buffer[len - 1] == '\r') len--;
    EsValue answer = len > 0 ? es_text_copy(buffer, len) : es_text(&es_empty_text);
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
