// Checking expressions: names, calls, "it", and the kind of every value.
// Type mistakes get the same wording the runtime used to give, now with the
// line and carets, before the program runs.

#include <string.h>
#include "front/check_internal.h"

static size_t param_count(const Function *f) {
    return f->definition->as.function.params.len;
}

static TypeVar typed(Checker *c, const Expr *expr, TypeVar var) {
    TypedExpr t = {(Expr *)expr, var};  // the checker writes the kind into it at the end
    vec_push(c->arena, &c->typed, t);
    return var;
}

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

// How a kind is written in a definition: "a number", "text", "yes or no".
static const char *kind_words(Type type) {
    switch (type) {
    case TYPE_NUMBER: return "a number";
    case TYPE_TEXT: return "text";
    case TYPE_YESNO: return "yes or no";
    default: return "nothing";
    }
}

// "to area with width (a number) and height", with input `marked` given the
// kind `mark` (when marked is in range), and kinds that were written down.
static const char *header(const Checker *c, const Function *f, size_t marked, Type mark) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    sb_appendf(&sb, "to %s", f->name);
    const Stmt *def = f->definition;
    for (size_t i = 0; i < def->as.function.params.len; i++) {
        sb_append(&sb, i == 0 ? " with " : " and ");
        sb_append(&sb, def->as.function.params.items[i].text);
        Type kind = i == marked ? mark : def->as.function.declared.items[i];
        if (kind != TYPE_UNKNOWN) sb_appendf(&sb, " (%s)", kind_words(kind));
    }
    if (def->as.function.declared_result != TYPE_UNKNOWN) {
        sb_appendf(&sb, ", giving back %s", kind_words(def->as.function.declared_result));
    }
    return sb.data;
}

const char *checker_function_header(const Checker *c, const Function *f) {
    return header(c, f, (size_t)-1, TYPE_UNKNOWN);
}

// --- Rules -------------------------------------------------------------------------

void checker_require(Checker *c, TypeVar var, Type type, const Expr *at, const char *message, const char *hint) {
    if (types_require(c, var, type, at->pos.line)) return;
    diag_error(c->diag, at->pos.span, "%s, but this is %s.", message, ast_type_name(types_of(c, var)));
    if (hint) diag_note(c->diag, "%s", hint);
}

void checker_arithmetic(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Span span, size_t line) {
    bool ok_left = types_require(c, left, TYPE_NUMBER, line);
    bool ok_right = types_require(c, right, TYPE_NUMBER, line);
    if (ok_left && ok_right) return;
    const char *l = ast_type_name(types_of(c, left)), *r = ast_type_name(types_of(c, right));
    switch (op) {
    case BINARY_ADD:
        diag_error(c->diag, span, "I can't add %s to %s.", r, l);
        if (types_of(c, left) == TYPE_TEXT || types_of(c, right) == TYPE_TEXT) {
            diag_note(c->diag, "To join text, use \"and\" or \"followed by\".");
        }
        break;
    case BINARY_SUBTRACT: diag_error(c->diag, span, "I can't subtract %s from %s.", r, l); break;
    case BINARY_MULTIPLY: diag_error(c->diag, span, "I can't multiply %s by %s.", l, r); break;
    case BINARY_DIVIDE: diag_error(c->diag, span, "I can't divide %s by %s.", l, r); break;
    default: diag_error(c->diag, span, "I can't find the remainder of %s divided by %s.", l, r); break;
    }
}

static void report_compare(Checker *c, const Expr *expr, TypeVar left, TypeVar right) {
    Type l = types_of(c, left), r = types_of(c, right);
    diag_error(c->diag, expr->pos.span, "I can't compare %s with %s.", ast_type_name(l), ast_type_name(r));
    if ((l == TYPE_TEXT && r == TYPE_NUMBER) || (l == TYPE_NUMBER && r == TYPE_TEXT)) {
        diag_note(c->diag, "Turn the text into a number first with \"as a number\".");
    }
}

static void require_or_side(Checker *c, TypeVar var, const Expr *expr, const char *side) {
    if (types_require(c, var, TYPE_YESNO, expr->pos.line)) return;
    diag_error(c->diag, expr->pos.span, "\"or\" needs yes or no on both sides, but the %s side is %s.", side,
               ast_type_name(types_of(c, var)));
}

static TypeVar check_binary(Checker *c, const Expr *expr) {
    BinaryOp op = expr->as.binary.op;
    TypeVar left = checker_check_expr(c, expr->as.binary.left);
    TypeVar right = checker_check_expr(c, expr->as.binary.right);
    size_t line = expr->pos.line;
    switch (op) {
    case BINARY_AND: {
        TypeVar result = types_new(c, TYPE_UNKNOWN, line);
        types_defer(c, DEFER_AND, expr, left, right, result);
        return result;
    }
    case BINARY_OR:
        require_or_side(c, left, expr, "left");
        require_or_side(c, right, expr, "right");
        return types_new(c, TYPE_YESNO, line);
    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
        if (!types_unify(c, left, right)) report_compare(c, expr, left, right);
        return types_new(c, TYPE_YESNO, line);
    case BINARY_LESS:
    case BINARY_LESS_EQUAL:
    case BINARY_GREATER:
    case BINARY_GREATER_EQUAL:
        if (types_unify(c, left, right)) {
            types_defer(c, DEFER_ORDER, expr, left, left, left);
        } else {
            report_compare(c, expr, left, right);
        }
        return types_new(c, TYPE_YESNO, line);
    case BINARY_JOIN: return types_new(c, TYPE_TEXT, line);
    default:
        checker_arithmetic(c, op, left, right, expr->pos.span, line);
        return types_new(c, TYPE_NUMBER, line);
    }
}

static TypeVar check_unary(Checker *c, const Expr *expr) {
    TypeVar operand = checker_check_expr(c, expr->as.unary.operand);
    if (expr->as.unary.op == UNARY_NOT) {
        checker_require(c, operand, TYPE_YESNO, expr, "\"not\" needs a yes/no value", NULL);
        return types_new(c, TYPE_YESNO, expr->pos.line);
    }
    if (!types_require(c, operand, TYPE_NUMBER, expr->pos.line)) {
        diag_error(c->diag, expr->pos.span, "I can't make %s negative.", ast_type_name(types_of(c, operand)));
    }
    return types_new(c, TYPE_NUMBER, expr->pos.line);
}

// --- Names and calls ---------------------------------------------------------------

// "it" is the number of the innermost loop that has one (count and times loops).
static bool check_it(Checker *c, const Expr *expr) {
    for (size_t i = c->loops.len; i > 0; i--) {
        if (c->loops.items[i - 1].has_it) return true;
    }
    diag_error(c->diag, expr->pos.span, "\"it\" doesn't refer to anything here.");
    if (c->loops.len > 0) {
        diag_note(c->diag, "\"it\" means the number of a counting loop or a \"repeat ... times\" loop, "
                           "but this loop doesn't count. Use a variable instead.");
    } else {
        diag_note(c->diag, "\"it\" only means something inside a loop, where it's the loop's number.");
    }
    return false;
}

static void report_unknown_function(Checker *c, const char *name, SourcePos pos) {
    if (checker_find_constant(c, name)) {
        diag_error(c->diag, pos.span, "\"%s\" is a constant, not a function.", name);
        diag_note(c->diag, "A constant is a fixed value: use it by its name alone, like \"say %s\".", name);
        return;
    }
    if (checker_find_made(c, name)) {
        diag_error(c->diag, pos.span, "\"%s\" is a variable, not a function.", name);
        diag_note(c->diag, "Only functions made with \"to %s ...:\" can be called.", name);
        return;
    }
    diag_error(c->diag, pos.span, "I don't know a function called \"%s\".", name);
    size_t limit = strlen(name) <= 3 ? 1 : 2;
    const Function *best = NULL;
    size_t best_distance = limit + 1;
    for (size_t i = 0; i < c->functions.len; i++) {
        size_t d = edit_distance(c->arena, name, c->functions.items[i].name);
        if (d < best_distance) {
            best_distance = d;
            best = &c->functions.items[i];
        }
    }
    if (best) {
        diag_note(c->diag, "Did you mean \"%s\"? It's defined on line %zu.", best->name, best->definition->pos.line);
    } else {
        diag_note(c->diag, "Define it first, like \"to %s someone:\" with its sentences indented below.", name);
    }
}

static void report_argument_count(Checker *c, const Function *f, size_t given, SourcePos pos) {
    size_t wanted = param_count(f);
    if (wanted == 0) {
        diag_error(c->diag, pos.span, "\"%s\" doesn't take any values, but this gives it %zu.", f->name, given);
    } else {
        diag_error(c->diag, pos.span, "\"%s\" needs %zu value%s, but this gives it %zu.", f->name, wanted,
                   wanted == 1 ? "" : "s", given);
    }
    diag_note(c->diag, "It's defined on line %zu: \"%s\".", f->definition->pos.line, checker_function_header(c, f));
}

// An argument of the wrong kind: says why the input has its kind.
static void report_argument_kind(Checker *c, const Function *f, size_t i, const Expr *arg, TypeVar given) {
    const Stmt *def = f->definition;
    const char *param = def->as.function.params.items[i].text;
    Type wanted = types_of(c, f->params.items[i]);
    diag_error(c->diag, arg->pos.span, "\"%s\" needs %s for \"%s\", but this is %s.", f->name, ast_type_name(wanted),
               param, ast_type_name(types_of(c, given)));
    if (def->as.function.declared.items[i] != TYPE_UNKNOWN) {
        diag_note(c->diag, "It's defined on line %zu: \"%s\".", def->pos.line, checker_function_header(c, f));
        return;
    }
    diag_note(c->diag, "\"%s\" is %s because of line %zu. An input has one kind of value everywhere; to make it "
                       "clear, write it in the definition, like \"%s:\".",
              param, ast_type_name(wanted), types_line(c, f->params.items[i]), header(c, f, i, wanted));
}

// What a call gives back. A function that gives back no value can only be
// called as a sentence of its own.
static TypeVar call_result(Checker *c, const Function *f, const Expr *expr, bool as_value) {
    if (!as_value || types_of(c, f->result) != TYPE_NOTHING) return f->result;
    diag_error(c->diag, expr->pos.span, "\"%s\" doesn't give back a value, so it can't be used here.", f->name);
    diag_note(c->diag, "It's defined on line %zu. To use what it works out, end it with \"give back ...\".",
              f->definition->pos.line);
    return error_var(c);
}

TypeVar checker_check_call(Checker *c, const Expr *expr, bool as_value) {
    size_t n = expr->as.call.args.len;
    TypeVar *args = arena_alloc(c->arena, (n + 1) * sizeof(TypeVar));
    for (size_t i = 0; i < n; i++) args[i] = checker_check_expr(c, expr->as.call.args.items[i]);
    const Function *f = checker_find_function(c, expr->as.call.name);
    if (!f) {
        report_unknown_function(c, expr->as.call.name, expr->pos);
        return error_var(c);
    }
    if (param_count(f) != n) {
        report_argument_count(c, f, n, expr->pos);
        return error_var(c);
    }
    for (size_t i = 0; i < n; i++) {
        if (!types_unify(c, f->params.items[i], args[i])) {
            report_argument_kind(c, f, i, expr->as.call.args.items[i], args[i]);
        }
    }
    return call_result(c, f, expr, as_value);
}

// A bare name: a variable, a constant, or a function that takes no values.
static TypeVar check_name(Checker *c, const Expr *expr) {
    const Symbol *symbol = checker_find_made(c, expr->as.name);
    if (symbol) return symbol->var;
    const Constant *k = checker_find_constant(c, expr->as.name);
    if (k) {
        if (!k->ok) return error_var(c);
        ConstKind kind = k->definition->as.constant.folded.kind;
        return types_new(c, kind == CONST_NUMBER ? TYPE_NUMBER : kind == CONST_TEXT ? TYPE_TEXT : TYPE_YESNO,
                         k->definition->pos.line);
    }
    const Function *f = checker_find_function(c, expr->as.name);
    if (!f) {
        checker_report_unknown(c, expr->as.name, expr->pos);
        return error_var(c);
    }
    if (param_count(f) > 0) {
        report_argument_count(c, f, 0, expr->pos);
        return error_var(c);
    }
    return call_result(c, f, expr, true);
}

static TypeVar check_expr(Checker *c, const Expr *expr) {
    size_t line = expr->pos.line;
    switch (expr->kind) {
    case EXPR_NUMBER: return types_new(c, TYPE_NUMBER, line);
    case EXPR_TEXT: return types_new(c, TYPE_TEXT, line);
    case EXPR_BOOLEAN: return types_new(c, TYPE_YESNO, line);
    case EXPR_NOTHING:
        diag_error(c->diag, expr->pos.span, "\"nothing\" can't be used as a value.");
        diag_note(c->diag, "Use 0, empty text (\"\"), or no instead. Only a function can give back nothing, "
                           "with \"return\" on its own.");
        return error_var(c);
    case EXPR_IT: return check_it(c, expr) ? types_new(c, TYPE_NUMBER, line) : error_var(c);
    case EXPR_NAME: return check_name(c, expr);
    case EXPR_CALL: return checker_check_call(c, expr, true);
    case EXPR_LENGTH:
        checker_require(c, checker_check_expr(c, expr->as.operand), TYPE_TEXT, expr->as.operand,
                        "I can only find the length of text", NULL);
        return types_new(c, TYPE_NUMBER, line);
    case EXPR_FILE_CONTENTS:
        checker_require(c, checker_check_expr(c, expr->as.operand), TYPE_TEXT, expr->as.operand,
                        "The name of a file has to be text", NULL);
        return types_new(c, TYPE_TEXT, line);
    case EXPR_UNARY: return check_unary(c, expr);
    case EXPR_BINARY: return check_binary(c, expr);
    case EXPR_CONVERT: {
        TypeVar operand = checker_check_expr(c, expr->as.convert.operand);
        if (expr->as.convert.target == CONVERT_TO_TEXT) return types_new(c, TYPE_TEXT, line);
        types_defer(c, DEFER_TO_NUMBER, expr, operand, operand, operand);
        return types_new(c, TYPE_NUMBER, line);
    }
    case EXPR_ERROR: return error_var(c);
    }
    return error_var(c);
}

TypeVar checker_check_expr(Checker *c, const Expr *expr) {
    return typed(c, expr, check_expr(c, expr));
}
