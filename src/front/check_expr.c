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

// How a kind is written in a definition: "a number", "text", "yes or no",
// "a list of numbers".
static const char *kind_words(Checker *c, Type type) {
    if (type_is_list(type) || type_is_record(type) || type_is_sum(type) || type_is_map(type)) {
        return checker_type_name(c, type);
    }
    switch (type) {
    case TYPE_NUMBER: return "a number";
    case TYPE_TEXT: return "text";
    case TYPE_YESNO: return "yes or no";
    default: return "nothing";
    }
}

// "to area with width (a number) and height", with input `marked` given the
// kind `mark` (when marked is in range), and kinds that were written down.
static const char *header(Checker *c, const Function *f, size_t marked, Type mark) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    const Stmt *def = f->definition;
    size_t first = def->as.function.method ? 1 : 0;  // a method: "to bark, for a dog" or "a dog can bark"
    if (def->as.function.can) {
        sb_appendf(&sb, "%s can %s", checker_type_name(c, def->as.function.declared.items[0]), f->display);
    } else {
        sb_appendf(&sb, "to %s", f->display);
        if (first) sb_appendf(&sb, ", for %s", checker_type_name(c, def->as.function.declared.items[0]));
    }
    for (size_t i = first; i < def->as.function.params.len; i++) {
        if (i == first) {
            sb_appendf(&sb, "%s %s ", first && !def->as.function.can ? "," : "",
                       def->as.function.intro ? def->as.function.intro : "with");
        }
        if (i > first) sb_append(&sb, " and ");
        sb_append(&sb, def->as.function.params.items[i].display);
        Type kind = i == marked ? mark : def->as.function.declared.items[i];
        if (kind != TYPE_UNKNOWN) sb_appendf(&sb, " (%s)", kind_words(c, kind));
    }
    if (def->as.function.declared_result != TYPE_UNKNOWN) {
        sb_appendf(&sb, ", giving back %s", kind_words(c, def->as.function.declared_result));
    }
    return sb.data;
}

const char *checker_function_header(Checker *c, const Function *f) {
    return header(c, f, (size_t)-1, TYPE_UNKNOWN);
}

// --- Rules -------------------------------------------------------------------------

void checker_require(Checker *c, TypeVar var, Type type, const Expr *at, const char *message, const char *hint) {
    if (types_require(c, var, type, at->pos.line)) return;
    diag_error(c->diag, at->pos.span, "%s, but this is %s.", message, checker_kind(c, var));
    if (type_is_maybe(types_of(c, var)) && type_present(types_of(c, var)) == type) {
        checker_note_maybe(c, var, at);
    } else if (hint) {
        diag_note(c->diag, "%s", hint);
    }
}

void checker_arithmetic(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Span span, size_t line) {
    bool ok_left = types_require(c, left, TYPE_NUMBER, line);
    bool ok_right = types_require(c, right, TYPE_NUMBER, line);
    if (ok_left && ok_right) return;
    const char *l = checker_kind(c, left), *r = checker_kind(c, right);
    switch (op) {
    case BINARY_ADD:
        diag_error(c->diag, span, "I can't add %s to %s.", r, l);
        if (types_of(c, left) == TYPE_TEXT || types_of(c, right) == TYPE_TEXT) {
            diag_note(c->diag, "To join text, use \"followed by\".");
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
    diag_error(c->diag, expr->pos.span, "I can't compare %s with %s.", checker_kind(c, left), checker_kind(c, right));
    if ((l == TYPE_TEXT && r == TYPE_NUMBER) || (l == TYPE_NUMBER && r == TYPE_TEXT)) {
        diag_note(c->diag, "Turn the text into a number first with \"as a number\".");
    } else if (type_is_maybe(l) && type_present(l) == r) {
        checker_note_maybe(c, left, expr->as.binary.left);
    } else if (type_is_maybe(r) && type_present(r) == l) {
        checker_note_maybe(c, right, expr->as.binary.right);
    }
}

static void require_logic_side(Checker *c, TypeVar var, const Expr *expr, const char *side) {
    if (types_require(c, var, TYPE_YESNO, expr->pos.line)) return;
    diag_error(c->diag, expr->pos.span, "\"%s\" needs yes or no on both sides, but the %s side is %s.",
               expr->as.binary.op == BINARY_AND ? "and" : "or", side, checker_kind(c, var));
}

// "and" between two values means both are yes. It no longer joins text.
static TypeVar check_and(Checker *c, const Expr *expr, TypeVar left, TypeVar right) {
    Type l = types_of(c, left), r = types_of(c, right);
    size_t line = expr->pos.line;
    if (l == TYPE_ERROR || r == TYPE_ERROR) return types_new(c, TYPE_ERROR, line);
    if (l == TYPE_TEXT || r == TYPE_TEXT) {
        diag_error(c->diag, expr->pos.span, "\"and\" doesn't join text: it means both, for yes or no values.");
        diag_note(c->diag, "To join text, use commas in \"say\" (say \"Hello,\", name) or \"followed by\" (\"Hello, \" followed "
                           "by name).");
        diag_info(c->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        diag_choice(c->diag, "say \"Hello,\", name");
        diag_choice(c->diag, "\"Hello, \" followed by name");
        return types_new(c, TYPE_ERROR, line);
    }
    if (l == TYPE_NUMBER && r == TYPE_NUMBER) {
        diag_error(c->diag, expr->pos.span, "I can't use \"and\" between a number and a number.");
        diag_note(c->diag, "To add numbers, use \"plus\".");
        return types_new(c, TYPE_ERROR, line);
    }
    size_t before = diag_count(c->diag);
    require_logic_side(c, left, expr, "left");
    if (diag_count(c->diag) == before) require_logic_side(c, right, expr, "right");
    return types_new(c, TYPE_YESNO, line);
}

// Two records are the same record or not, even if one could be nothing.
static bool same_record_kind(Checker *c, TypeVar left, TypeVar right) {
    Type l = types_of(c, left), r = types_of(c, right);
    return (type_is_record(l) || type_is_sum(l)) && type_present(l) == type_present(r);
}

// "if rex is loyal" where nothing says what "loyal" means (or makes a name "loyal").
static bool report_unknown_adjective(Checker *c, const Expr *expr, TypeVar left) {
    const Expr *right = expr->as.binary.right;
    Type kind = types_of(c, left);
    if (right->kind != EXPR_NAME || right->module || strchr(right->as.name, ' ')) return false;
    if (!(type_is_record(kind) || type_is_sum(kind))) return false;
    const char *name = right->as.name;
    if (checker_find_made(c, name) || checker_find_later(c, name) || checker_find_constant(c, name) ||
        checker_find_function(c, name)) {
        return false;
    }
    const char *what = checker_type_name(c, type_present(kind));
    diag_error(c->diag, right->pos.span, "I don't know what \"%s\" means for %s.", right->display, what);
    diag_note(c->diag, "Say what it means, like \"%s is %s if ...\".", what, right->display);
    diag_info(c->diag, DIAG_UNKNOWN, arena_sprintf(c->arena, "what \"%s\" means", right->display), NULL);
    return true;
}

static TypeVar check_binary(Checker *c, const Expr *expr) {
    BinaryOp op = expr->as.binary.op;
    TypeVar result;
    if (checker_check_nothing_test(c, expr, &result)) return result;
    TypeVar left = checker_check_expr(c, expr->as.binary.left);
    if ((op == BINARY_EQUAL || op == BINARY_NOT_EQUAL) && report_unknown_adjective(c, expr, left)) {
        return types_new(c, TYPE_ERROR, expr->pos.line);
    }
    size_t outer = c->present.len;  // "x is not nothing and x's age is 3": the right side knows x isn't nothing
    if (op == BINARY_AND || op == BINARY_OR) checker_narrow_condition(c, expr->as.binary.left, op == BINARY_AND);
    TypeVar right = checker_check_expr(c, expr->as.binary.right);
    c->present.len = outer;
    size_t line = expr->pos.line;
    switch (op) {
    case BINARY_AND: return check_and(c, expr, left, right);
    case BINARY_OR:
        require_logic_side(c, left, expr, "left");
        require_logic_side(c, right, expr, "right");
        return types_new(c, TYPE_YESNO, line);
    case BINARY_PIECES: return types_new(c, TYPE_TEXT, line);  // any value can be a piece, like "say X"
    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
        if (!same_record_kind(c, left, right) && !types_unify(c, left, right)) report_compare(c, expr, left, right);
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
    case BINARY_JOIN: {
        TypeVar result = types_new(c, TYPE_UNKNOWN, line);
        types_defer(c, DEFER_JOIN, expr, left, right, result);
        return result;
    }
    case BINARY_CONTAINS:
    case BINARY_NOT_CONTAINS: return checker_check_contains(c, expr, left, right);
    case BINARY_STARTS_WITH:
    case BINARY_NOT_STARTS_WITH:
    case BINARY_ENDS_WITH:
    case BINARY_NOT_ENDS_WITH: return checker_check_starts(c, expr, left, right);
    case BINARY_HAS:
    case BINARY_NOT_HAS: return checker_check_has(c, expr, left, right);
    default: {
        size_t before = diag_count(c->diag);
        checker_arithmetic(c, op, left, right, expr->pos.span, line);
        if (diag_count(c->diag) > before) {  // "p plus 1" where p could be nothing
            checker_note_maybe(c, types_of(c, left) == TYPE_NUMBER ? right : left,
                               types_of(c, left) == TYPE_NUMBER ? expr->as.binary.right : expr->as.binary.left);
        }
        return types_new(c, TYPE_NUMBER, line);
    }
    }
}

static TypeVar check_unary(Checker *c, const Expr *expr) {
    TypeVar operand = checker_check_expr(c, expr->as.unary.operand);
    if (expr->as.unary.op == UNARY_NOT) {
        checker_require(c, operand, TYPE_YESNO, expr, "\"not\" needs a yes/no value", NULL);
        return types_new(c, TYPE_YESNO, expr->pos.line);
    }
    if (!types_require(c, operand, TYPE_NUMBER, expr->pos.line)) {
        diag_error(c->diag, expr->pos.span, "I can't make %s negative.", checker_kind(c, operand));
    }
    return types_new(c, TYPE_NUMBER, expr->pos.line);
}

// --- Names and calls ---------------------------------------------------------------

// "it" is the number of the innermost loop that has one (count and times
// loops), or the item of a for-each loop (or of "every" and "only").
static TypeVar check_name(Checker *c, const Expr *expr);

// "it" in a step after ", then" (its display is the step before's hidden
// name): that name, unless a loop's or question's item could be meant too.
static bool check_step_it(Checker *c, const Expr *expr, TypeVar *result) {
    if (!expr->display) return false;
    bool item = false;
    for (size_t i = 0; i < c->loops.len; i++) item |= c->loops.items[i].has_it;
    if (!*expr->display) {
        diag_error(c->diag, expr->pos.span, "The sentence before this one doesn't give anything that \"it\" could mean.");
        diag_note(c->diag, "Use a name instead, or say it in the sentence before, like \"total the amount per month "
                           "and call it totals\".");
        diag_info(c->diag, DIAG_UNKNOWN, "what \"it\" means", NULL);
    } else if (item) {
        diag_error(c->diag, expr->pos.span, "\"it\" could mean what the sentence before gave, or the item here.");
        diag_note(c->diag, "Use a name instead: end the sentence before with \"and call it ...\", or say the item's "
                           "name.");
        diag_info(c->diag, DIAG_TWO_MEANINGS, "which \"it\" is meant", NULL);
        diag_choice(c->diag, "what the sentence before gave");
        diag_choice(c->diag, "the item here");
    } else {
        Expr *e = (Expr *)expr;
        e->kind = EXPR_NAME;
        e->as.name = expr->display;
        e->display = "it";
        *result = check_name(c, e);
        return true;
    }
    *result = error_var(c);
    return true;
}

static TypeVar check_it(Checker *c, const Expr *expr) {
    TypeVar step;
    if (check_step_it(c, expr, &step)) return step;
    for (size_t i = c->loops.len; i > 0; i--) {
        if (c->loops.items[i - 1].has_it) return c->loops.items[i - 1].it;
    }
    diag_error(c->diag, expr->pos.span, "\"it\" doesn't refer to anything here.");
    if (c->lambda && c->lambda->in_loop) {
        diag_note(c->diag, "A function made inside a loop runs later, when it's called, so \"it\" can't mean the "
                           "loop's number or item there. Use the loop's name for it instead.");
    } else if (c->loops.len > 0) {
        diag_note(c->diag, "\"it\" means the number of a counting loop or a \"repeat ... times\" loop, or the item "
                           "of a \"for each\" loop, but this loop has neither. Use a name instead.");
    } else {
        diag_note(c->diag, "\"it\" only means something inside a loop, where it's the loop's number or item.");
    }
    return error_var(c);
}

static void report_unknown_function(Checker *c, const char *name, const char *display, SourcePos pos) {
    if (checker_report_hidden(c, name, display, pos)) return;
    const Constant *k = checker_find_constant(c, name);
    if (k) {
        diag_error(c->diag, pos.span, "\"%s\" is a constant, so it can't take inputs.", k->display);
        diag_note(c->diag, "A constant is a fixed value: use it by its name alone, like \"say %s\".", k->display);
        return;
    }
    diag_error(c->diag, pos.span, "I don't know how to \"%s\".", display);
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
        diag_note(c->diag, "Did you mean \"%s\"? It's defined on %s.", best->display, checker_where(c, best->definition->pos));
    } else {
        diag_note(c->diag, "Define it first, like \"to %s with someone:\" with its sentences indented below.", display);
    }
}

static void report_argument_count(Checker *c, const Function *f, size_t given, SourcePos pos) {
    size_t wanted = param_count(f);
    if (wanted == 0) {
        diag_error(c->diag, pos.span, "\"%s\" doesn't take any values, but this gives it %zu.", f->display, given);
    } else {
        diag_error(c->diag, pos.span, "\"%s\" needs %zu value%s, but this gives it %zu.", f->display, wanted,
                   wanted == 1 ? "" : "s", given);
    }
    diag_note(c->diag, "It's defined on line %zu: \"%s\".", f->definition->pos.line, checker_function_header(c, f));
}

// An argument of the wrong kind: says why the input has its kind.
// "if product is best" where only customers say what "best" means.
static bool report_other_kind(Checker *c, const Function *f, const Expr *arg, TypeVar given) {
    int defines = f->definition->as.function.defines;
    Type wanted = types_of(c, f->params.items[0]), kind = types_of(c, given);
    if (!defines || !(type_is_record(kind) || type_is_sum(kind))) return false;
    const char *mine = checker_type_name(c, wanted), *other = checker_type_name(c, type_present(kind));
    if (defines == DEFINES_ADJECTIVE) {
        const char *word = f->name + 3;  // "is best"
        diag_error(c->diag, arg->pos.span, "I know what \"%s\" means for %s, but not for %s.", word, mine, other);
        diag_note(c->diag, "Say what it means, like \"%s is %s if ...\".", other, word);
    } else {
        const char *field = f->display;
        diag_error(c->diag, arg->pos.span, "I know how to work out %s's %s, but not %s's.", mine, field, other);
        diag_note(c->diag, "Say how, like \"%s's %s is ...\".", other, field);
    }
    diag_info(c->diag, DIAG_UNKNOWN, "what it means for this kind", NULL);
    return true;
}

static void report_argument_kind(Checker *c, const Function *f, size_t i, const Expr *arg, TypeVar given) {
    if (i == 0 && report_other_kind(c, f, arg, given)) return;
    const Stmt *def = f->definition;
    const char *param = def->as.function.params.items[i].display;
    Type wanted = types_of(c, f->params.items[i]);
    diag_error(c->diag, arg->pos.span, "\"%s\" needs %s for \"%s\", but this is %s.", f->display,
               checker_type_name(c, wanted), param, checker_kind(c, given));
    if (type_is_maybe(types_of(c, given)) && type_present(types_of(c, given)) == wanted) {
        const char *path = checker_path(c, arg);
        diag_note(c->diag, "It could be nothing. Check it first, like \"if %s is not nothing:\".", path ? path : "NAME");
        return;
    }
    if (def->as.function.declared.items[i] != TYPE_UNKNOWN) {
        diag_note(c->diag, "It's defined on line %zu: \"%s\".", def->pos.line, checker_function_header(c, f));
        return;
    }
    diag_note(c->diag, "\"%s\" is %s because of line %zu. An input has one kind of value everywhere; to make it "
                       "clear, write it in the definition, like \"%s:\".",
              param, checker_type_name(c, wanted), types_line(c, f->params.items[i]), header(c, f, i, wanted));
}

// What a call gives back. A function that gives back no value can only be
// called as a sentence of its own.
static TypeVar call_result(Checker *c, const Function *f, const Expr *expr, bool as_value) {
    if (!as_value || types_of(c, f->result) != TYPE_NOTHING) return f->result;
    diag_error(c->diag, expr->pos.span, "\"%s\" doesn't give back a value, so it can't be used here.", f->display);
    diag_note(c->diag, "It's defined on line %zu. To use what it works out, end it with \"give back ...\".",
              f->definition->pos.line);
    return error_var(c);
}

// The inputs given to f (checked already), and what the call gives back.
static TypeVar finish_call(Checker *c, const Function *f, const Expr *expr, const TypeVar *args, bool as_value) {
    size_t n = expr->as.call.args.len;
    if (param_count(f) != n) {
        report_argument_count(c, f, n, expr->pos);
        return error_var(c);
    }
    for (size_t i = 0; i < n; i++) {
        const Expr *arg = expr->as.call.args.items[i];
        if (!checker_fits(c, f->params.items[i], args[i], arg, arg->pos.line)) {
            report_argument_kind(c, f, i, expr->as.call.args.items[i], args[i]);
        }
    }
    ((Expr *)expr)->as.call.target = f->definition;
    return call_result(c, f, expr, as_value);
}

// "speak, for a dog" and "speak, for a cat", and this is a fish: lists the kinds.
static void report_no_method(Checker *c, const Expr *expr, Type kind) {
    StrBuf kinds;
    sb_init(&kinds, c->arena);
    size_t count = 0, total = 0;
    for (size_t i = 0; i < c->functions.len; i++) total += strcmp(c->functions.items[i].name, expr->as.call.name) == 0;
    const Function *first = NULL;
    for (size_t i = 0; i < c->functions.len; i++) {
        const Function *f = &c->functions.items[i];
        if (strcmp(f->name, expr->as.call.name) != 0) continue;
        if (!first) first = f;
        if (count++ > 0) sb_append(&kinds, count == total ? " and " : ", ");
        sb_append(&kinds, ast_type_name(c->arena, c->records, f->definition->as.function.declared.items[0]));
    }
    const Expr *record = expr->as.call.args.items[0];
    diag_error(c->diag, record->pos.span, "\"%s\" is a method for %s, but this is %s.", first->display, kinds.data,
               ast_type_name(c->arena, c->records, kind));
    if (type_is_record(kind) || type_is_sum(kind)) {
        diag_note(c->diag, "Define one for it too, like \"to %s, for %s:\".", first->display,
                  ast_type_name(c->arena, c->records, kind));
    }
}

bool checker_settle_method(Checker *c, const Deferred *d) {
    Type kind = types_of(c, d->left);
    if (kind == TYPE_ERROR) return true;
    TypeVar dispatched;
    if (type_is_iface(kind) && checker_dispatch(c, d->expr, d->args, d->allowed, &dispatched)) {
        types_unify(c, d->result, dispatched);
        return true;
    }
    if (type_is_maybe(kind)) kind = type_present(kind);
    if (!type_is_record(kind) && !type_is_sum(kind)) {
        if (kind == TYPE_UNKNOWN || kind == TYPE_MAYBE) return false;
        report_no_method(c, d->expr, kind);
        return true;
    }
    for (size_t i = 0; i < c->functions.len; i++) {
        const Function *f = &c->functions.items[i];
        if (strcmp(f->name, d->expr->as.call.name) == 0 && f->definition->as.function.declared.items[0] == kind) {
            types_unify(c, d->result, finish_call(c, f, d->expr, d->args, d->allowed));
            return true;
        }
    }
    report_no_method(c, d->expr, kind);
    return true;
}

void checker_report_unsettled_method(Checker *c, const Deferred *d) {
    const Expr *record = d->expr->as.call.args.items[0];
    Type example = checker_find_function(c, d->expr->as.call.name)->definition->as.function.declared.items[0];
    diag_error(c->diag, record->pos.span, "I can't tell which \"%s\" this is, because I can't tell what kind of record "
               "this is.", d->expr->display);
    diag_note(c->diag, "Several kinds of record have a method called \"%s\". Say which kind this is where it's made, "
                       "like \"(%s)\" after an input.", d->expr->display,
              ast_type_name(c->arena, c->records, example));
}

static TypeVar check_call(Checker *c, const Expr *expr, bool as_value);

TypeVar checker_check_call(Checker *c, const Expr *expr, bool as_value) {
    NameContext saved;
    if (!expr->as.call.callee && !checker_enter_names(c, expr, expr->as.call.name, &saved)) {
        for (size_t i = 0; i < expr->as.call.args.len; i++) checker_check_expr(c, expr->as.call.args.items[i]);
        return error_var(c);
    }
    TypeVar result = check_call(c, expr, as_value);
    if (!expr->as.call.callee) checker_leave_names(c, &saved);
    return result;
}

static TypeVar check_call(Checker *c, const Expr *expr, bool as_value) {
    if (!expr->as.call.callee && !checker_find_function(c, expr->as.call.name) &&
        checker_find_made(c, expr->as.call.name)) {  // "f with 3": f holds a function
        Expr *name = ast_new_expr(c->arena, EXPR_NAME, expr->pos);
        name->pos.span.length = strlen(expr->display);  // the name is written first
        name->as.name = expr->as.call.name;
        name->display = expr->display;
        ((Expr *)expr)->as.call.callee = name;
    }
    if (expr->as.call.callee) return checker_check_value_call(c, expr, as_value);
    size_t n = expr->as.call.args.len;
    TypeVar *args = arena_alloc(c->arena, (n + 1) * sizeof(TypeVar));
    unsigned only_file = c->only_file;  // "helpers's double with x": x is a name here, not in helpers.es
    c->only_file = 0;
    for (size_t i = 0; i < n; i++) args[i] = checker_check_value(c, expr->as.call.args.items[i]);
    c->only_file = only_file;
    TypeVar dispatched;
    if (checker_dispatch(c, expr, args, as_value, &dispatched)) return dispatched;  // "speak pet": an interface's way
    const Function *f = checker_find_function(c, expr->as.call.name);
    if (!f) {
        report_unknown_function(c, expr->as.call.name, expr->display, expr->pos);
        return error_var(c);
    }
    checker_resolved(c, expr, f->definition);
    if (n == 0 || !checker_shared_method(c, expr->as.call.name)) return finish_call(c, f, expr, args, as_value);
    TypeVar result = types_new(c, TYPE_UNKNOWN, 0);  // which method depends on the record's kind
    Deferred d = {DEFER_METHOD, expr, args[0], 0, result, false, as_value, NULL, args};
    if (checker_settle_method(c, &d)) return result;
    vec_push(c->arena, &c->deferred, d);
    return result;
}

static TypeVar check_bare_name(Checker *c, const Expr *expr);

// A bare name: a variable, a constant, or a function that takes no values.
static TypeVar check_name(Checker *c, const Expr *expr) {
    NameContext saved;
    if (!checker_enter_names(c, expr, expr->as.name, &saved)) return error_var(c);
    TypeVar result = check_bare_name(c, expr);
    checker_leave_names(c, &saved);
    return result;
}

static TypeVar check_bare_name(Checker *c, const Expr *expr) {
    const Symbol *symbol = checker_find_made(c, expr->as.name);
    if (symbol) {
        checker_use(c, symbol, false);
        return checker_narrow(c, expr, symbol->var);
    }
    const Constant *k = checker_find_constant(c, expr->as.name);
    if (k) {
        checker_resolved(c, expr, k->definition);
        if (!k->ok) return error_var(c);
        return types_new(c, checker_const_type(c, &k->definition->as.constant.folded), k->definition->pos.line);
    }
    const Function *f = checker_find_function(c, expr->as.name);
    TypeVar library;
    if (!f && checker_library_name(c, expr, &library)) return library;  // pi, today, ...
    if (!f) {
        checker_report_unknown(c, expr->as.name, expr->display, expr->pos);
        return error_var(c);
    }
    if (param_count(f) > 0) {
        report_argument_count(c, f, 0, expr->pos);
        return error_var(c);
    }
    checker_resolved(c, expr, f->definition);
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
        diag_note(c->diag, "Use 0, empty text (\"\"), or no instead. Only a definition can give back nothing, "
                           "with \"return\" on its own.");
        return error_var(c);
    case EXPR_IT: return check_it(c, expr);
    case EXPR_NAME: return check_name(c, expr);
    case EXPR_CALL: return checker_check_call(c, expr, true);
    case EXPR_LENGTH: {
        TypeVar operand = checker_check_expr(c, expr->as.operand);
        if (type_is_map(types_of(c, operand))) {
            diag_error(c->diag, expr->as.operand->pos.span, "I can only find the length of text or a list, but this is "
                                                            "%s.", checker_kind(c, operand));
            diag_note(c->diag, "To count the entries of a map, write \"the number of entries in ages\".");
            return types_new(c, TYPE_NUMBER, line);
        }
        types_defer_one_of(c, expr->as.operand, operand, operand, KIND_TEXT | KIND_LIST,
                           "I can only find the length of text or a list");
        return types_new(c, TYPE_NUMBER, line);
    }
    case EXPR_FILE_CONTENTS:
        checker_require(c, checker_check_expr(c, expr->as.operand), TYPE_TEXT, expr->as.operand,
                        "The name of a file has to be text", NULL);
        return types_new(c, TYPE_TEXT, line);
    case EXPR_UNARY: return check_unary(c, expr);
    case EXPR_BINARY: return check_binary(c, expr);
    case EXPR_CONVERT: {
        TypeVar operand = checker_check_expr(c, expr->as.convert.operand);
        if (expr->as.convert.target == CONVERT_TO_TEXT) return types_new(c, TYPE_TEXT, line);
        if (expr->as.convert.target >= CONVERT_TO_EXACT_TEXT) {  // and "as money", "as a percent", "with commas"
            static const char *const names[] = {"as exact text", "as money", "as a percent", "with commas"};
            size_t before = diag_count(c->diag);
            checker_require(c, operand, TYPE_NUMBER, expr->as.convert.operand,
                            arena_sprintf(c->arena, "\"%s\" needs a number",
                                          names[expr->as.convert.target - CONVERT_TO_EXACT_TEXT]),
                            NULL);
            if (diag_count(c->diag) > before) diag_info(c->diag, DIAG_WRONG_KIND, NULL, "a number");
            return types_new(c, TYPE_TEXT, line);
        }
        types_defer(c, DEFER_TO_NUMBER, expr, operand, operand, operand);
        return types_new(c, TYPE_NUMBER, line);
    }
    case EXPR_LIST:
    case EXPR_EMPTY_LIST:
    case EXPR_ITEM:
    case EXPR_LIST_OP:
    case EXPR_EVERY:
    case EXPR_ONLY:
    case EXPR_SLICE: return checker_check_list_expr(c, expr);
    case EXPR_TEXT_OP: return checker_check_text_op(c, expr);
    case EXPR_FIELD: return checker_narrow(c, expr, checker_check_field(c, expr));
    case EXPR_NEW: return checker_check_new(c, expr);
    case EXPR_KIND_TEST: return checker_check_kind_test(c, expr);
    case EXPR_MAP:
    case EXPR_EMPTY_MAP:
    case EXPR_MAP_GET:
    case EXPR_MAP_WITHOUT: return checker_check_map_expr(c, expr);
    case EXPR_FUNCTION_REF: return checker_check_function_ref(c, expr);
    case EXPR_LAMBDA: return checker_check_lambda(c, expr);
    case EXPR_APPLY: return checker_check_apply(c, expr);
    case EXPR_AGGREGATE: return checker_check_aggregate(c, expr);
    case EXPR_DEFAULT: return checker_check_default(c, expr);
    case EXPR_TABLE: return checker_check_table(c, expr);
    case EXPR_LIBRARY: return checker_check_library(c, expr);
    case EXPR_ERROR: return error_var(c);
    }
    return error_var(c);
}

TypeVar checker_check_expr(Checker *c, const Expr *expr) {
    return typed(c, expr, check_expr(c, expr));
}
