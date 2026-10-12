// Checking lists: a list holds one kind of item, so every list value has a
// type like "a list of numbers", and every sentence that puts something into
// a list checks that it's the right kind.

#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

TypeVar checker_require_list(Checker *c, TypeVar var, const Expr *at, const char *message) {
    TypeVar item = types_new(c, TYPE_UNKNOWN, 0);
    if (!types_unify(c, var, types_new_list(c, item, at->pos.line))) {
        diag_error(c->diag, at->pos.span, "%s, but this is %s.", message, checker_kind(c, var));
        return error_var(c);
    }
    return types_of(c, var) == TYPE_ERROR ? error_var(c) : item;
}

// The value going into (or looked for in) a list has to be the kind it holds:
// "\"scores\" is a list of numbers, so it can't hold text."
static void require_item(Checker *c, const Expr *list, TypeVar list_var, TypeVar item, const Expr *value,
                         TypeVar value_var, bool contain) {
    if (!contain && checker_iface_fits(c, item, value_var, value)) return;
    if (types_unify(c, item, value_var)) return;
    const char *subject = list->kind == EXPR_NAME ? arena_sprintf(c->arena, "\"%s\"", list->display) : "This";
    if (contain) {
        diag_error(c->diag, value->pos.span, "%s is %s, so it can't contain %s.", subject, checker_kind(c, list_var),
                   checker_kind(c, value_var));
    } else {
        diag_error(c->diag, value->pos.span, "%s is %s, so it can't hold %s.", subject, checker_kind(c, list_var),
                   checker_kind(c, value_var));
    }
    diag_note(c->diag, "A list holds one kind of value. To keep %s too, make a separate list for it.",
              checker_kind(c, value_var));
}

static void require_position(Checker *c, const Expr *position) {
    if (!position) return;
    checker_require(c, checker_check_expr(c, position), TYPE_NUMBER, position,
                    "The position of an item has to be a number", NULL);
}

// --- Values --------------------------------------------------------------------------

static TypeVar check_literal(Checker *c, const Expr *expr) {
    TypeVar first = checker_check_expr(c, expr->as.items.items[0]);
    for (size_t i = 1; i < expr->as.items.len; i++) {
        const Expr *item = expr->as.items.items[i];
        TypeVar var = checker_check_expr(c, item);
        if (!types_unify(c, first, var)) {
            diag_error(c->diag, item->pos.span,
                       "All the items of a list have to be the same kind: the first is %s, but this is %s.",
                       checker_kind(c, first), checker_kind(c, var));
            Type shared = checker_shared_interface(c, types_of(c, first), types_of(c, var));
            if (shared != TYPE_UNKNOWN) {
                diag_note(c->diag, "Both can be used as %s. To keep them in one list, start with \"an empty list of "
                                   "%s\" and add each one to its end.", checker_type_name(c, shared),
                          c->records->interfaces.items[type_iface_index(shared)].plural);
            }
        }
    }
    return types_new_list(c, first, expr->pos.line);
}

// The item's name (or "it") exists only while `body` is checked.
static TypeVar check_for_each_item(Checker *c, const Name *name, TypeVar item, const Expr *body) {
    size_t outer = c->made.len;
    if (name) checker_make_item_name(c, name, item);
    Loop loop = {true, item, false, NO_VARIANT, true};
    vec_push(c->arena, &c->loops, loop);
    TypeVar result = checker_check_expr(c, body);
    c->loops.len--;
    c->made.len = outer;
    return result;
}

static TypeVar check_list_op(Checker *c, const Expr *expr) {
    const Expr *operand = expr->as.list_op.operand;
    TypeVar var = checker_check_expr(c, operand);
    size_t line = expr->pos.line;
    switch (expr->as.list_op.op) {
    case LIST_SUM:
        checker_require(c, var, type_list_of(TYPE_NUMBER), operand, "I can only add together a list of numbers", NULL);
        return types_new(c, TYPE_NUMBER, line);
    case LIST_BIGGEST:
    case LIST_SMALLEST: {
        const char *message = expr->as.list_op.op == LIST_BIGGEST
                                  ? "I can only find the biggest item of a list of numbers or text"
                                  : "I can only find the smallest item of a list of numbers or text";
        TypeVar item = checker_require_list(c, var, operand, message);
        types_defer_one_of(c, operand, item, var, KIND_NUMBER | KIND_TEXT | KIND_DATE, message);
        return item;
    }
    case LIST_REVERSED: checker_require_list(c, var, operand, "I can only reverse a list"); return var;
    case LIST_SORTED:
    case LIST_SORTED_DOWN:
    case LIST_SORTED_LENGTH:
    case LIST_SORTED_LENGTH_DOWN: {
        bool length = expr->as.list_op.op >= LIST_SORTED_LENGTH;
        if (!length && type_is_map(types_of(c, var))) {  // a map, in the order of its values
            types_defer_one_of(c, operand, types_item(c, var), var, KIND_NUMBER | KIND_TEXT | KIND_DATE,
                               "I can only sort a map whose values are numbers or text");
            return var;
        }
        const char *message = length ? "I can only sort a list of text or lists by their length"
                                     : "I can only sort a list of numbers or text";
        TypeVar item = checker_require_list(c, var, operand, message);
        types_defer_one_of(c, operand, item, var, length ? KIND_TEXT | KIND_LIST : KIND_NUMBER | KIND_TEXT | KIND_DATE, message);
        return var;
    }
    case LIST_COPY: {  // a record, map or kind with variants when it's known to be one, otherwise a list
        Type kind = types_of(c, var);
        if (type_is_map(kind) || ((type_is_record(kind) || type_is_sum(kind)) && !type_is_maybe(kind))) {
            return var;
        }
        checker_require_list(c, var, operand, "I can only copy a list, a map or a record");
        return var;
    }
    case LIST_IS_EMPTY:
    case LIST_NOT_EMPTY:
        types_defer_one_of(c, operand, var, var, KIND_TEXT | KIND_LIST | KIND_MAP,
                           "I can only check whether text, a list or a map is empty");
        return types_new(c, TYPE_YESNO, line);
    case LIST_ENTRIES:
    case LIST_KEYS:
    case LIST_VALUES: return checker_check_map_op(c, expr, var);
    }
    return error_var(c);
}

// A list form used on text ("item 1 of word"): says how to write it for text.
static TypeVar require_list_not_text(Checker *c, TypeVar var, const Expr *at, const char *message, bool one) {
    TypeVar item = checker_require_list(c, var, at, message);
    if (types_of(c, var) == TYPE_TEXT) {
        diag_note(c->diag, "To take %s of text, write it with \"%s\", like \"%s\".", one ? "a character" : "characters",
                  one ? "character" : "characters", one ? "character 1 of word" : "characters 2 to 4 of word");
    }
    return item;
}

TypeVar checker_check_list_expr(Checker *c, const Expr *expr) {
    if ((expr->kind == EXPR_ITEM && expr->as.item.characters) || (expr->kind == EXPR_SLICE && expr->as.slice.characters)) {
        return checker_check_characters(c, expr);
    }
    switch (expr->kind) {
    case EXPR_LIST: return check_literal(c, expr);
    case EXPR_EMPTY_LIST: return types_new(c, expr->as.list_type, expr->pos.line);
    case EXPR_ITEM:
        require_position(c, expr->as.item.position);
        return require_list_not_text(c, checker_check_expr(c, expr->as.item.list), expr->as.item.list,
                                     "I can only take an item from a list", true);
    case EXPR_LIST_OP: return check_list_op(c, expr);
    case EXPR_EVERY: {
        const Expr *list = expr->as.every.list;
        TypeVar item = checker_require_list(c, checker_check_expr(c, list), list, "\"every\" needs a list to go through");
        TypeVar body = check_for_each_item(c, &expr->as.every.var, item, expr->as.every.body);
        return types_new_list(c, body, expr->pos.line);
    }
    case EXPR_SLICE: {
        const Expr *from = expr->as.slice.from;
        if (expr->as.slice.kind == SLICE_WITHOUT_AT) {  // written list first
            TypeVar list = checker_check_expr(c, expr->as.slice.list);
            require_list_not_text(c, list, expr->as.slice.list, "I can only take items from a list", false);
            require_position(c, from);
            return list;
        }
        if (expr->as.slice.kind == SLICE_RANGE) {
            require_position(c, from);
            require_position(c, expr->as.slice.to);
        } else if (from) {
            checker_require(c, checker_check_expr(c, from), TYPE_NUMBER, from, "The number of items has to be a number",
                            NULL);
        }
        TypeVar list = checker_check_expr(c, expr->as.slice.list);
        require_list_not_text(c, list, expr->as.slice.list, "I can only take items from a list", false);
        return list;
    }
    case EXPR_ONLY: {
        const Expr *list = expr->as.only.list;
        TypeVar var = checker_check_expr(c, list);
        TypeVar item = checker_require_list(c, var, list, "I can only choose items from a list");
        check_for_each_item(c, NULL, item, expr->as.only.condition);
        return var;
    }
    default: return error_var(c);
    }
}

void checker_require_item(Checker *c, const Expr *list, TypeVar list_var, const Expr *value, TypeVar value_var) {
    require_item(c, list, list_var, types_item(c, list_var), value, value_var, true);
}

// --- Sentences -----------------------------------------------------------------------

// "add E to X" where X is a list: adding to its end is the only meaning
// unless it holds numbers (then "add 5 to scores" could also mean adding 5
// to each score), so that's an error that shows both ways to say it.
void checker_add_to_list(Checker *c, Stmt *stmt, TypeVar list, TypeVar amount) {
    TypeVar item = types_item(c, list);
    Type kind = types_of(c, item);
    if (kind == TYPE_ERROR) return;
    const Expr *field = stmt->as.change.field;
    if (field && field->kind == EXPR_MAP_GET) {  // "add 5 to the value for K in M", the value a list
        if (kind == TYPE_NUMBER || kind == TYPE_UNKNOWN) {
            diag_error(c->diag, stmt->pos.span, "This value is %s, so this could mean two things.", checker_kind(c, list));
            diag_note(c->diag, "To put it at the end of the list, write \"add ... to the end of the value for ...\". To "
                               "add it to every item, set the value to \"every n in ... plus ...\".");
            return;
        }
        stmt->as.change.append = true;
        require_item(c, field, list, item, stmt->as.change.amount, amount, false);
        return;
    }
    const char *name = field ? arena_sprintf(c->arena, "%s's %s", checker_path(c, field->as.field.record)
                                                                     ? checker_path(c, field->as.field.record) : "...",
                                             field->as.field.name.display)
                             : stmt->as.change.target.display;
    if (kind == TYPE_NUMBER || kind == TYPE_UNKNOWN) {
        diag_error(c->diag, stmt->pos.span, "\"%s\" is %s, so this could mean two things.", name,
                   checker_kind(c, list));
        diag_note(c->diag, "To put it at the end of the list, write \"add ... to the end of %s\". To add it to every "
                           "item, write \"set %s to every n in %s plus ...\".", name, name, name);
        return;
    }
    stmt->as.change.append = true;
    Expr target = {0};  // only for the message
    target.kind = EXPR_NAME;
    target.display = name;
    require_item(c, &target, list, item, stmt->as.change.amount, amount, false);
}

// A fixed list or map ("keep weekdays as a list of ...") can't change.
bool checker_reject_fixed(Checker *c, const Expr *list) {
    if (list->kind != EXPR_NAME || checker_find_made(c, list->as.name)) return false;
    const Constant *k = checker_find_constant(c, list->as.name);
    if (!k) return false;
    diag_error(c->diag, list->pos.span, "\"%s\" is a constant, so it can't change.", k->display);
    diag_note(c->diag, "It's kept on line %zu. If it needs to change, make it with \"let %s be ...\" instead of "
                       "\"keep\".", k->definition->pos.line, k->display);
    return true;
}

void checker_check_list_stmt(Checker *c, const Stmt *stmt) {
    if (stmt->as.list.op == LIST_REMOVE_KEY) {
        checker_check_remove_key(c, stmt);
        return;
    }
    if (stmt->as.list.op == LIST_APPLY || stmt->as.list.op == LIST_KEEP || stmt->as.list.op == LIST_SORT_BY) {
        checker_check_replace(c, stmt);
        return;
    }
    const Expr *list = stmt->as.list.list;
    if (checker_reject_fixed(c, list)) return;
    const Expr *value = stmt->as.list.value;
    TypeVar value_var = 0;
    if (value && stmt->as.list.op != LIST_SET_ITEM) value_var = checker_check_expr(c, value);  // written first
    require_position(c, stmt->as.list.position);
    TypeVar list_var = checker_check_expr(c, list);
    if (value && stmt->as.list.op == LIST_SET_ITEM) value_var = checker_check_expr(c, value);  // written last
    const char *message = NULL;
    switch (stmt->as.list.op) {
    case LIST_ADD_END: message = "I can only add to the end of a list"; break;
    case LIST_INSERT: message = "I can only insert into a list"; break;
    case LIST_REMOVE: message = "I can only remove an item from a list"; break;
    case LIST_SET_ITEM: message = "I can only change an item of a list"; break;
    case LIST_SORT:
        message = stmt->as.list.by_length ? "I can only sort a list of text or lists by their length"
                                          : "I can only sort a list of numbers or text";
        break;
    case LIST_REMOVE_KEY:
    case LIST_APPLY:
    case LIST_KEEP:
    case LIST_SORT_BY: break;  // checked above
    }
    if (stmt->as.list.op == LIST_SORT && !stmt->as.list.by_length && type_is_map(types_of(c, list_var))) {
        if (checker_reject_fixed(c, list)) return;
        types_defer_one_of(c, list, types_item(c, list_var), list_var, KIND_NUMBER | KIND_TEXT | KIND_DATE,
                           "I can only sort a map whose values are numbers or text");
        return;
    }
    TypeVar item = checker_require_list(c, list_var, list, message);
    if (types_of(c, item) == TYPE_ERROR) return;
    if (stmt->as.list.op == LIST_SORT) {
        types_defer_one_of(c, list, item, list_var, stmt->as.list.by_length ? KIND_TEXT | KIND_LIST : KIND_NUMBER | KIND_TEXT | KIND_DATE,
                           message);
    } else if (value) {
        require_item(c, list, list_var, item, value, value_var, false);
    }
}
