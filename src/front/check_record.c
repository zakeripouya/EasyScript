// Checking records: their definitions (fields, starting values), new
// records, fields, and values that can be nothing.
//
// A field whose kind is "a node or nothing" can't be used as a node until the
// program checks it: inside "if the node's next is not nothing:" (or a
// "while" loop like that, or after "if node is nothing, give back 0."), the
// name or field checked (its path, like "node's next") is the record it
// holds, until something sets it. Generated code still checks it while the
// program runs (another name for the same record could have changed it), so
// a program can never use nothing as a record.

#include <string.h>
#include "front/check_internal.h"
#include "front/consteval.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

static const Stmt *definition(const Checker *c, Type type) {
    return c->records->items[type_record_index(type)].definition;
}

static const Field *find_field(const Checker *c, Type type, const char *name, size_t *index) {
    const Stmt *def = definition(c, type);
    for (size_t i = 0; def && i < def->as.record.fields.len; i++) {
        if (strcmp(def->as.record.fields.items[i].name.text, name) == 0) {
            *index = i;
            return &def->as.record.fields.items[i];
        }
    }
    return NULL;
}

// "A dog", for the start of a sentence.
static const char *capitalized(Checker *c, Type type) {
    char *name = (char *)arena_strdup(c->arena, checker_type_name(c, type));
    if (name[0] >= 'a' && name[0] <= 'z') name[0] = (char)(name[0] - 'a' + 'A');
    return name;
}

const RecordKind *checker_field_owner(const Checker *c, const char *name) {
    size_t index;
    for (size_t i = 0; i < c->records->len; i++) {
        if (find_field(c, type_record(i), name, &index)) return &c->records->items[i];
    }
    return NULL;
}

// --- Paths and checks for nothing ------------------------------------------------------

static const char *path_of(Checker *c, const Expr *expr, bool display) {
    if (expr->kind == EXPR_NAME) return display ? expr->display : expr->as.name;
    if (expr->kind != EXPR_FIELD) return NULL;
    const char *record = path_of(c, expr->as.field.record, display);
    if (!record) return NULL;
    const Name *field = &expr->as.field.name;
    return arena_sprintf(c->arena, "%s's %s", record, display ? field->display : field->text);
}

const char *checker_path(Checker *c, const Expr *expr) {
    return path_of(c, expr, false);
}

void checker_report_maybe(Checker *c, const Expr *at, TypeVar var) {
    const char *path = path_of(c, at, true);
    if (path) {
        diag_error(c->diag, at->pos.span, "\"%s\" could be nothing, so check it first.", path);
        diag_note(c->diag, "Use it inside \"if %s is not nothing:\".", path);
        return;
    }
    diag_error(c->diag, at->pos.span, "This is %s, so check it first.", checker_kind(c, var));
    diag_note(c->diag, "Give it a name with \"let\", then use it inside \"if NAME is not nothing:\".");
}

static bool is_narrowed(const Checker *c, const char *path) {
    for (size_t i = 0; i < c->present.len; i++) {
        if (!c->present.items[i].killed && strcmp(c->present.items[i].path, path) == 0) return true;
    }
    return false;
}

TypeVar checker_narrow(Checker *c, const Expr *expr, TypeVar var) {
    Type type = types_of(c, var);
    if (expr == c->unnarrowed || !(type == TYPE_MAYBE || type_is_maybe(type))) return var;
    const char *path = checker_path(c, expr);
    if (!path || !is_narrowed(c, path)) return var;
    ((Expr *)expr)->narrowed = true;  // codegen still checks it while the program runs
    if (type != TYPE_MAYBE) return types_new(c, type_present(type), expr->pos.line);
    TypeVar present = types_new(c, TYPE_UNKNOWN, 0);
    types_defer(c, DEFER_PRESENT, expr, var, present, present);
    return present;
}

static void add_narrowing(Checker *c, const Expr *expr) {
    const char *path = checker_path(c, expr);
    if (!path) return;
    Narrowing n = {path, false};
    vec_push(c->arena, &c->present, n);
}

void checker_narrow_condition(Checker *c, const Expr *condition, bool when) {
    if (condition->kind == EXPR_UNARY && condition->as.unary.op == UNARY_NOT) {
        checker_narrow_condition(c, condition->as.unary.operand, !when);
        return;
    }
    if (condition->kind != EXPR_BINARY) return;
    const Expr *left = condition->as.binary.left, *right = condition->as.binary.right;
    switch (condition->as.binary.op) {
    case BINARY_AND:
        if (!when) return;
        checker_narrow_condition(c, left, true);
        checker_narrow_condition(c, right, true);
        return;
    case BINARY_OR:
        if (when) return;
        checker_narrow_condition(c, left, false);
        checker_narrow_condition(c, right, false);
        return;
    case BINARY_EQUAL:
    case BINARY_NOT_EQUAL:
        if ((condition->as.binary.op == BINARY_NOT_EQUAL) != when) return;
        if (right->kind == EXPR_NOTHING) add_narrowing(c, left);
        if (left->kind == EXPR_NOTHING) add_narrowing(c, right);
        return;
    default: return;
    }
}

void checker_kill_path(Checker *c, const Expr *target, const char *name) {
    const char *path = target ? checker_path(c, target) : name;
    if (!path) return;
    size_t len = strlen(path);
    for (size_t i = 0; i < c->present.len; i++) {
        const char *p = c->present.items[i].path;
        if (strncmp(p, path, len) == 0 && (p[len] == '\0' || strncmp(p + len, "'s ", 3) == 0)) {
            c->present.items[i].killed = true;
        }
    }
}

void checker_kill_set_in(Checker *c, const Block *block) {
    for (size_t i = 0; i < block->len; i++) {
        const Stmt *stmt = block->items[i];
        if (stmt->kind == STMT_SET) checker_kill_path(c, stmt->as.assign.field, stmt->as.assign.name.text);
        if (stmt->kind == STMT_LOOP) checker_kill_set_in(c, &stmt->as.loop.body);
        for (size_t b = 0; stmt->kind == STMT_IF && b < stmt->as.if_stmt.branches.len; b++) {
            checker_kill_set_in(c, &stmt->as.if_stmt.branches.items[b].body);
        }
    }
}

// --- Values that can be nothing ------------------------------------------------------------

TypeVar checker_check_value(Checker *c, const Expr *expr) {
    if (expr->kind != EXPR_NOTHING) return checker_check_expr(c, expr);
    TypeVar var = types_new(c, TYPE_NOTHING, expr->pos.line);
    TypedExpr typed = {(Expr *)expr, var};
    vec_push(c->arena, &c->typed, typed);
    return var;
}

bool checker_fits(Checker *c, TypeVar target, TypeVar value, size_t line) {
    Type t = types_of(c, target), v = types_of(c, value);
    if (v == TYPE_NOTHING) {
        if (t == TYPE_UNKNOWN) return types_require(c, target, TYPE_MAYBE, line);
        return t == TYPE_MAYBE || type_is_maybe(t) || t == TYPE_ERROR;
    }
    bool record = type_is_record(v) && !type_is_maybe(v);
    if (record && type_is_maybe(t) && type_present(t) == v) return true;  // a dog where a dog or nothing goes
    if (record && t == TYPE_MAYBE) return types_require(c, target, v + TYPE_MAYBE, line);
    return types_unify(c, target, value);
}

bool checker_check_nothing_test(Checker *c, const Expr *expr, TypeVar *result) {
    if (expr->kind != EXPR_BINARY || (expr->as.binary.op != BINARY_EQUAL && expr->as.binary.op != BINARY_NOT_EQUAL)) {
        return false;
    }
    const Expr *left = expr->as.binary.left, *right = expr->as.binary.right;
    if (left->kind != EXPR_NOTHING && right->kind != EXPR_NOTHING) return false;
    *result = types_new(c, TYPE_YESNO, expr->pos.line);
    const Expr *other = left->kind == EXPR_NOTHING ? right : left;
    checker_check_value(c, left->kind == EXPR_NOTHING ? left : right);
    if (other->kind == EXPR_NOTHING) {
        diag_error(c->diag, expr->pos.span, "I can't compare nothing with nothing.");
        diag_note(c->diag, "Compare a record or nothing with it, like \"if the node's next is nothing\".");
        return true;
    }
    const Expr *saved = c->unnarrowed;
    c->unnarrowed = other;  // "x is nothing" asks about x as it is, even where it was checked before
    TypeVar var = checker_check_value(c, other);
    c->unnarrowed = saved;
    Type type = types_of(c, var);
    if (type == TYPE_UNKNOWN) {
        types_require(c, var, TYPE_MAYBE, expr->pos.line);
    } else if (type_is_record(type) && !type_is_maybe(type)) {
        const char *path = path_of(c, other, true);
        diag_error(c->diag, other->pos.span, "%s is %s, so it's never nothing.",
                   path ? arena_sprintf(c->arena, "\"%s\"", path) : "This", checker_kind(c, var));
        diag_note(c->diag, "Only a value that's \"%s or nothing\" can be nothing.", checker_kind(c, var));
    } else if (type != TYPE_MAYBE && !type_is_maybe(type) && type != TYPE_ERROR) {
        diag_error(c->diag, expr->pos.span, "I can't compare %s with nothing.", checker_kind(c, var));
        diag_note(c->diag, "Only a record can be nothing: a field or input whose kind is like \"a dog or nothing\".");
    }
    return true;
}

// --- Fields ---------------------------------------------------------------------------

static void report_no_field(Checker *c, const Name *name, Type type) {
    if (!type) {
        diag_error(c->diag, name->pos.span, "No kind of record has a field called \"%s\".", name->display);
    } else {
        diag_error(c->diag, name->pos.span, "%s doesn't have a field called \"%s\".", capitalized(c, type_present(type)),
                   name->display);
    }
    const char *best = NULL;
    size_t best_distance = (strlen(name->text) <= 3 ? 1 : 2) + 1;
    for (size_t r = 0; r < c->records->len; r++) {
        const Stmt *def = c->records->items[r].definition;
        if ((type && type_record(r) != type_present(type)) || !def) continue;
        for (size_t i = 0; i < def->as.record.fields.len; i++) {
            size_t d = edit_distance(c->arena, name->text, def->as.record.fields.items[i].name.text);
            if (d < best_distance) {
                best_distance = d;
                best = def->as.record.fields.items[i].name.display;
            }
        }
    }
    if (best) {
        diag_note(c->diag, "Did you mean \"%s\"?", best);
    } else if (type) {
        const Stmt *def = definition(c, type_present(type));
        StrBuf sb;
        sb_init(&sb, c->arena);
        for (size_t i = 0; i < def->as.record.fields.len; i++) {
            if (i > 0) sb_append(&sb, i + 1 == def->as.record.fields.len ? " and " : ", ");
            sb_appendf(&sb, "\"%s\"", def->as.record.fields.items[i].name.display);
        }
        diag_note(c->diag, "Its fields are %s (line %zu).", sb.data, def->pos.line);
    } else {
        diag_note(c->diag, "Fields are listed where a kind of record is defined, like \"a name, which is text.\".");
    }
}

// The field of a record whose kind is known.
static TypeVar field_of(Checker *c, const Expr *expr, TypeVar base, Type type) {
    const Expr *record = expr->as.field.record;
    if (type_is_maybe(type)) checker_report_maybe(c, record, base);  // then goes on as if it were checked
    if (!type_is_record(type)) {
        diag_error(c->diag, record->pos.span, "Only a record has fields, but this is %s.", checker_kind(c, base));
        diag_note(c->diag, "A field is part of a record, like \"rex's name\" for a dog called rex.");
        return error_var(c);
    }
    size_t index;
    const Field *field = find_field(c, type_present(type), expr->as.field.name.text, &index);
    if (!field) {
        report_no_field(c, &expr->as.field.name, type);
        return error_var(c);
    }
    ((Expr *)expr)->as.field.index = index;
    return types_new(c, field->type, expr->pos.line);
}

TypeVar checker_check_field(Checker *c, const Expr *expr) {
    TypeVar base = checker_check_expr(c, expr->as.field.record);
    Type type = types_of(c, base);
    if (type == TYPE_ERROR) return error_var(c);
    if (type == TYPE_UNKNOWN || type == TYPE_MAYBE) {  // the only kind of record with this field, if there's one
        size_t owners = 0, owner = 0, index;
        for (size_t i = 0; i < c->records->len; i++) {
            if (find_field(c, type_record(i), expr->as.field.name.text, &index)) owners++, owner = i;
        }
        if (owners == 0) {
            report_no_field(c, &expr->as.field.name, 0);
            return error_var(c);
        }
        if (owners > 1) {
            TypeVar result = types_new(c, TYPE_UNKNOWN, 0);
            types_defer(c, DEFER_FIELD, expr, base, result, result);
            return result;
        }
        types_require(c, base, type_record(owner) + (type == TYPE_MAYBE ? TYPE_MAYBE : 0), expr->pos.line);
        type = types_of(c, base);
    }
    return field_of(c, expr, base, type);
}

bool checker_settle_field(Checker *c, const Deferred *d) {
    Type type = types_of(c, d->left);
    if (type == TYPE_UNKNOWN || type == TYPE_MAYBE) return false;
    if (type == TYPE_ERROR) return types_require(c, d->result, TYPE_ERROR, 0);
    types_unify(c, d->result, field_of(c, d->expr, d->left, type));
    return true;
}

bool checker_settle_present(Checker *c, const Deferred *d) {
    Type maybe = types_of(c, d->left), present = types_of(c, d->right);
    if (maybe == TYPE_ERROR || present == TYPE_ERROR) return true;
    if (type_is_maybe(maybe)) return types_require(c, d->right, type_present(maybe), d->expr->pos.line) || true;
    if (type_is_record(present)) return types_require(c, d->left, present + TYPE_MAYBE, d->expr->pos.line) || true;
    if (present == TYPE_UNKNOWN) return false;
    diag_error(c->diag, d->expr->pos.span, "This is a record or nothing, but it's used as %s here.",
               checker_kind(c, d->right));
    return true;
}

// --- New records -------------------------------------------------------------------------

static void report_missing(Checker *c, const Expr *expr, const bool *given) {
    Type type = expr->as.new_record.type;
    const Stmt *def = definition(c, type);
    StrBuf names;
    sb_init(&names, c->arena);
    size_t missing = 0, total = 0;
    const Field *example = NULL;
    for (size_t i = 0; i < def->as.record.fields.len; i++) {
        const Field *f = &def->as.record.fields.items[i];
        total += !given[i] && !f->initial && !type_is_maybe(f->type);
    }
    for (size_t i = 0; i < def->as.record.fields.len; i++) {
        const Field *f = &def->as.record.fields.items[i];
        if (given[i] || f->initial || type_is_maybe(f->type)) continue;
        if (missing++ > 0) sb_append(&names, missing == total ? " and " : ", ");
        sb_appendf(&names, "\"%s\"", f->name.display);
        if (!example) example = f;
    }
    if (total == 0) return;
    diag_error(c->diag, expr->pos.span, "A new %s needs a value for %s.", c->records->items[type_record_index(type)].display,
               names.data);
    diag_note(c->diag, "Give it after \"with\", like \"with %s ...\", or give the field a starting value where %s is "
                       "defined (line %zu), like \"..., starting at ...\".",
              example->name.display, checker_type_name(c, type), def->pos.line);
}

TypeVar checker_check_new(Checker *c, const Expr *expr) {
    Type type = expr->as.new_record.type;
    const Stmt *def = definition(c, type);
    bool *given = arena_alloc(c->arena, def->as.record.fields.len + 1);
    for (size_t i = 0; i < expr->as.new_record.values.len; i++) {
        FieldValue *v = (FieldValue *)&expr->as.new_record.values.items[i];
        TypeVar value = checker_check_value(c, v->value);
        size_t index;
        const Field *field = find_field(c, type, v->field.text, &index);
        if (!field) {
            report_no_field(c, &v->field, type);
            continue;
        }
        v->index = index;
        if (given[index]) {
            diag_error(c->diag, v->field.pos.span, "This gives \"%s\" a value twice.", v->field.display);
            diag_note(c->diag, "Give each field one value.");
            continue;
        }
        given[index] = true;
        if (!checker_fits(c, types_new(c, field->type, field->pos.line), value, v->value->pos.line)) {
            diag_error(c->diag, v->value->pos.span, "The field \"%s\" of %s is %s, but this is %s.", field->name.display,
                       checker_type_name(c, type), checker_type_name(c, field->type), checker_kind(c, value));
        }
    }
    report_missing(c, expr, given);
    return types_new(c, type, expr->pos.line);
}

// --- Definitions --------------------------------------------------------------------------

void checker_collect_records(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        const Stmt *stmt = program->items[i];
        if (stmt->kind != STMT_RECORD) continue;
        const Stmt *first = c->records->items[stmt->as.record.index].definition;
        if (first == stmt) continue;
        diag_error(c->diag, stmt->as.record.name.pos.span, "You already defined \"%s\" on line %zu.",
                   stmt->as.record.name.display, first->pos.line);
        diag_note(c->diag, "Each kind of record needs its own name.");
    }
}

// A name in a field's starting value: only a constant will do.
static bool lookup_default(void *context, const Expr *expr, ConstValue *out) {
    Checker *c = context;
    const Constant *k = checker_find_constant(c, expr->as.name);
    if (k) {
        *out = k->definition->as.constant.folded;
        return k->ok;
    }
    diag_error(c->diag, expr->pos.span, "\"%s\" isn't a constant, so a field can't start at it.", expr->display);
    diag_note(c->diag, "A field's starting value is fixed before the program runs: use numbers, text, yes or no, "
                       "and constants made with \"keep\".");
    c->explained_default = true;
    return false;
}

static Type folded_type(const ConstValue *v) {
    switch (v->kind) {
    case CONST_NUMBER: return TYPE_NUMBER;
    case CONST_TEXT: return TYPE_TEXT;
    case CONST_YESNO: return TYPE_YESNO;
    default: return v->list_type;
    }
}

static void check_default(Checker *c, Field *field) {
    const Expr *initial = field->initial;
    if (initial->kind == EXPR_NOTHING) {
        if (type_is_maybe(field->type)) return;  // it starts as nothing anyway
        diag_error(c->diag, initial->pos.span, "Only a field that can be nothing can start at nothing.");
        diag_note(c->diag, "A field whose kind is like \"a dog or nothing\" can be nothing (and starts as nothing).");
        return;
    }
    if (type_is_record(field->type)) {
        diag_error(c->diag, initial->pos.span, "A field that holds a record can't have a starting value, because "
                                               "records are made while the program runs.");
        diag_note(c->diag, "Leave out \"starting at\" and give it a value when you make a new record, after \"with\".");
        return;
    }
    if (initial->kind == EXPR_EMPTY_LIST) {  // any empty list will do: each record gets its own
        field->folded.kind = CONST_LIST;
        field->folded.list_type = initial->as.list_type;
        field->folded.text = "";
    } else {
        size_t before = diag_count(c->diag);
        c->explained_default = false;
        if (!const_eval(c->arena, c->diag, initial, lookup_default, c, &field->folded)) {
            if (diag_count(c->diag) > before && !c->explained_default) {
                diag_note(c->diag, "A field's starting value is worked out the same way.");
            }
            return;
        }
    }
    Type type = folded_type(&field->folded);
    if (type == field->type || (type_is_list(field->type) && type == type_list_of(TYPE_UNKNOWN))) return;
    diag_error(c->diag, initial->pos.span, "The field \"%s\" is %s, but its starting value is %s.", field->name.display,
               checker_type_name(c, field->type), checker_type_name(c, type));
}

// Whether every `from` needs a `to` inside it, through fields that hold a
// record and can't be nothing.
static bool needs(const Checker *c, Type from, Type to, size_t depth) {
    const Stmt *def = definition(c, from);
    for (size_t i = 0; def && depth <= c->records->len && i < def->as.record.fields.len; i++) {
        Type type = def->as.record.fields.items[i].type;
        if (!type_is_record(type) || type_is_maybe(type)) continue;
        if (type == to || needs(c, type, to, depth + 1)) return true;
    }
    return false;
}

void checker_check_record(Checker *c, Stmt *stmt) {
    const Name *name = &stmt->as.record.name;
    if (c->function || c->blocks > 0) {
        diag_error(c->diag, name->pos.span, "Kinds of record can only be defined at the top level, not inside %s.",
                   c->function ? "a function" : "an \"if\" or a loop");
        diag_note(c->diag, "Move \"%s is a kind of thing with:\" to the start of a line, outside every block.",
                  checker_type_name(c, type_record(stmt->as.record.index)));
        return;
    }
    if (c->records->items[stmt->as.record.index].definition != stmt) return;  // defined twice, already reported
    Type self = type_record(stmt->as.record.index);
    for (size_t i = 0; i < stmt->as.record.fields.len; i++) {
        Field *field = &stmt->as.record.fields.items[i];
        for (size_t j = 0; j < i; j++) {
            if (strcmp(stmt->as.record.fields.items[j].name.text, field->name.text) != 0) continue;
            diag_error(c->diag, field->name.pos.span, "%s already has a field called \"%s\", on line %zu.",
                       capitalized(c, self), field->name.display, stmt->as.record.fields.items[j].pos.line);
            diag_note(c->diag, "Give each field its own name.");
            break;
        }
        if (type_is_record(field->type) && !type_is_maybe(field->type) && (field->type == self || needs(c, field->type, self, 0))) {
            diag_error(c->diag, field->pos.span, "Every %s would need another %s inside it, so none could ever be made.",
                       name->display, name->display);
            diag_note(c->diag, "Let the field be nothing: write its kind like \"%s or nothing\".",
                      checker_type_name(c, field->type));
        }
        if (field->initial) check_default(c, field);
    }
}
