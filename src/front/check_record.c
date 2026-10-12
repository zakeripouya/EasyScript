// Checking records: their definitions (fields, starting values), new
// records and fields. Values that can be nothing, and checking them, are in
// check_narrow.c.

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

const char *checker_capitalized(Checker *c, Type type) {
    char *name = (char *)arena_strdup(c->arena, checker_type_name(c, type));
    if (name[0] >= 'a' && name[0] <= 'z') name[0] = (char)(name[0] - 'a' + 'A');
    return name;
}

const RecordKind *checker_field_owner(const Checker *c, const char *name) {
    size_t index;
    for (size_t i = 0; i < c->records->len; i++) {
        if (find_field(c, type_record(i), name, &index)) return &c->records->items[i];
    }
    Type sum = checker_sum_with_field(c, name);
    return type_is_sum(sum) ? &c->records->sums.items[type_sum_index(sum)] : NULL;
}

// --- Fields ---------------------------------------------------------------------------

static void report_no_field(Checker *c, const Name *name, Type type) {
    if (!type) {
        diag_error(c->diag, name->pos.span, "No kind of record has a field called \"%s\".", name->display);
    } else {
        diag_error(c->diag, name->pos.span, "%s doesn't have a field called \"%s\".", checker_capitalized(c, type_present(type)),
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
    if (type_is_sum(type)) return checker_sum_field(c, expr, base, type);
    if (type_is_iface(type)) {
        diag_error(c->diag, record->pos.span, "%s has no fields, only ways.", checker_capitalized(c, type_present(type)));
        diag_note(c->diag, "Use one of its ways instead, like a method: \"speak pet\".");
        return error_var(c);
    }
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
    return checker_field_kind(c, (Field *)field, expr->pos.line);
}

// A field's kind as a variable: written down, or worked out from use (one
// variable for the field, shared by every use).
TypeVar checker_field_kind(Checker *c, Field *field, size_t line) {
    if (field->type != TYPE_UNKNOWN) return types_new(c, field->type, line);
    for (size_t i = 0; i < c->field_kinds.len; i++) {
        if (c->field_kinds.items[i].field == field) return c->field_kinds.items[i].var;
    }
    FieldKind kind = {field, types_new(c, TYPE_UNKNOWN, field->pos.line)};
    vec_push(c->arena, &c->field_kinds, kind);
    TypedSlot slot = {&field->type, kind.var};
    vec_push(c->arena, &c->slots, slot);
    return kind.var;
}

// Fields whose kind nothing decided (reported only when there's no other mistake).
void checker_report_undecided_fields(Checker *c) {
    for (size_t i = 0; i < c->field_kinds.len; i++) {
        const FieldKind *kind = &c->field_kinds.items[i];
        Type type = types_of(c, kind->var);
        bool nothing = type == TYPE_NOTHING || type == TYPE_MAYBE;
        if (!nothing && !ast_type_undecided(c->records, type)) continue;
        const Field *field = kind->field;
        diag_error(c->diag, field->name.pos.span, "I can't tell what kind of value \"%s\" holds.", field->name.display);
        if (nothing) {
            diag_note(c->diag, "It's only ever given nothing. Say its kind in parentheses, like \"%s (a dog or "
                               "nothing)\".", field->name.display);
        } else {
            diag_note(c->diag, "Say it in parentheses after the field, like \"%s (a number)\" or \"%s (text)\".",
                      field->name.display, field->name.display);
        }
    }
}

TypeVar checker_check_field(Checker *c, const Expr *expr) {
    TypeVar base = checker_check_expr(c, expr->as.field.record);
    Type type = types_of(c, base);
    if (type == TYPE_ERROR) return error_var(c);
    if (type == TYPE_DATE) return checker_date_field(c, expr, base);  // today's year
    if (type == TYPE_UNKNOWN || type == TYPE_MAYBE) {  // the only kind of record with this field, if there's one
        size_t owners = 0, index;
        Type owner = checker_sum_with_field(c, expr->as.field.name.text);
        owners += owner == TYPE_ERROR ? 2 : owner != TYPE_UNKNOWN;
        for (size_t i = 0; i < c->records->len; i++) {
            if (find_field(c, type_record(i), expr->as.field.name.text, &index)) owners++, owner = type_record(i);
        }
        if (owners == 0 && type == TYPE_UNKNOWN && checker_date_part(expr->as.field.name.text) >= 0) {
            types_require(c, base, TYPE_DATE, expr->pos.line);  // "d's year": d is a date
            return checker_date_field(c, expr, base);
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
        types_require(c, base, owner + (type == TYPE_MAYBE ? TYPE_MAYBE : 0), expr->pos.line);
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
    if (type_is_sum(type)) return checker_check_new_variant(c, expr);
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
        TypeVar kind = checker_field_kind(c, (Field *)field, field->pos.line);
        if (!checker_fits(c, kind, value, v->value, v->value->pos.line)) {
            diag_error(c->diag, v->value->pos.span, "The field \"%s\" of %s is %s, but this is %s.", field->name.display,
                       checker_type_name(c, type), checker_kind(c, kind), checker_kind(c, value));
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

static void check_default(Checker *c, Field *field) {
    const Expr *initial = field->initial;
    if (initial->kind == EXPR_NOTHING) {
        if (type_is_maybe(field->type)) return;  // it starts as nothing anyway
        diag_error(c->diag, initial->pos.span, "Only a field that can be nothing can start at nothing.");
        diag_note(c->diag, "A field whose kind is like \"a dog or nothing\" can be nothing (and starts as nothing).");
        return;
    }
    if (type_is_sum(field->type)) {
        diag_error(c->diag, initial->pos.span, "A field that holds %s can't have a starting value, because its "
                                               "values are made while the program runs.",
                   checker_type_name(c, type_present(field->type)));
        diag_note(c->diag, "Leave out \"starting at\" and give it a value when you make a new record, after \"with\".");
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
    } else if (initial->kind == EXPR_EMPTY_MAP) {  // any empty map too
        field->folded.kind = CONST_MAP;
        field->folded.key_type = ast_map_key(c->records, initial->as.list_type);
        field->folded.value_type = ast_map_value(c->records, initial->as.list_type);
        field->folded.text = "";
    } else {
        size_t before = diag_count(c->diag);
        c->explained_default = false;
        if (!const_eval(c->arena, c->diag, c->records, initial, lookup_default, c, &field->folded)) {
            if (diag_count(c->diag) > before && !c->explained_default) {
                diag_note(c->diag, "A field's starting value is worked out the same way.");
            }
            return;
        }
    }
    Type type = checker_const_type(c, &field->folded);
    if (field->type == TYPE_UNKNOWN) field->type = type;  // "an age (starting at 0)": a number
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
                   c->function ? "a definition" : "an \"if\" or a loop");
        diag_note(c->diag, stmt->as.record.has ? "Move \"%s has ...\" to the start of a line, outside every block."
                                               : "Move \"%s is a kind of thing with:\" to the start of a line, outside every block.",
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
                       checker_capitalized(c, self), field->name.display, stmt->as.record.fields.items[j].pos.line);
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
        if (field->type == TYPE_UNKNOWN) checker_field_kind(c, field, field->pos.line);  // reported if nothing decides it
    }
}
