/* EasyScript runtime.
 *
 * This file is embedded into the compiler at build time and copied to the top
 * of every generated C program, so programs need nothing else to build.
 * Everything here is static; generated code calls these functions.
 *
 * Values are tagged: nothing, a number (double), text, or yes/no. Text made at
 * run time lives in a simple block allocator that's freed when the program
 * exits. Runtime errors print "Line N: <plain English>" to stderr and exit 1.
 */
#ifndef ES_RUNTIME_H
#define ES_RUNTIME_H

#define _POSIX_C_SOURCE 200809L /* getline */

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Programs use only some of these functions; that's expected. */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif

typedef enum { ES_NOTHING, ES_NUMBER, ES_TEXT, ES_YESNO } EsKind;

typedef struct {
    EsKind kind;
    double number;
    bool yes;
    const char *text; /* always NUL-terminated; len doesn't count the NUL */
    size_t len;
} EsValue;

/* --- Memory ------------------------------------------------------------- */

typedef struct EsBlock {
    struct EsBlock *next;
    size_t used;
    size_t cap;
    char data[];
} EsBlock;

static EsBlock *es_blocks;

static void es_free_all(void) {
    while (es_blocks) {
        EsBlock *next = es_blocks->next;
        free(es_blocks);
        es_blocks = next;
    }
}

static _Noreturn void es_out_of_memory(void) {
    fflush(stdout);
    fprintf(stderr, "The program ran out of memory.\n");
    exit(1);
}

static char *es_alloc(size_t n) {
    if (!es_blocks || es_blocks->cap - es_blocks->used < n) {
        size_t cap = n > 65536 ? n : 65536;
        EsBlock *block = malloc(sizeof(EsBlock) + cap);
        if (!block) es_out_of_memory();
        block->next = es_blocks;
        block->used = 0;
        block->cap = cap;
        es_blocks = block;
    }
    char *p = es_blocks->data + es_blocks->used;
    es_blocks->used += n;
    return p;
}

/* The interactive shell reruns the whole session for every line it keeps,
 * and sets two environment variables (normal programs see neither):
 *   ES_SKIP_OUTPUT=N  don't print the first N bytes (shown on earlier runs)
 *   ES_ANSWERS=path   answers to earlier "ask"s, one per line, used before
 *                     reading the keyboard; new answers are added to it */
static size_t es_skip_output;
static FILE *es_answers;
static bool es_answers_used_up;

static void es_close_answers(void) {
    if (es_answers) fclose(es_answers);
}

static void es_init(void) {
    atexit(es_free_all);
    const char *skip = getenv("ES_SKIP_OUTPUT");
    if (skip) es_skip_output = (size_t)strtoull(skip, NULL, 10);
    const char *answers = getenv("ES_ANSWERS");
    if (answers) {
        es_answers = fopen(answers, "a+");
        if (es_answers) rewind(es_answers);
        atexit(es_close_answers);
        setvbuf(stdin, NULL, _IONBF, 0); /* read only our own line: the shell reads the rest */
    }
}

/* Everything the program prints goes through here. */
static void es_out(const char *s, size_t n) {
    if (es_skip_output >= n) {
        es_skip_output -= n;
        return;
    }
    s += es_skip_output;
    n -= es_skip_output;
    es_skip_output = 0;
    fwrite(s, 1, n, stdout);
}

/* --- Errors ------------------------------------------------------------- */

/* Prints "Line N: message", then the hint on its own line if there is one. */
static _Noreturn void es_fail(int line, const char *hint, const char *fmt, ...) {
    va_list args;
    fflush(stdout);
    fprintf(stderr, "Line %d: ", line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    if (hint) fprintf(stderr, "%s\n", hint);
    exit(1);
}

static const char *es_kind_name(EsValue v) {
    switch (v.kind) {
    case ES_NUMBER: return "a number";
    case ES_TEXT: return "text";
    case ES_YESNO: return "a yes/no value";
    default: return "nothing";
    }
}

static const char *es_reason(int err) {
    switch (err) {
    case ENOENT: return "it doesn't exist";
    case EACCES: return "permission was denied";
    case EISDIR: return "it's a folder, not a file";
    case ENOTDIR: return "part of its path isn't a folder";
    default: return strerror(err);
    }
}

/* --- Making values ------------------------------------------------------ */

static EsValue es_nothing(void) {
    EsValue v = {ES_NOTHING, 0, false, "", 0};
    return v;
}

static EsValue es_num(double x) {
    EsValue v = {ES_NUMBER, x, false, "", 0};
    return v;
}

static EsValue es_yesno(bool yes) {
    EsValue v = {ES_YESNO, 0, yes, "", 0};
    return v;
}

/* Text from a literal or other memory that outlives the value. */
static EsValue es_text_lit(const char *text, size_t len) {
    EsValue v = {ES_TEXT, 0, false, text, len};
    return v;
}

/* Text copied into runtime memory. */
static EsValue es_text_copy(const char *text, size_t len) {
    char *p = es_alloc(len + 1);
    memcpy(p, text, len);
    p[len] = '\0';
    return es_text_lit(p, len);
}

/* --- Text ----------------------------------------------------------------- */

/* Whole numbers print without decimals; others with up to 15 significant digits. */
static EsValue es_number_text(double x) {
    if (isnan(x)) return es_text_lit("not a number", 12);
    if (isinf(x)) return x > 0 ? es_text_lit("infinity", 8) : es_text_lit("-infinity", 9);
    if (x == 0) x = 0; /* no "-0" */
    const char *fmt = (x == floor(x) && fabs(x) < 1e15) ? "%.0f" : "%.15g";
    int n = snprintf(NULL, 0, fmt, x);
    char *p = es_alloc((size_t)n + 1);
    snprintf(p, (size_t)n + 1, fmt, x);
    return es_text_lit(p, (size_t)n);
}

static EsValue es_to_text(EsValue v) {
    switch (v.kind) {
    case ES_NUMBER: return es_number_text(v.number);
    case ES_TEXT: return v;
    case ES_YESNO: return v.yes ? es_text_lit("yes", 3) : es_text_lit("no", 2);
    default: return es_text_lit("nothing", 7);
    }
}

/* "followed by": both sides as text, joined. */
static EsValue es_join(EsValue a, EsValue b) {
    EsValue ta = es_to_text(a);
    EsValue tb = es_to_text(b);
    char *p = es_alloc(ta.len + tb.len + 1);
    memcpy(p, ta.text, ta.len);
    memcpy(p + ta.len, tb.text, tb.len);
    p[ta.len + tb.len] = '\0';
    return es_text_lit(p, ta.len + tb.len);
}

static void es_say(EsValue v) {
    EsValue t = es_to_text(v);
    es_out(t.text, t.len);
    es_out("\n", 1);
}

/* --- Arithmetic --------------------------------------------------------- */

static const char *const es_join_hint = "To join text, use \"and\" or \"followed by\".";

static EsValue es_add(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) return es_num(a.number + b.number);
    es_fail(line, (a.kind == ES_TEXT || b.kind == ES_TEXT) ? es_join_hint : NULL, "I can't add %s to %s.",
            es_kind_name(b), es_kind_name(a));
    return es_nothing();
}

static EsValue es_sub(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) return es_num(a.number - b.number);
    es_fail(line, NULL, "I can't subtract %s from %s.", es_kind_name(b), es_kind_name(a));
    return es_nothing();
}

static EsValue es_mul(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) return es_num(a.number * b.number);
    es_fail(line, NULL, "I can't multiply %s by %s.", es_kind_name(a), es_kind_name(b));
    return es_nothing();
}

static EsValue es_div(int line, EsValue a, EsValue b) {
    if (a.kind != ES_NUMBER || b.kind != ES_NUMBER) {
        es_fail(line, NULL, "I can't divide %s by %s.", es_kind_name(a), es_kind_name(b));
    }
    if (b.number == 0) es_fail(line, NULL, "You divided by zero.");
    return es_num(a.number / b.number);
}

static EsValue es_mod(int line, EsValue a, EsValue b) {
    if (a.kind != ES_NUMBER || b.kind != ES_NUMBER) {
        es_fail(line, NULL, "I can't find the remainder of %s divided by %s.", es_kind_name(a), es_kind_name(b));
    }
    if (b.number == 0) es_fail(line, NULL, "You divided by zero.");
    return es_num(fmod(a.number, b.number));
}

static EsValue es_neg(int line, EsValue a) {
    if (a.kind != ES_NUMBER) es_fail(line, NULL, "I can't make %s negative.", es_kind_name(a));
    return es_num(-a.number);
}

/* --- Logic ---------------------------------------------------------------- */

static bool es_is_yes(EsValue v) {
    return v.kind == ES_YESNO && v.yes;
}

static bool es_is_no(EsValue v) {
    return v.kind == ES_YESNO && !v.yes;
}

/* "and" once the left side is known not to be "no" (that case short-circuits
 * in the generated code). Two yes/no values: logical and. Text on either side
 * (with a number, nothing, or text): joins them. */
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

/* "or" once the left side is known not to be "yes". */
static EsValue es_or(int line, EsValue a, EsValue b) {
    if (a.kind != ES_YESNO) es_fail(line, NULL, "\"or\" needs yes or no on both sides, but the left side is %s.", es_kind_name(a));
    if (b.kind != ES_YESNO) es_fail(line, NULL, "\"or\" needs yes or no on both sides, but the right side is %s.", es_kind_name(b));
    return b;
}

static EsValue es_not(int line, EsValue a) {
    if (a.kind != ES_YESNO) es_fail(line, NULL, "\"not\" needs a yes/no value, but this is %s.", es_kind_name(a));
    return es_yesno(!a.yes);
}

/* The condition of an if or "otherwise if" must be yes or no. */
static bool es_if(int line, EsValue v) {
    if (v.kind != ES_YESNO) {
        es_fail(line, "Compare it with something, like \"if x is 5\".",
                "An \"if\" needs yes or no to decide, but this is %s.", es_kind_name(v));
    }
    return v.yes;
}

/* --- Functions ------------------------------------------------------------- */

/* Every function call goes one level deeper; endless recursion stops with a
 * friendly error instead of crashing. */
#define ES_MAX_DEPTH 10000
static int es_depth;

static void es_enter(int line) {
    if (++es_depth > ES_MAX_DEPTH) {
        es_fail(line, "Check that the function stops calling itself at some point.",
                "Functions are calling each other too deeply (more than %d calls inside each other).", ES_MAX_DEPTH);
    }
}

static EsValue es_leave(EsValue result) {
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
        if (!(step.number > 0)) {
            es_fail(line, "Counting goes up or down by itself; the step only says how far to go each time.",
                    "The step of a count has to be more than zero, but it's %s.", es_number_text(step.number).text);
        }
        c.step = step.number;
    }
    c.from = from.number;
    c.to = to.number;
    if (down) {
        c.direction = -1;
        c.empty = c.from < c.to && !es_close(c.from, c.to);
    } else {
        c.direction = c.from <= c.to || es_close(c.from, c.to) ? 1 : -1;
    }
    return c;
}

/* Sets *var to round i's number, or returns false when the count is past its end.
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
    *var = es_num(v);
    return true;
}

/* "do this N times": N must be a whole number, zero or more. */
static long long es_times(int line, EsValue n) {
    if (n.kind != ES_NUMBER) es_fail(line, NULL, "The number of times has to be a number, but this is %s.", es_kind_name(n));
    if (n.number < 0) es_fail(line, NULL, "The number of times can't be negative, but it's %s.", es_number_text(n.number).text);
    if (n.number != floor(n.number)) {
        es_fail(line, NULL, "The number of times has to be a whole number, but it's %s.", es_number_text(n.number).text);
    }
    if (n.number > 9e18) es_fail(line, NULL, "That's too many times to repeat.");
    return (long long)n.number;
}

/* The condition of a while or until loop must be yes or no. */
static bool es_loop_condition(int line, EsValue v) {
    if (v.kind != ES_YESNO) {
        es_fail(line, "Compare it with something, like \"while count is less than 10\".",
                "A loop needs yes or no to decide whether to keep going, but this is %s.", es_kind_name(v));
    }
    return v.yes;
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
    case ES_NUMBER: return es_close(a.number, b.number);
    case ES_TEXT: return a.len == b.len && memcmp(a.text, b.text, a.len) == 0;
    case ES_YESNO: return a.yes == b.yes;
    default: return true;
    }
}

static EsValue es_eq(EsValue a, EsValue b) {
    return es_yesno(es_same(a, b));
}

static EsValue es_ne(EsValue a, EsValue b) {
    return es_yesno(!es_same(a, b));
}

/* -1, 0 or 1. Numbers compare by value (numbers that count as equal give 0,
 * so "is at most" agrees with "is") and text alphabetically (by bytes). */
static int es_order(int line, EsValue a, EsValue b) {
    if (a.kind == ES_NUMBER && b.kind == ES_NUMBER) {
        if (es_close(a.number, b.number)) return 0;
        return a.number < b.number ? -1 : 1;
    }
    if (a.kind == ES_TEXT && b.kind == ES_TEXT) {
        size_t n = a.len < b.len ? a.len : b.len;
        int c = memcmp(a.text, b.text, n);
        if (c != 0) return c < 0 ? -1 : 1;
        return (a.len > b.len) - (a.len < b.len);
    }
    bool mixed = (a.kind == ES_TEXT && b.kind == ES_NUMBER) || (a.kind == ES_NUMBER && b.kind == ES_TEXT);
    es_fail(line, mixed ? "Turn the text into a number first with \"as a number\"." : NULL, "I can't compare %s with %s.",
            es_kind_name(a), es_kind_name(b));
    return 0;
}

static EsValue es_lt(int line, EsValue a, EsValue b) { return es_yesno(es_order(line, a, b) < 0); }
static EsValue es_le(int line, EsValue a, EsValue b) { return es_yesno(es_order(line, a, b) <= 0); }
static EsValue es_gt(int line, EsValue a, EsValue b) { return es_yesno(es_order(line, a, b) > 0); }
static EsValue es_ge(int line, EsValue a, EsValue b) { return es_yesno(es_order(line, a, b) >= 0); }

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
        if (es_parse_number(v.text, v.len, &x)) return es_num(x);
        es_fail(line, NULL, "I can't turn \"%.*s\" into a number.", (int)(v.len > 60 ? 60 : v.len), v.text);
        return es_nothing();
    default:
        es_fail(line, NULL, "I can't turn %s into a number.", v.kind == ES_YESNO ? (v.yes ? "yes" : "no") : "nothing");
        return es_nothing();
    }
}

/* Length in characters (UTF-8 aware). */
static EsValue es_length(int line, EsValue v) {
    if (v.kind != ES_TEXT) es_fail(line, NULL, "I can only find the length of text, but this is %s.", es_kind_name(v));
    size_t chars = 0;
    for (size_t i = 0; i < v.len; i++) {
        if (((unsigned char)v.text[i] & 0xC0) != 0x80) chars++;
    }
    return es_num((double)chars);
}

/* --- Files and input ------------------------------------------------------ */

static void es_require_file_name(int line, EsValue path) {
    if (path.kind != ES_TEXT) es_fail(line, NULL, "The name of a file has to be text, but this is %s.", es_kind_name(path));
}

/* The whole file as text, without its final new line. */
static EsValue es_read_file(int line, EsValue path) {
    es_require_file_name(line, path);
    FILE *f = fopen(path.text, "rb");
    if (!f) es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", path.text, es_reason(errno));
    if (fseek(f, 0, SEEK_END) != 0) es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", path.text, es_reason(errno));
    long size = ftell(f);
    if (size < 0 || fseek(f, 0, SEEK_SET) != 0) {
        es_fail(line, NULL, "I couldn't read the file \"%s\": %s.", path.text, es_reason(errno));
    }
    char *p = es_alloc((size_t)size + 1);
    size_t got = fread(p, 1, (size_t)size, f);
    fclose(f);
    if (got > 0 && p[got - 1] == '\n') got--;
    if (got > 0 && p[got - 1] == '\r') got--;
    p[got] = '\0';
    return es_text_lit(p, got);
}

/* Writes the value's text and a new line, replacing or adding to the file. */
static void es_write_file(int line, EsValue value, EsValue path, bool append) {
    es_require_file_name(line, path);
    EsValue text = es_to_text(value);
    FILE *f = fopen(path.text, append ? "ab" : "wb");
    if (!f) es_fail(line, NULL, "I couldn't write to the file \"%s\": %s.", path.text, es_reason(errno));
    fwrite(text.text, 1, text.len, f);
    fputc('\n', f);
    if (fclose(f) != 0) es_fail(line, NULL, "I couldn't write to the file \"%s\": %s.", path.text, es_reason(errno));
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

/* Prints the question, then reads one line. At the end of input the answer is empty text. */
static EsValue es_ask(int line, EsValue prompt) {
    (void)line;
    EsValue question = es_to_text(prompt);
    es_out(question.text, question.len);
    fflush(stdout);
    char *buffer = NULL;
    size_t cap = 0;
    ssize_t n = es_read_answer(&buffer, &cap);
    size_t len = n > 0 ? (size_t)n : 0;
    if (len > 0 && buffer[len - 1] == '\n') len--;
    if (len > 0 && buffer[len - 1] == '\r') len--;
    EsValue answer = es_text_copy(len > 0 ? buffer : "", len);
    free(buffer);
    return answer;
}

static _Noreturn void es_stop(void) {
    fflush(stdout);
    exit(0);
}

#endif
