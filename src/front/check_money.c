// Exact money ("$19.99", "4.50 euros"): an amount plus or minus an amount of
// the same currency is an amount; an amount times or divided by a number is
// an amount (rounded to the cent while running); an amount divided by an
// amount is a number. Mixing currencies, or adding a plain number, is an
// error that says how to write it.

#include "front/check_internal.h"

static const char *sign_of(Type money) {
    return money == TYPE_DOLLARS ? "$" : money == TYPE_EUROS ? "€" : "£";
}

static TypeVar fail(Checker *c, size_t line) {
    return types_new(c, TYPE_ERROR, line);
}

static TypeVar add_or_subtract(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Type money, Span span,
                               size_t line) {
    if (types_unify(c, left, right)) return types_new(c, money, line);  // the other side becomes money too
    Type l = types_of(c, left), r = types_of(c, right);
    const char *verb = op == BINARY_ADD ? "add" : "subtract";
    if (type_is_money(l) && type_is_money(r)) {
        diag_error(c->diag, span, "I can't %s %s and %s: they're different kinds of money.", verb,
                   checker_kind(c, left), checker_kind(c, right));
        diag_note(c->diag, "Amounts in different currencies can't be mixed.");
        diag_info(c->diag, DIAG_WRONG_KIND, NULL, checker_kind(c, left));
        return fail(c, line);
    }
    TypeVar plain = type_is_money(l) ? right : left;
    diag_error(c->diag, span, "I can't %s %s and %s.", verb, checker_kind(c, left), checker_kind(c, right));
    if (types_of(c, plain) == TYPE_NUMBER) {
        diag_note(c->diag, "Write the number as money too, like \"%s5\".", sign_of(money));
    }
    diag_info(c->diag, DIAG_WRONG_KIND, NULL, checker_type_name(c, money));
    return fail(c, line);
}

static TypeVar times_or_divide(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Span span, size_t line) {
    Type l = types_of(c, left), r = types_of(c, right);
    if (op == BINARY_DIVIDE && type_is_money(l) && type_is_money(r)) {  // $10 divided by $2 is 5
        if (types_unify(c, left, right)) return types_new(c, TYPE_NUMBER, line);
        return add_or_subtract(c, BINARY_SUBTRACT, left, right, l, span, line);  // reports the mix
    }
    if (op == BINARY_DIVIDE && l == TYPE_UNKNOWN && types_unify(c, left, right)) {  // "bill divided by $3"
        return types_new(c, TYPE_NUMBER, line);
    }
    if (op == BINARY_DIVIDE && !type_is_money(l)) {
        diag_error(c->diag, span, "I can't divide %s by %s.", checker_kind(c, left), checker_kind(c, right));
        diag_note(c->diag, "An amount of money can be divided by a number, or by another amount.");
        diag_info(c->diag, DIAG_WRONG_KIND, NULL, "a number");
        return fail(c, line);
    }
    if (type_is_money(l) && type_is_money(r)) {
        diag_error(c->diag, span, "I can't multiply two amounts of money.");
        diag_note(c->diag, "Multiply an amount by a number, like \"price times 3\".");
        diag_info(c->diag, DIAG_WRONG_KIND, NULL, "a number");
        return fail(c, line);
    }
    Type money = type_is_money(l) ? l : r;
    TypeVar number = type_is_money(l) ? right : left;
    if (!types_require(c, number, TYPE_NUMBER, line)) {
        diag_error(c->diag, span, "I can't %s %s by %s.", op == BINARY_DIVIDE ? "divide" : "multiply",
                   checker_kind(c, left), checker_kind(c, right));
        diag_info(c->diag, DIAG_WRONG_KIND, NULL, "a number");
        return fail(c, line);
    }
    return types_new(c, money, line);
}

// When either side is an amount of money: the result's kind (an error
// reported if they don't fit), or 0 when neither is money.
TypeVar checker_money_arithmetic(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Span span, size_t line) {
    Type l = types_of(c, left), r = types_of(c, right);
    if (!type_is_money(l) && !type_is_money(r)) return 0;
    switch (op) {
    case BINARY_ADD:
    case BINARY_SUBTRACT: return add_or_subtract(c, op, left, right, type_is_money(l) ? l : r, span, line);
    case BINARY_MULTIPLY:
    case BINARY_DIVIDE: return times_or_divide(c, op, left, right, span, line);
    default:
        diag_error(c->diag, span, "I can't find the remainder of an amount of money.");
        diag_note(c->diag, "Divide it by a number instead, like \"price divided by 3\".");
        diag_info(c->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        return fail(c, line);
    }
}
