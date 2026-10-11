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
    const Records *records;
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

static Type kind_type(ConstValue v) {
    switch (v.kind) {
    case CONST_NUMBER: return TYPE_NUMBER;
    case CONST_TEXT: return TYPE_TEXT;
    case CONST_YESNO: return TYPE_YESNO;
    default: return v.list_type;
    }
}

static const char *kind_name(const Eval *ev, ConstValue v) {
    if (v.kind == CONST_MAP) {  // its kind isn't kept as a Type yet (the checker does that)
        static const char *const plural[] = {NULL, "numbers", "text", "yes/no values"};
        if (v.value_type > TYPE_YESNO) return "a map";
        return arena_sprintf(ev->arena, "a map from %s to %s", v.key_type == TYPE_TEXT ? "text" : "numbers",
                             plural[v.value_type]);
    }
    return ast_type_name(ev->arena, ev->records, kind_type(v));
}

static ConstValue number(double x) {
    ConstValue v = {.kind = CONST_NUMBER, .number = x, .text = ""};
    return v;
}

static ConstValue yesno(bool yes) {
    ConstValue v = {.kind = CONST_YESNO, .yes = yes, .text = ""};
    return v;
}

static ConstValue text(const char *s, size_t len) {
    ConstValue v = {.kind = CONST_TEXT, .text = s, .len = len};
    return v;
}

// --- Text (as es_number_text / es_to_text / es_join) --------------------------

// Whole numbers up to 2^53 print exactly (es_runtime.h's ES_EXACT_WHOLE).
#define EXACT_WHOLE 9007199254740992.0

const char *const_number_text(Arena *arena, double x, size_t *len) {
    const char *special = isnan(x) ? "not a number" : isinf(x) ? (x > 0 ? "infinity" : "-infinity") : NULL;
    if (special) {
        *len = strlen(special);
        return special;
    }
    char *s = NULL;
    if (x != floor(x) || fabs(x) > EXACT_WHOLE) {  // at most 10 significant digits
        s = arena_sprintf(arena, "%.10g", x);
        x = strtod(s, NULL);
    }
    if (x == 0) x = 0;  // no "-0"
    if (x == floor(x) && fabs(x) <= EXACT_WHOLE) s = arena_sprintf(arena, "%.0f", x);
    *len = strlen(s);
    return s;
}

const char *const_key_text(Arena *arena, double x) {
    if (isnan(x)) return "not a number";
    if (isinf(x)) return x > 0 ? "infinity" : "-infinity";
    if (x == 0) x = 0;
    return arena_sprintf(arena, (x == floor(x) && fabs(x) <= EXACT_WHOLE) ? "%.0f" : "%.12g", x);
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
                "I can't compare %s with %s.", kind_name(ev, a), kind_name(ev, b));
}

// --- Operators ------------------------------------------------------------------

static bool eval(Eval *ev, const Expr *expr, ConstValue *out);

// --- Fixed lists ------------------------------------------------------------------

static const char *const list_note =
    "A constant can be a list written out, like \"keep weekdays as a list of \"Mon\" and \"Tue\"\" (and its length). "
    "To work something out from a list, use \"let\" instead of \"keep\".";

static bool reject_list(Eval *ev, Span span) {
    return fail(ev, span, list_note, "A constant can't work things out from a list.");
}

static const char *const map_note =
    "A constant can be a map written out, like \"keep prices as a map of \"tea\" to 2\" (and its number of "
    "entries). To work something out from a map, use \"let\" instead of \"keep\".";

static bool reject_map(Eval *ev, Span span) {
    return fail(ev, span, map_note, "A constant can't work things out from a map.");
}

// An operator was given a fixed list or map: only "length of" (or "the
// number of entries in") can use one.
static bool reject_list_operand(Eval *ev, Span span, ConstValue a, ConstValue b) {
    if (a.kind == CONST_MAP || b.kind == CONST_MAP) return !reject_map(ev, span);
    if (a.kind != CONST_LIST && b.kind != CONST_LIST) return false;
    reject_list(ev, span);
    return true;
}

static bool reject_text_tool(Eval *ev, Span span) {
    return fail(ev, span, "Make it with \"let\" instead of \"keep\".", "A constant can't take text apart or look inside it.");
}

static bool reject_nested(Eval *ev, Span span) {
    return fail(ev, span, "Make it with \"let\" instead of \"keep\".",
                "A fixed list can't hold lists or maps, because those could change.");
}

// a list of A, B and C: every item fixed, of one kind, and not a list or map.
static bool list(Eval *ev, const Expr *expr, ConstValue *out) {
    size_t n = expr->as.items.len;
    ConstValue *items = arena_alloc(ev->arena, n * sizeof *items);
    for (size_t i = 0; i < n; i++) {
        const Expr *item = expr->as.items.items[i];
        if (!eval(ev, item, &items[i])) return false;
        if (items[i].kind == CONST_LIST || items[i].kind == CONST_MAP) return reject_nested(ev, item->pos.span);
        if (items[i].kind != items[0].kind) {
            return fail(ev, item->pos.span, NULL,
                        "All the items of a list have to be the same kind: the first is %s, but this is %s.",
                        kind_name(ev, items[0]), kind_name(ev, items[i]));
        }
    }
    ConstValue v = {.kind = CONST_LIST, .text = "", .len = n, .list_type = type_list_of(kind_type(items[0])), .items = items};
    *out = v;
    return true;
}

static bool empty_list(Eval *ev, const Expr *expr, ConstValue *out) {
    Type item = type_item(expr->as.list_type);
    if (type_is_list(item) || type_is_map(item)) return reject_nested(ev, expr->pos.span);
    ConstValue v = {.kind = CONST_LIST, .text = "", .len = 0, .list_type = expr->as.list_type};
    *out = v;
    return true;
}

// --- Fixed maps --------------------------------------------------------------------

static bool reject_map_values(Eval *ev, Span span) {
    return fail(ev, span, "Make it with \"let\" instead of \"keep\".",
                "A fixed map can't hold lists or maps, because those could change.");
}

// Whether two fixed keys are the same key: text with the same characters,
// or numbers that are the same to 12 significant digits (as es_key_number).
static bool same_key(Eval *ev, ConstValue a, ConstValue b) {
    if (a.kind == CONST_TEXT) return a.len == b.len && memcmp(a.text, b.text, a.len) == 0;
    return strcmp(const_key_text(ev->arena, a.number), const_key_text(ev->arena, b.number)) == 0;
}

static bool map_key(Eval *ev, const Expr *expr, ConstValue *keys, size_t i) {
    if (!eval(ev, expr, &keys[i])) return false;
    if (keys[i].kind != CONST_TEXT && keys[i].kind != CONST_NUMBER) {
        return fail(ev, expr->pos.span, NULL, "A map's keys have to be text or numbers, but this is %s.",
                    kind_name(ev, keys[i]));
    }
    if (keys[i].kind != keys[0].kind) {
        return fail(ev, expr->pos.span, NULL, "All the keys of a map have to be the same kind: the first is %s, but "
                                              "this is %s.", kind_name(ev, keys[0]), kind_name(ev, keys[i]));
    }
    for (size_t j = 0; j < i; j++) {
        if (!same_key(ev, keys[j], keys[i])) continue;
        size_t len;
        const char *shown = keys[i].kind == CONST_TEXT ? arena_sprintf(ev->arena, "\"%s\"", keys[i].text)
                                                       : const_number_text(ev->arena, keys[i].number, &len);
        return fail(ev, expr->pos.span, "Each key has one value, so give each key once.",
                    "The key %s is in this map twice.", shown);
    }
    return true;
}

// a map of K to V, ...: every key and value fixed, keys text or numbers of
// one kind (each once), values of one kind and not lists or maps.
static bool map(Eval *ev, const Expr *expr, ConstValue *out) {
    size_t n = expr->as.items.len / 2;
    ConstValue *keys = arena_alloc(ev->arena, n * sizeof *keys), *values = arena_alloc(ev->arena, n * sizeof *values);
    for (size_t i = 0; i < n; i++) {
        const Expr *value = expr->as.items.items[2 * i + 1];
        if (!map_key(ev, expr->as.items.items[2 * i], keys, i) || !eval(ev, value, &values[i])) return false;
        if (values[i].kind == CONST_LIST || values[i].kind == CONST_MAP) return reject_map_values(ev, value->pos.span);
        if (values[i].kind != values[0].kind) {
            return fail(ev, value->pos.span, NULL, "All the values of a map have to be the same kind: the first is %s, "
                                                   "but this is %s.", kind_name(ev, values[0]), kind_name(ev, values[i]));
        }
    }
    ConstValue v = {.kind = CONST_MAP, .text = "", .len = n, .items = values, .keys = keys,
                    .key_type = kind_type(keys[0]), .value_type = kind_type(values[0])};
    *out = v;
    return true;
}

static bool empty_map(Eval *ev, const Expr *expr, ConstValue *out) {
    Type value = ast_map_value(ev->records, expr->as.list_type);
    if (type_is_list(value) || type_is_map(value)) return reject_map_values(ev, expr->pos.span);
    ConstValue v = {.kind = CONST_MAP, .text = "", .key_type = ast_map_key(ev->records, expr->as.list_type),
                    .value_type = value};
    *out = v;
    return true;
}

// "the number of entries in M" for a fixed map; anything else worked out from a map is an error.
static bool map_op(Eval *ev, const Expr *expr, ConstValue *out) {
    if (expr->as.list_op.op != LIST_ENTRIES) {
        return expr->as.list_op.op == LIST_KEYS || expr->as.list_op.op == LIST_VALUES ? reject_map(ev, expr->pos.span)
                                                                                       : reject_list(ev, expr->pos.span);
    }
    ConstValue a;
    if (!eval(ev, expr->as.list_op.operand, &a)) return false;
    if (a.kind != CONST_MAP) {
        return fail(ev, expr->pos.span, NULL, "I can only count the entries of a map, but this is %s.", kind_name(ev, a));
    }
    *out = number((double)a.len);
    return true;
}

static bool arithmetic(Eval *ev, const Expr *expr, ConstValue a, ConstValue b, ConstValue *out) {
    BinaryOp op = expr->as.binary.op;
    Span span = expr->pos.span;
    bool numbers = a.kind == CONST_NUMBER && b.kind == CONST_NUMBER;
    if (op == BINARY_ADD && !numbers) {
        return fail(ev, span, (a.kind == CONST_TEXT || b.kind == CONST_TEXT) ? "To join text, use \"and\" or \"followed by\"." : NULL,
                    "I can't add %s to %s.", kind_name(ev, b), kind_name(ev, a));
    }
    if (op == BINARY_SUBTRACT && !numbers) return fail(ev, span, NULL, "I can't subtract %s from %s.", kind_name(ev, b), kind_name(ev, a));
    if (op == BINARY_MULTIPLY && !numbers) return fail(ev, span, NULL, "I can't multiply %s by %s.", kind_name(ev, a), kind_name(ev, b));
    if (op == BINARY_DIVIDE && !numbers) return fail(ev, span, NULL, "I can't divide %s by %s.", kind_name(ev, a), kind_name(ev, b));
    if (op == BINARY_MODULO && !numbers) {
        return fail(ev, span, NULL, "I can't find the remainder of %s divided by %s.", kind_name(ev, a), kind_name(ev, b));
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
                    "I can't use \"and\" between %s and %s.", kind_name(ev, a), kind_name(ev, b));
    }
    if (a.kind == CONST_TEXT || b.kind == CONST_TEXT) {
        *out = join(ev, a, b);
        return true;
    }
    return fail(ev, span, "To add numbers, use \"plus\".", "I can't use \"and\" between %s and %s.", kind_name(ev, a),
                kind_name(ev, b));
}

static bool logic(Eval *ev, const Expr *expr, ConstValue *out) {
    bool is_and = expr->as.binary.op == BINARY_AND;
    ConstValue a, b;
    if (!eval(ev, expr->as.binary.left, &a)) return false;
    if (reject_list_operand(ev, expr->pos.span, a, a)) return false;
    if (a.kind == CONST_YESNO && a.yes != is_and) {  // "no and ..." / "yes or ..." stop here, as at run time
        *out = a;
        return true;
    }
    if (!eval(ev, expr->as.binary.right, &b)) return false;
    if (reject_list_operand(ev, expr->pos.span, b, b)) return false;
    if (is_and) return and_values(ev, expr, a, b, out);
    if (a.kind != CONST_YESNO) {
        return fail(ev, expr->pos.span, NULL, "\"or\" needs yes or no on both sides, but the left side is %s.", kind_name(ev, a));
    }
    if (b.kind != CONST_YESNO) {
        return fail(ev, expr->pos.span, NULL, "\"or\" needs yes or no on both sides, but the right side is %s.", kind_name(ev, b));
    }
    *out = b;
    return true;
}

static bool binary(Eval *ev, const Expr *expr, ConstValue *out) {
    BinaryOp op = expr->as.binary.op;
    if (op == BINARY_STARTS_WITH || op == BINARY_NOT_STARTS_WITH || op == BINARY_ENDS_WITH ||
        op == BINARY_NOT_ENDS_WITH) {
        return reject_text_tool(ev, expr->pos.span);
    }
    if (op == BINARY_CONTAINS || op == BINARY_NOT_CONTAINS) {
        ConstValue in;
        if (!eval(ev, expr->as.binary.left, &in)) return false;
        if (in.kind == CONST_MAP) return reject_map(ev, expr->pos.span);
        return in.kind == CONST_LIST ? reject_list(ev, expr->pos.span) : reject_text_tool(ev, expr->pos.span);
    }
    if (op == BINARY_HAS || op == BINARY_NOT_HAS) {
        ConstValue in;
        if (!eval(ev, expr->as.binary.left, &in)) return false;
        if (in.kind == CONST_MAP) return reject_map(ev, expr->pos.span);
        return fail(ev, expr->pos.span, NULL, "I can only check whether a map has a key, but this is %s.",
                    kind_name(ev, in));
    }
    if (op == BINARY_AND || op == BINARY_OR) return logic(ev, expr, out);
    ConstValue a, b;
    if (!eval(ev, expr->as.binary.left, &a) || !eval(ev, expr->as.binary.right, &b)) return false;
    if (reject_list_operand(ev, expr->pos.span, a, b)) return false;
    int c = 0;
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
    if (reject_list_operand(ev, expr->pos.span, a, a)) return false;
    if (expr->as.unary.op == UNARY_NEGATE) {
        if (a.kind != CONST_NUMBER) return fail(ev, expr->pos.span, NULL, "I can't make %s negative.", kind_name(ev, a));
        *out = number(-a.number);
    } else {
        if (a.kind != CONST_YESNO) {
            return fail(ev, expr->pos.span, NULL, "\"not\" needs a yes/no value, but this is %s.", kind_name(ev, a));
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

// The runtime's message (es_text_number): the text up to its 60th character.
static bool not_a_number(Eval *ev, Span span, ConstValue a) {
    static const char *const hint = "A number is digits, with a \"-\" before them if it's negative and a \".\" for "
                                    "decimals, like 42, -7 or 3.5.";
    if (a.len == 0) return fail(ev, span, hint, "I can't turn empty text into a number.");
    size_t shown = 0;
    for (size_t chars = 0; shown < a.len && chars < 60; chars++) {
        shown++;
        while (shown < a.len && ((unsigned char)a.text[shown] & 0xC0) == 0x80) shown++;
    }
    return fail(ev, span, hint, "I can't turn \"%.*s%s\" into a number.", (int)shown, a.text, shown < a.len ? "..." : "");
}

static bool convert(Eval *ev, const Expr *expr, ConstValue *out) {
    ConstValue a;
    if (!eval(ev, expr->as.convert.operand, &a)) return false;
    if (reject_list_operand(ev, expr->pos.span, a, a)) return false;
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
        return not_a_number(ev, expr->pos.span, a);
    } else {
        return fail(ev, expr->pos.span, NULL, "I can't turn %s into a number.", a.yes ? "yes" : "no");
    }
    return true;
}

static bool length(Eval *ev, const Expr *expr, ConstValue *out) {
    ConstValue a;
    if (!eval(ev, expr->as.operand, &a)) return false;
    if (a.kind == CONST_LIST) {  // the length of a fixed list
        *out = number((double)a.len);
        return true;
    }
    if (a.kind != CONST_TEXT) {
        return fail(ev, expr->pos.span, NULL, "I can only find the length of text or a list, but this is %s.", kind_name(ev, a));
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
    case EXPR_LIST: return list(ev, expr, out);
    case EXPR_EMPTY_LIST: return empty_list(ev, expr, out);
    case EXPR_ITEM:
        return expr->as.item.characters ? reject_text_tool(ev, span) : reject_list(ev, span);
    case EXPR_SLICE:
        return expr->as.slice.characters ? reject_text_tool(ev, span) : reject_list(ev, span);
    case EXPR_LIST_OP: return map_op(ev, expr, out);
    case EXPR_EVERY:
    case EXPR_ONLY:
    case EXPR_APPLY: return reject_list(ev, span);
    case EXPR_MAP: return map(ev, expr, out);
    case EXPR_EMPTY_MAP: return empty_map(ev, expr, out);
    case EXPR_MAP_GET:
    case EXPR_MAP_WITHOUT: return reject_map(ev, span);
    case EXPR_TEXT_OP: return reject_text_tool(ev, span);
    case EXPR_FILE_CONTENTS:
        return fail(ev, span, fixed_note, "A constant can't read a file, because files are read while the program runs.");
    case EXPR_FIELD:
        return fail(ev, span, fixed_note, "A constant can't use a field of a record, because records are made while "
                                          "the program runs.");
    case EXPR_NEW:
        if (type_is_sum(expr->as.new_record.type)) {
            return fail(ev, span, fixed_note, "A constant can't be %s, because those are made while the program runs.",
                        ast_type_name(ev->arena, ev->records, expr->as.new_record.type));
        }
        return fail(ev, span, fixed_note, "A constant can't be a record, because records are made while the program "
                                          "runs.");
    case EXPR_KIND_TEST:
        return fail(ev, span, fixed_note, "A constant can't check which kind a value is, because those values are "
                                          "made while the program runs.");
    case EXPR_FUNCTION_REF:
    case EXPR_LAMBDA:
        return fail(ev, span, fixed_note, "A constant can't be a function, because functions are kept as values while "
                                          "the program runs.");
    case EXPR_ERROR: return false;
    }
    return false;
}

bool const_eval(Arena *arena, Diag *diag, const Records *records, const Expr *expr, ConstLookup lookup, void *context,
                ConstValue *out) {
    Eval ev = {arena, diag, records, lookup, context};
    return eval(&ev, expr, out);
}
