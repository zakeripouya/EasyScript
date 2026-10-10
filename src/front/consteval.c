// Constant values, worked out by the compiler. The rules and messages mirror
// runtime/es_runtime.h on purpose: a constant must print and behave exactly
// like the same expression worked out while the program runs (tests/run/
// constant_matches_runtime.es checks this).

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "front/consteval.h"

typedef struct {
    Arena *arena;
    Diag *diag;
    ConstLookup lookup;
    void *context;
} Eval;

static const char *const fixed_note =
    "A constant is fixed before the program starts, so it can only use numbers, text, yes or no, and constants made "
    "above it. If the value is only known while the program runs, use \"let\" instead of \"keep\".";

static bool fail(Eval *ev, Span span, const char *hint, const char *fmt, ...) PRINTF_LIKE(4, 5);

static bool fail(Eval *ev, Span span, const char *hint, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const char *message = arena_vsprintf(ev->arena, fmt, args);
    va_end(args);
    diag_error(ev->diag, span, "%s", message);
    if (hint) diag_note(ev->diag, "%s", hint);
    return false;
}

static const char *kind_name(ConstValue v) {
    switch (v.kind) {
    case CONST_NUMBER: return "a number";
    case CONST_TEXT: return "text";
    default: return "a yes/no value";
    }
}

static ConstValue number(double x) {
    ConstValue v = {CONST_NUMBER, x, false, "", 0};
    return v;
}

static ConstValue yesno(bool yes) {
    ConstValue v = {CONST_YESNO, 0, yes, "", 0};
    return v;
}

static ConstValue text(const char *s, size_t len) {
    ConstValue v = {CONST_TEXT, 0, false, s, len};
    return v;
}

// --- Text (as es_number_text / es_to_text / es_join) --------------------------

const char *const_number_text(Arena *arena, double x, size_t *len) {
    const char *special = isnan(x) ? "not a number" : isinf(x) ? (x > 0 ? "infinity" : "-infinity") : NULL;
    if (special) {
        *len = strlen(special);
        return special;
    }
    if (x == 0) x = 0;  // no "-0"
    const char *fmt = (x == floor(x) && fabs(x) < 1e15) ? "%.0f" : "%.15g";
    char *s = arena_sprintf(arena, fmt, x);
    *len = strlen(s);
    return s;
}

static ConstValue to_text(Eval *ev, ConstValue v) {
    size_t len;
    switch (v.kind) {
    case CONST_NUMBER: {
        const char *s = const_number_text(ev->arena, v.number, &len);
        return text(s, len);
    }
    case CONST_TEXT: return v;
    default: return v.yes ? text("yes", 3) : text("no", 2);
    }
}

static ConstValue join(Eval *ev, ConstValue a, ConstValue b) {
    ConstValue ta = to_text(ev, a);
    ConstValue tb = to_text(ev, b);
    char *s = arena_alloc(ev->arena, ta.len + tb.len + 1);
    memcpy(s, ta.text, ta.len);
    memcpy(s + ta.len, tb.text, tb.len);
    return text(s, ta.len + tb.len);
}

// --- Comparisons (as es_close / es_same / es_order) ---------------------------

static bool close_enough(double a, double b) {
    return a == b || fabs(a - b) <= 1e-12 * fmax(fabs(a), fabs(b));
}

static bool same(ConstValue a, ConstValue b) {
    if (a.kind != b.kind) return false;
    if (a.kind == CONST_NUMBER) return close_enough(a.number, b.number);
    if (a.kind == CONST_TEXT) return a.len == b.len && memcmp(a.text, b.text, a.len) == 0;
    return a.yes == b.yes;
}

static bool order(Eval *ev, Span span, ConstValue a, ConstValue b, int *out) {
    if (a.kind == CONST_NUMBER && b.kind == CONST_NUMBER) {
        *out = close_enough(a.number, b.number) ? 0 : a.number < b.number ? -1 : 1;
        return true;
    }
    if (a.kind == CONST_TEXT && b.kind == CONST_TEXT) {
        size_t n = a.len < b.len ? a.len : b.len;
        int c = memcmp(a.text, b.text, n);
        *out = c != 0 ? (c < 0 ? -1 : 1) : (a.len > b.len) - (a.len < b.len);
        return true;
    }
    bool mixed = (a.kind == CONST_TEXT && b.kind == CONST_NUMBER) || (a.kind == CONST_NUMBER && b.kind == CONST_TEXT);
    return fail(ev, span, mixed ? "Turn the text into a number first with \"as a number\"." : NULL,
                "I can't compare %s with %s.", kind_name(a), kind_name(b));
}

// --- Operators ------------------------------------------------------------------

static bool eval(Eval *ev, const Expr *expr, ConstValue *out);

static bool arithmetic(Eval *ev, const Expr *expr, ConstValue a, ConstValue b, ConstValue *out) {
    BinaryOp op = expr->as.binary.op;
    Span span = expr->pos.span;
    bool numbers = a.kind == CONST_NUMBER && b.kind == CONST_NUMBER;
    if (op == BINARY_ADD && !numbers) {
        return fail(ev, span, (a.kind == CONST_TEXT || b.kind == CONST_TEXT) ? "To join text, use \"and\" or \"followed by\"." : NULL,
                    "I can't add %s to %s.", kind_name(b), kind_name(a));
    }
    if (op == BINARY_SUBTRACT && !numbers) return fail(ev, span, NULL, "I can't subtract %s from %s.", kind_name(b), kind_name(a));
    if (op == BINARY_MULTIPLY && !numbers) return fail(ev, span, NULL, "I can't multiply %s by %s.", kind_name(a), kind_name(b));
    if (op == BINARY_DIVIDE && !numbers) return fail(ev, span, NULL, "I can't divide %s by %s.", kind_name(a), kind_name(b));
    if (op == BINARY_MODULO && !numbers) {
        return fail(ev, span, NULL, "I can't find the remainder of %s divided by %s.", kind_name(a), kind_name(b));
    }
    if ((op == BINARY_DIVIDE || op == BINARY_MODULO) && b.number == 0) return fail(ev, span, NULL, "You divided by zero.");
    switch (op) {
    case BINARY_ADD: *out = number(a.number + b.number); break;
    case BINARY_SUBTRACT: *out = number(a.number - b.number); break;
    case BINARY_MULTIPLY: *out = number(a.number * b.number); break;
    case BINARY_DIVIDE: *out = number(a.number / b.number); break;
    default: *out = number(fmod(a.number, b.number)); break;
    }
    return true;
}

// "and" (as es_and): logical for two yes/no values, joining when text is involved.
static bool and_values(Eval *ev, const Expr *expr, ConstValue a, ConstValue b, ConstValue *out) {
    Span span = expr->pos.span;
    if (a.kind == CONST_YESNO && b.kind == CONST_YESNO) {
        *out = b;
        return true;
    }
    if (a.kind == CONST_YESNO || b.kind == CONST_YESNO) {
        bool with_text = a.kind == CONST_TEXT || b.kind == CONST_TEXT;
        return fail(ev, span, with_text ? "To join them, turn the yes/no value into text first with \"as text\"." : NULL,
                    "I can't use \"and\" between %s and %s.", kind_name(a), kind_name(b));
    }
    if (a.kind == CONST_TEXT || b.kind == CONST_TEXT) {
        *out = join(ev, a, b);
        return true;
    }
    return fail(ev, span, "To add numbers, use \"plus\".", "I can't use \"and\" between %s and %s.", kind_name(a),
                kind_name(b));
}

static bool logic(Eval *ev, const Expr *expr, ConstValue *out) {
    bool is_and = expr->as.binary.op == BINARY_AND;
    ConstValue a, b;
    if (!eval(ev, expr->as.binary.left, &a)) return false;
    if (a.kind == CONST_YESNO && a.yes != is_and) {  // "no and ..." / "yes or ..." stop here, as at run time
        *out = a;
        return true;
    }
    if (!eval(ev, expr->as.binary.right, &b)) return false;
    if (is_and) return and_values(ev, expr, a, b, out);
    if (a.kind != CONST_YESNO) {
        return fail(ev, expr->pos.span, NULL, "\"or\" needs yes or no on both sides, but the left side is %s.", kind_name(a));
    }
    if (b.kind != CONST_YESNO) {
        return fail(ev, expr->pos.span, NULL, "\"or\" needs yes or no on both sides, but the right side is %s.", kind_name(b));
    }
    *out = b;
    return true;
}

static bool binary(Eval *ev, const Expr *expr, ConstValue *out) {
    BinaryOp op = expr->as.binary.op;
    if (op == BINARY_AND || op == BINARY_OR) return logic(ev, expr, out);
    ConstValue a, b;
    if (!eval(ev, expr->as.binary.left, &a) || !eval(ev, expr->as.binary.right, &b)) return false;
    int c;
    switch (op) {
    case BINARY_JOIN: *out = join(ev, a, b); return true;
    case BINARY_EQUAL: *out = yesno(same(a, b)); return true;
    case BINARY_NOT_EQUAL: *out = yesno(!same(a, b)); return true;
    case BINARY_LESS:
    case BINARY_LESS_EQUAL:
    case BINARY_GREATER:
    case BINARY_GREATER_EQUAL:
        if (!order(ev, expr->pos.span, a, b, &c)) return false;
        *out = yesno(op == BINARY_LESS ? c < 0 : op == BINARY_LESS_EQUAL ? c <= 0 : op == BINARY_GREATER ? c > 0 : c >= 0);
        return true;
    default: return arithmetic(ev, expr, a, b, out);
    }
}

static bool unary(Eval *ev, const Expr *expr, ConstValue *out) {
    ConstValue a;
    if (!eval(ev, expr->as.unary.operand, &a)) return false;
    if (expr->as.unary.op == UNARY_NEGATE) {
        if (a.kind != CONST_NUMBER) return fail(ev, expr->pos.span, NULL, "I can't make %s negative.", kind_name(a));
        *out = number(-a.number);
    } else {
        if (a.kind != CONST_YESNO) {
            return fail(ev, expr->pos.span, NULL, "\"not\" needs a yes/no value, but this is %s.", kind_name(a));
        }
        *out = yesno(!a.yes);
    }
    return true;
}

// Spaces around it, an optional "-", digits, and optionally "." and digits
// (as es_parse_number).
static bool parse_number(const char *s, size_t len, double *out) {
    size_t i = 0, end = len;
    while (i < end && s[i] == ' ') i++;
    while (end > i && s[end - 1] == ' ') end--;
    size_t start = i;
    if (i < end && s[i] == '-') i++;
    size_t digits = i;
    while (i < end && s[i] >= '0' && s[i] <= '9') i++;
    if (i == digits) return false;
    if (i < end && s[i] == '.') {
        size_t frac = ++i;
        while (i < end && s[i] >= '0' && s[i] <= '9') i++;
        if (i == frac) return false;
    }
    if (i != end) return false;
    *out = strtod(s + start, NULL);
    return true;
}

static bool convert(Eval *ev, const Expr *expr, ConstValue *out) {
    ConstValue a;
    if (!eval(ev, expr->as.convert.operand, &a)) return false;
    if (expr->as.convert.target == CONVERT_TO_TEXT) {
        *out = to_text(ev, a);
        return true;
    }
    double x;
    if (a.kind == CONST_NUMBER) {
        *out = a;
    } else if (a.kind == CONST_TEXT && parse_number(a.text, a.len, &x)) {
        *out = number(x);
    } else if (a.kind == CONST_TEXT) {
        return fail(ev, expr->pos.span, NULL, "I can't turn \"%.*s\" into a number.", (int)(a.len > 60 ? 60 : a.len), a.text);
    } else {
        return fail(ev, expr->pos.span, NULL, "I can't turn %s into a number.", a.yes ? "yes" : "no");
    }
    return true;
}

static bool length(Eval *ev, const Expr *expr, ConstValue *out) {
    ConstValue a;
    if (!eval(ev, expr->as.operand, &a)) return false;
    if (a.kind != CONST_TEXT) {
        return fail(ev, expr->pos.span, NULL, "I can only find the length of text, but this is %s.", kind_name(a));
    }
    size_t chars = 0;
    for (size_t i = 0; i < a.len; i++) {
        if (((unsigned char)a.text[i] & 0xC0) != 0x80) chars++;
    }
    *out = number((double)chars);
    return true;
}

static bool eval(Eval *ev, const Expr *expr, ConstValue *out) {
    Span span = expr->pos.span;
    switch (expr->kind) {
    case EXPR_NUMBER: *out = number(strtod(expr->as.number.text, NULL)); return true;
    case EXPR_TEXT: *out = text(expr->as.text.value, expr->as.text.len); return true;
    case EXPR_BOOLEAN: *out = yesno(expr->as.boolean); return true;
    case EXPR_NAME: return ev->lookup(ev->context, expr, out);
    case EXPR_UNARY: return unary(ev, expr, out);
    case EXPR_BINARY: return binary(ev, expr, out);
    case EXPR_CONVERT: return convert(ev, expr, out);
    case EXPR_LENGTH: return length(ev, expr, out);
    case EXPR_NOTHING: return fail(ev, span, "Use a number, text, or yes or no.", "A constant can't be nothing.");
    case EXPR_CALL:
        return fail(ev, span, fixed_note, "A constant can't call a function, because functions only run while the "
                                          "program runs.");
    case EXPR_IT: return fail(ev, span, fixed_note, "\"it\" changes while the program runs, so a constant can't use it.");
    case EXPR_FILE_CONTENTS:
        return fail(ev, span, fixed_note, "A constant can't read a file, because files are read while the program runs.");
    case EXPR_ERROR: return false;
    }
    return false;
}

bool const_eval(Arena *arena, Diag *diag, const Expr *expr, ConstLookup lookup, void *context, ConstValue *out) {
    Eval ev = {arena, diag, lookup, context};
    return eval(&ev, expr, out);
}
