// Checking text tools: what each needs (text, a list, a number) and what it
// makes. "contains" and "the position of" work on text and on lists, so
// they wait until the kind of what they look in is known (DEFER_CONTAINS).

#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

static void require_text(Checker *c, const Expr *expr, const char *message, const char *hint) {
    checker_require(c, checker_check_expr(c, expr), TYPE_TEXT, expr, message, hint);
}

// --- Characters ---------------------------------------------------------------------

// "character 2 of T", "characters 2 to 4 of T", ...: the positions and
// counts are numbers, and T is text. The result is text.
TypeVar checker_check_characters(Checker *c, const Expr *expr) {
    const Expr *text;
    if (expr->kind == EXPR_ITEM) {
        const Expr *position = expr->as.item.position;
        if (position) {
            checker_require(c, checker_check_expr(c, position), TYPE_NUMBER, position,
                            "The position of a character has to be a number", NULL);
        }
        text = expr->as.item.list;
    } else {
        const Expr *parts[] = {expr->as.slice.from, expr->as.slice.to};
        bool position = expr->as.slice.kind == SLICE_RANGE || expr->as.slice.kind == SLICE_WITHOUT_AT;
        const char *message = position ? "The position of a character has to be a number"
                                                                  : "The number of characters has to be a number";
        for (size_t i = 0; i < 2; i++) {
            if (parts[i]) checker_require(c, checker_check_expr(c, parts[i]), TYPE_NUMBER, parts[i], message, NULL);
        }
        text = expr->as.slice.list;
    }
    TypeVar var = checker_check_expr(c, text);
    if (types_require(c, var, TYPE_TEXT, text->pos.line)) return types_new(c, TYPE_TEXT, expr->pos.line);
    bool one = expr->kind == EXPR_ITEM;
    diag_error(c->diag, text->pos.span, "I can only take %s from text, but this is %s.", one ? "a character" : "characters",
               checker_kind(c, var));
    if (type_is_list(types_of(c, var))) {
        diag_note(c->diag, "To take %s of a list, write it with \"%s\", like \"%s\".", one ? "an item" : "items",
                  one ? "item" : "items", one ? "item 1 of scores" : "items 2 to 4 of scores");
    }
    return error_var(c);
}

// --- Looking for something -------------------------------------------------------------

// The text or list "contains" or "the position of" looks in, and what it looks for.
static void search_parts(const Expr *expr, const Expr **in, const Expr **wanted) {
    if (expr->kind == EXPR_BINARY) {
        *in = expr->as.binary.left;
        *wanted = expr->as.binary.right;
    } else {
        *in = expr->as.text_op.operand;
        *wanted = expr->as.text_op.arg;
    }
}

bool checker_settle_search(Checker *c, const Deferred *d) {
    const Expr *in, *wanted;
    search_parts(d->expr, &in, &wanted);
    Type in_type = types_of(c, d->left), wanted_type = types_of(c, d->right);
    if (in_type == TYPE_ERROR || wanted_type == TYPE_ERROR) return true;
    if (in_type == TYPE_UNKNOWN) {
        // Text only holds text, so looking for anything else means a list.
        if (wanted_type == TYPE_UNKNOWN || wanted_type == TYPE_TEXT) return false;
        types_unify(c, d->left, types_new_list(c, d->right, in->pos.line));
        in_type = types_of(c, d->left);
    }
    if (type_is_list(in_type)) {
        checker_require_item(c, in, d->left, wanted, d->right);
        return true;
    }
    if (in_type != TYPE_TEXT) {
        diag_error(c->diag, in->pos.span, "I can only look for something in text or a list, but this is %s.",
                   checker_kind(c, d->left));
        if (type_is_map(in_type)) diag_note(c->diag, "To check whether a map has a key, use \"has\".");
        return true;
    }
    if (!types_require(c, d->right, TYPE_TEXT, wanted->pos.line)) {
        diag_error(c->diag, wanted->pos.span, "I can only look for text in text, but this is %s.",
                   checker_kind(c, d->right));
        diag_note(c->diag, "Turn it into text first with \"as text\".");
    }
    return true;
}

// "T contains X" and "the position of X in T": T is text or a list.
static void defer_search(Checker *c, const Expr *expr, TypeVar in, TypeVar wanted) {
    types_defer(c, DEFER_SEARCH, expr, in, wanted, 0);
}

TypeVar checker_check_contains(Checker *c, const Expr *expr, TypeVar left, TypeVar right) {
    defer_search(c, expr, left, right);
    return types_new(c, TYPE_YESNO, expr->pos.line);
}

// "starts with" / "ends with": text on both sides.
TypeVar checker_check_starts(Checker *c, const Expr *expr, TypeVar left, TypeVar right) {
    bool starts = expr->as.binary.op == BINARY_STARTS_WITH || expr->as.binary.op == BINARY_NOT_STARTS_WITH;
    const char *message = starts ? "\"starts with\" needs text on both sides" : "\"ends with\" needs text on both sides";
    checker_require(c, left, TYPE_TEXT, expr->as.binary.left, message, NULL);
    checker_require(c, right, TYPE_TEXT, expr->as.binary.right, message, NULL);
    return types_new(c, TYPE_YESNO, expr->pos.line);
}

// --- Text tools ---------------------------------------------------------------------------

// The message for a text tool given something that isn't text.
static const char *needs_text(TextOp op) {
    switch (op) {
    case TEXT_CHARACTERS: return "I can only take the characters of text";
    case TEXT_WORDS: return "I can only take the words of text";
    case TEXT_LINES: return "I can only take the lines of text";
    case TEXT_UPPERCASE: return "I can only put text in uppercase";
    case TEXT_LOWERCASE: return "I can only put text in lowercase";
    case TEXT_TRIM: return "I can only take extra spaces off text";
    case TEXT_SPLIT: return "I can only split text";
    case TEXT_REPLACE: return "I can only replace things in text";
    case TEXT_CODE: return "I can only find the code of a character";
    case TEXT_IS_LETTER: return "I can only check whether text is a letter";
    case TEXT_IS_DIGIT: return "I can only check whether text is a digit";
    case TEXT_IS_SPACE: return "I can only check whether text is a space";
    default: return NULL;
    }
}

// "L joined with S": a list of numbers, text or yes/no values, and text between them.
static TypeVar check_joined(Checker *c, const Expr *expr) {
    static const char *const message = "I can only join numbers, text or yes/no values";
    const Expr *list = expr->as.text_op.operand;
    TypeVar var = checker_check_expr(c, list);
    TypeVar item = checker_require_list(c, var, list, "I can only join the items of a list");
    if (types_of(c, item) != TYPE_ERROR) {
        types_defer_one_of(c, list, item, var, KIND_NUMBER | KIND_TEXT | KIND_YESNO | KIND_DATE, message);
    }
    require_text(c, expr->as.text_op.arg, "The items can only be joined with text", NULL);
    return types_new(c, TYPE_TEXT, expr->pos.line);
}

TypeVar checker_check_text_op(Checker *c, const Expr *expr) {
    TextOp op = expr->as.text_op.op;
    size_t line = expr->pos.line;
    switch (op) {
    case TEXT_JOINED: return check_joined(c, expr);
    case TEXT_POSITION: {
        TypeVar wanted = checker_check_expr(c, expr->as.text_op.arg);  // written first
        TypeVar in = checker_check_expr(c, expr->as.text_op.operand);
        defer_search(c, expr, in, wanted);
        return types_new(c, TYPE_MAYBE + TYPE_NUMBER, line);  // nothing when it isn't there
    }
    case TEXT_FROM_CODE:
        checker_require(c, checker_check_expr(c, expr->as.text_op.operand), TYPE_NUMBER, expr->as.text_op.operand,
                        "A character code has to be a number", NULL);
        return types_new(c, TYPE_TEXT, line);
    default: break;
    }
    require_text(c, expr->as.text_op.operand, needs_text(op), NULL);
    switch (op) {
    case TEXT_SPLIT: require_text(c, expr->as.text_op.arg, "Text can only be split by text", NULL); break;
    case TEXT_REPLACE:
        require_text(c, expr->as.text_op.arg, "I can only replace text with text", NULL);
        require_text(c, expr->as.text_op.arg2, "I can only replace text with text", NULL);
        break;
    default: break;
    }
    switch (op) {
    case TEXT_CHARACTERS:
    case TEXT_WORDS:
    case TEXT_LINES:
    case TEXT_SPLIT: return types_new(c, type_list_of(TYPE_TEXT), line);
    case TEXT_CODE: return types_new(c, TYPE_NUMBER, line);
    case TEXT_IS_LETTER:
    case TEXT_IS_DIGIT:
    case TEXT_IS_SPACE: return types_new(c, TYPE_YESNO, line);
    default: return types_new(c, TYPE_TEXT, line);
    }
}
