// Questions about a list (EXPR_AGGREGATE): "how many", "any", "every", "the
// total of", "the top 3", "the most common", "with the highest", "per". The
// list is checked first; its item is "it" in the test, the key and the group.

#include <string.h>
#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

static const char *const list_messages[] = {
    "I can only count the items of a list",
    "I can only test the items of a list",
    "I can only test the items of a list",
    "I can only test the items of a list",
    "I can only add up the items of a list",
    "I can only find the average of a list",
    "I can only find the middle of a list",
    "I can only pick the top items of a list",
    "I can only find the most common items of a list",
    "I can only pick an item of a list",
};

// "any score" with no list called "scores".
static bool report_no_plural(Checker *c, const Expr *expr) {
    const Expr *list = expr->as.aggregate.list;
    if (checker_find_made(c, list->as.name) || checker_find_constant(c, list->as.name)) return false;
    const Name *item = &expr->as.aggregate.item;
    static const char *const words[] = {"", "any", "every", "no"};
    AggregateKind kind = expr->as.aggregate.kind;
    const char *phrase = kind == AGG_BEST ? arena_sprintf(c->arena, "the %s with ...", item->display)
                                          : arena_sprintf(c->arena, "%s %s", words[kind], item->display);
    diag_error(c->diag, item->pos.span, "There's no list called \"%s\" for \"%s\" to look through.", list->display,
               phrase);
    const char *example = "groceries";  // a list the program has, if it has one
    for (size_t i = c->made.len; i-- > c->floor;) {
        if (type_is_list(types_of(c, c->made.items[i].var))) example = c->made.items[i].display;
    }
    if (kind == AGG_BEST) {
        diag_note(c->diag, "\"the %s with ...\" looks through the list called \"%s\". Make that list first, or say "
                           "which list, like \"the %s in %s with ...\".",
                  item->display, list->display, item->display, example);
    } else {
        diag_note(c->diag, "\"%s\" looks through the list called \"%s\". Make that list first, or say which list, "
                           "like \"%s in %s ...\".",
                  phrase, list->display, phrase, example);
    }
    diag_info(c->diag, DIAG_MISSING, "the list to look through", "a list");
    for (size_t i = c->floor; i < c->made.len; i++) {  // the lists it could mean
        if (type_is_list(types_of(c, c->made.items[i].var))) {
            diag_choice(c->diag, "%s in %s", phrase, c->made.items[i].display);
        }
    }
    return true;
}

// "the total of order" where order is a record: the phrase adds up a list,
// and a field with the same name is written with "'s".
static TypeVar require_list(Checker *c, const Expr *expr, TypeVar list) {
    const Expr *at = expr->as.aggregate.list;
    Type type = types_of(c, list);
    TypeVar item = checker_require_list(c, list, at, list_messages[expr->as.aggregate.kind]);
    if (types_of(c, item) != TYPE_ERROR || type == TYPE_ERROR || type_is_list(type)) return item;
    static const char *const words[] = {"number", NULL, NULL, NULL, "total", "average", "median"};
    AggregateKind kind = expr->as.aggregate.kind;
    const char *word = kind < sizeof words / sizeof words[0] ? words[kind] : NULL;
    if (word && checker_field_owner(c, word) && at->kind == EXPR_NAME) {
        diag_note(c->diag, "For a field called \"%s\", write \"%s's %s\".", word, at->display, word);
    }
    return item;
}

// What the key has to be, for the kinds that use it.
static void check_key(Checker *c, const Expr *expr, TypeVar list, TypeVar item, TypeVar key) {
    const Expr *at = expr->as.aggregate.key ? expr->as.aggregate.key : expr->as.aggregate.list;
    bool own = expr->as.aggregate.key == NULL;  // the items themselves
    switch (expr->as.aggregate.kind) {
    case AGG_TOTAL:
    case AGG_AVERAGE:
    case AGG_MEDIAN: {
        static const char *const verbs[] = {"add up", "find the average of", "find the middle of"};
        const char *verb = verbs[expr->as.aggregate.kind - AGG_TOTAL];
        if (own) {
            checker_require(c, list, type_list_of(TYPE_NUMBER), at, arena_sprintf(c->arena, "I can only %s a list "
                            "of numbers", verb), NULL);
        } else {
            checker_require(c, key, TYPE_NUMBER, at, arena_sprintf(c->arena, "I can only %s numbers", verb), NULL);
        }
        return;
    }
    case AGG_TOP:
        types_defer_one_of(c, at, key, own ? list : key, KIND_NUMBER | KIND_DATE,
                           own ? "The top items are picked from a list of numbers or dates"
                               : "The top items are picked by a number or a date");
        return;
    case AGG_MOST_COMMON:
        types_defer_one_of(c, at, item, list, KIND_NUMBER | KIND_TEXT,
                           "I can only count how often numbers or text come up");
        return;
    case AGG_BEST:
        types_defer_one_of(c, at, key, key, KIND_NUMBER | KIND_DATE | KIND_LIST,
                           "The one picked has to be the highest by a number, a date, or how many items a list has");
        return;
    default: return;
    }
}

static TypeVar result_of(Checker *c, const Expr *expr, TypeVar item) {
    size_t line = expr->pos.line;
    switch (expr->as.aggregate.kind) {
    case AGG_COUNT:
    case AGG_TOTAL:
    case AGG_AVERAGE:
    case AGG_MEDIAN: return types_new(c, TYPE_NUMBER, line);
    case AGG_ANY:
    case AGG_ALL:
    case AGG_NONE: return types_new(c, TYPE_YESNO, line);
    case AGG_TOP:
    case AGG_MOST_COMMON: return types_new_list(c, item, line);
    case AGG_BEST: return item;
    }
    return error_var(c);
}

// "per month" without "of FIELD": the item's one date field.
static bool find_date_field(Checker *c, const Expr *group, TypeVar item) {
    Expr *date = group->as.library.a;
    if (date->as.field.name.text) return true;
    Type type = types_of(c, item);
    const char *period = ast_date_part_name(group->as.library.part);
    const Field *found = NULL;
    size_t count = 0;
    if (type_is_record(type) && !type_is_maybe(type)) {
        Stmt *def = c->records->items[type_record_index(type)].definition;
        for (size_t i = 0; i < def->as.record.fields.len; i++) {
            Field *field = &def->as.record.fields.items[i];
            if (types_of(c, checker_field_kind(c, field, group->pos.line)) != TYPE_DATE) continue;
            if (count++ == 0) found = field;
        }
    }
    if (count == 0 && type_is_record(type) && c->records->items[type_record_index(type)].table) {
        // a table's rows: the column called "date" (added if the program hasn't used it yet)
        Stmt *def = c->records->items[type_record_index(type)].definition;
        Field *column = NULL;
        for (size_t i = 0; i < def->as.record.fields.len && !column; i++) {
            if (strcmp(def->as.record.fields.items[i].name.text, "date") == 0) column = &def->as.record.fields.items[i];
        }
        if (!column) {
            Field added = {{"date", "date", group->pos}, TYPE_DATE, NULL, group->pos, {0}, {0}};
            column = checker_add_column(c, def, added);
        }
        types_require(c, checker_field_kind(c, column, group->pos.line), TYPE_DATE, group->pos.line);
        found = column, count = 1;
    }
    if (count == 1) {
        date->as.field.name.text = found->name.text;
        date->as.field.name.display = found->name.display;
        return true;
    }
    if (count == 0) {
        diag_error(c->diag, group->pos.span, "\"per %s\" needs a date to group by, but %s has no date in it.", period,
                   checker_kind(c, item));
        diag_note(c->diag, "Give the items a date, like \"an order has a day (a date)\".");
        diag_info(c->diag, DIAG_MISSING, "a date to group by", "a date");
        return false;
    }
    diag_error(c->diag, group->pos.span, "\"per %s\" could mean more than one date here.", period);
    diag_note(c->diag, "Say which, like \"per %s of %s\".", period, found->name.display);
    diag_info(c->diag, DIAG_TWO_MEANINGS, "which date to group by", "a date");
    Stmt *def = c->records->items[type_record_index(type)].definition;
    for (size_t i = 0; i < def->as.record.fields.len; i++) {
        Field *field = &def->as.record.fields.items[i];
        if (types_of(c, checker_field_kind(c, field, group->pos.line)) == TYPE_DATE) {
            diag_choice(c->diag, "per %s of %s", period, field->name.display);
        }
    }
    return false;
}

TypeVar checker_check_aggregate(Checker *c, const Expr *expr) {
    if (expr->as.aggregate.implied && report_no_plural(c, expr)) return error_var(c);
    const Expr *count = expr->as.aggregate.count;
    if (count) {
        checker_require(c, checker_check_expr(c, count), TYPE_NUMBER, count, "How many to take has to be a number",
                        NULL);
    }
    TypeVar list = checker_check_expr(c, expr->as.aggregate.list);
    TypeVar item = require_list(c, expr, list);
    if (types_of(c, item) == TYPE_ERROR) return error_var(c);
    size_t outer = c->made.len;
    Loop loop = {true, item, false, NO_VARIANT, true};
    vec_push(c->arena, &c->loops, loop);
    const Expr *condition = expr->as.aggregate.condition;
    if (condition) {
        checker_require(c, checker_check_expr(c, condition), TYPE_YESNO, condition, "A test has to be yes or no",
                        "Compare it with something, like \"any score is above 90\".");
    }
    TypeVar key = expr->as.aggregate.key ? checker_check_expr(c, expr->as.aggregate.key) : item;
    const Expr *group = expr->as.aggregate.group;
    bool period = group && group->kind == EXPR_LIBRARY && group->as.library.op == LIB_PERIOD_KEY;
    if (period && !find_date_field(c, group, item)) {
        c->loops.len--;
        c->made.len = outer;
        return error_var(c);
    }
    TypeVar group_var = group ? checker_check_expr(c, group) : 0;
    c->loops.len--;
    c->made.len = outer;
    check_key(c, expr, list, item, key);
    TypeVar result = result_of(c, expr, item);
    if (!group) return result;
    types_defer_one_of(c, group, group_var, group_var, KIND_TEXT | KIND_NUMBER,
                       "Things can only be grouped by text or a number");
    return types_new_map(c, group_var, result, expr->pos.line);
}
