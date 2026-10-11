// Checking kinds with variants ("a shape is one of:"): their definitions,
// making their values ("a new circle with radius 3"), their fields, "s is a
// circle", and "check which kind s is".
//
// A value of a kind with variants is one of them, and only that one has its
// fields. So "s's radius" needs s checked to be a circle first: inside "if s
// is a circle:" (or after "let s be a new circle ..."), or inside "when it's
// a circle:" of "check which kind s is". The checks are narrowings
// (check_narrow.c), like checks for nothing. A value's fields never change:
// a different value is made instead.

#include <string.h>
#include "front/check_internal.h"

static TypeVar error_var(Checker *c) {
    return types_new(c, TYPE_ERROR, 0);
}

static const VariantKind *variant_kind(const Checker *c, size_t id) {
    return &c->records->variants.items[id];
}

// "a circle", "an ellipse"
static const char *variant_name(const Checker *c, size_t id) {
    const char *display = variant_kind(c, id)->display;
    return arena_sprintf(c->arena, "%s %s", strchr("aeiouAEIOU", display[0]) ? "an" : "a", display);
}

// "A circle", for the start of a sentence.
static const char *capital_name(const Checker *c, size_t id) {
    char *name = (char *)variant_name(c, id);
    name[0] = 'A';
    return name;
}

static const VariantDef *variant_def(const Checker *c, size_t id) {
    const Stmt *def = c->records->sums.items[variant_kind(c, id)->sum].definition;
    for (size_t i = 0; def && i < def->as.sum.variants.len; i++) {
        if (def->as.sum.variants.items[i].id == id) return &def->as.sum.variants.items[i];
    }
    return NULL;
}

static const Field *variant_field(const Checker *c, size_t id, const char *name, size_t *index) {
    const VariantDef *def = variant_def(c, id);
    for (size_t i = 0; def && i < def->fields.len; i++) {
        if (strcmp(def->fields.items[i].name.text, name) == 0) {
            *index = i;
            return &def->fields.items[i];
        }
    }
    return NULL;
}

Type checker_sum_with_field(const Checker *c, const char *name) {
    Type found = TYPE_UNKNOWN;
    size_t index;
    for (size_t v = 0; v < c->records->variants.len; v++) {
        if (!variant_field(c, v, name, &index)) continue;
        Type sum = type_sum(variant_kind(c, v)->sum);
        if (found != TYPE_UNKNOWN && found != sum) return TYPE_ERROR;
        found = sum;
    }
    return found;
}

// "a circle or a square", "a circle, a square or a point"
static const char *join_names(Checker *c, const size_t *ids, size_t n) {
    StrBuf sb;
    sb_init(&sb, c->arena);
    for (size_t i = 0; i < n; i++) {
        if (i > 0) sb_append(&sb, i + 1 == n ? " or " : ", ");
        sb_append(&sb, variant_name(c, ids[i]));
    }
    return sb.data;
}

// --- Definitions ---------------------------------------------------------------------

static void report_taken(Checker *c, const Name *name, const char *what, size_t line) {
    diag_error(c->diag, name->pos.span, "\"%s\" is already %s, on line %zu.", name->display, what, line);
    diag_note(c->diag, "Give each kind and each variant its own name, so \"a new %s\" can only mean one thing.",
              name->display);
}

static bool check_kind_name(Checker *c, const Stmt *stmt) {
    const RecordKind *self = &c->records->sums.items[stmt->as.sum.index];
    if (self->definition != stmt) {
        diag_error(c->diag, stmt->as.sum.name.pos.span, "You already defined \"%s\" on line %zu.",
                   stmt->as.sum.name.display, self->definition->pos.line);
        diag_note(c->diag, "Each kind needs its own name.");
        return false;
    }
    for (size_t r = 0; r < c->records->len; r++) {
        const RecordKind *record = &c->records->items[r];
        if (record->definition && strcmp(record->name, self->name) == 0) {
            report_taken(c, &stmt->as.sum.name, "the name of a kind of record", record->definition->pos.line);
            return false;
        }
    }
    return true;
}

static void check_variant_names(Checker *c, const VariantDef *variant) {
    if (variant->id == NO_VARIANT) return;
    const char *name = variant->name.text;
    for (size_t r = 0; r < c->records->len; r++) {
        if (c->records->items[r].definition && strcmp(c->records->items[r].name, name) == 0) {
            report_taken(c, &variant->name, "the name of a kind of record", c->records->items[r].definition->pos.line);
            return;
        }
    }
    for (size_t s = 0; s < c->records->sums.len; s++) {
        if (c->records->sums.items[s].definition && strcmp(c->records->sums.items[s].name, name) == 0) {
            report_taken(c, &variant->name, "the name of a kind", c->records->sums.items[s].definition->pos.line);
            return;
        }
    }
    for (size_t v = 0; v < variant->id; v++) {
        if (strcmp(variant_kind(c, v)->name, name) != 0) continue;
        const VariantDef *other = variant_def(c, v);
        const RecordKind *sum = &c->records->sums.items[variant_kind(c, v)->sum];
        if (other) report_taken(c, &variant->name, arena_sprintf(c->arena, "a variant of \"%s\"", sum->display),
                                other->pos.line);
        return;
    }
}

void checker_collect_sums(Checker *c, const Block *program) {
    for (size_t i = 0; i < program->len; i++) {
        const Stmt *stmt = program->items[i];
        if (stmt->kind != STMT_SUM || !check_kind_name(c, stmt)) continue;
        for (size_t v = 0; v < stmt->as.sum.variants.len; v++) check_variant_names(c, &stmt->as.sum.variants.items[v]);
    }
}

// Whether a value of the kind can ever be made: some variant has no field
// that needs a value of a kind that can't be (itself, directly or not).
static bool makeable(const Checker *c, size_t sum, const bool *done) {
    const Stmt *def = c->records->sums.items[sum].definition;
    for (size_t v = 0; def && v < def->as.sum.variants.len; v++) {
        bool ok = true;
        const VariantDef *variant = &def->as.sum.variants.items[v];
        for (size_t f = 0; ok && f < variant->fields.len; f++) {
            Type type = variant->fields.items[f].type;
            ok = !type_is_sum(type) || type_is_maybe(type) || done[type_sum_index(type)];
        }
        if (ok) return true;
    }
    return !def;
}

static bool ever_made(const Checker *c, size_t sum) {
    size_t n = c->records->sums.len;
    bool *done = arena_alloc(c->arena, n + 1);
    for (bool progress = true; progress;) {
        progress = false;
        for (size_t i = 0; i < n; i++) {
            if (!done[i] && makeable(c, i, done)) done[i] = progress = true;
        }
    }
    return done[sum];
}

void checker_check_sum(Checker *c, const Stmt *stmt) {
    const Name *name = &stmt->as.sum.name;
    if (c->function || c->blocks > 0) {
        diag_error(c->diag, name->pos.span, "Kinds can only be defined at the top level, not inside %s.",
                   c->function ? "a function" : "an \"if\" or a loop");
        diag_note(c->diag, "Move \"%s is one of:\" to the start of a line, outside every block.",
                  checker_type_name(c, type_sum(stmt->as.sum.index)));
        return;
    }
    if (c->records->sums.items[stmt->as.sum.index].definition != stmt) return;  // reported already
    for (size_t v = 0; v < stmt->as.sum.variants.len; v++) {
        const VariantDef *variant = &stmt->as.sum.variants.items[v];
        for (size_t i = 0; i < variant->fields.len; i++) {
            const Field *field = &variant->fields.items[i];
            for (size_t j = 0; j < i; j++) {
                if (strcmp(variant->fields.items[j].name.text, field->name.text) != 0) continue;
                diag_error(c->diag, field->name.pos.span, "\"%s\" already has a field called \"%s\".",
                           variant->name.display, field->name.display);
                diag_note(c->diag, "Give each field its own name.");
                break;
            }
        }
    }
    if (stmt->as.sum.variants.len > 0 && !ever_made(c, stmt->as.sum.index)) {
        diag_error(c->diag, name->pos.span, "Every variant of \"%s\" needs another %s inside it, so none could ever "
                                            "be made.", name->display, name->display);
        diag_note(c->diag, "Add a variant that doesn't hold one (a variant can have no fields at all), or let a field "
                           "be nothing: write its kind like \"(%s or nothing)\".",
                  checker_type_name(c, type_sum(stmt->as.sum.index)));
    }
}

// --- Values ----------------------------------------------------------------------------

static void report_fields(Checker *c, size_t id) {
    const VariantDef *def = variant_def(c, id);
    if (!def || def->fields.len == 0) {
        diag_note(c->diag, "%s has no fields.", capital_name(c, id));
        return;
    }
    StrBuf sb;
    sb_init(&sb, c->arena);
    for (size_t i = 0; i < def->fields.len; i++) {
        if (i > 0) sb_append(&sb, i + 1 == def->fields.len ? " and " : ", ");
        sb_appendf(&sb, "\"%s\"", def->fields.items[i].name.display);
    }
    diag_note(c->diag, "Its fields are %s (line %zu).", sb.data, def->pos.line);
}

static void report_missing(Checker *c, const Expr *expr, const VariantDef *def, const bool *given) {
    StrBuf names;
    sb_init(&names, c->arena);
    size_t missing = 0, total = 0;
    const Field *example = NULL;
    for (size_t i = 0; i < def->fields.len; i++) total += !given[i] && !type_is_maybe(def->fields.items[i].type);
    for (size_t i = 0; i < def->fields.len; i++) {
        const Field *f = &def->fields.items[i];
        if (given[i] || type_is_maybe(f->type)) continue;
        if (missing++ > 0) sb_append(&names, missing == total ? " and " : ", ");
        sb_appendf(&names, "\"%s\"", f->name.display);
        if (!example) example = f;
    }
    if (total == 0) return;
    diag_error(c->diag, expr->pos.span, "A new %s needs a value for %s.", def->name.display, names.data);
    diag_note(c->diag, "Give it after \"with\", like \"a new %s with %s ...\".", def->name.display, example->name.display);
}

TypeVar checker_check_new_variant(Checker *c, const Expr *expr) {
    size_t id = expr->as.new_record.variant;
    Type type = expr->as.new_record.type;
    const VariantDef *def = variant_def(c, id);
    if (!def) return error_var(c);  // its kind is defined twice (reported)
    bool *given = arena_alloc(c->arena, def->fields.len + 1);
    for (size_t i = 0; i < expr->as.new_record.values.len; i++) {
        FieldValue *v = (FieldValue *)&expr->as.new_record.values.items[i];
        TypeVar value = checker_check_value(c, v->value);
        size_t index;
        const Field *field = variant_field(c, id, v->field.text, &index);
        if (!field) {
            diag_error(c->diag, v->field.pos.span, "%s doesn't have a field called \"%s\".",
                       capital_name(c, id), v->field.display);
            report_fields(c, id);
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
                       variant_name(c, id), checker_type_name(c, field->type), checker_kind(c, value));
        }
    }
    report_missing(c, expr, def, given);
    return types_new(c, type, expr->pos.line);
}

// --- Fields ----------------------------------------------------------------------------

static void report_no_sum_field(Checker *c, const Name *name, size_t sum) {
    diag_error(c->diag, name->pos.span, "%s doesn't have a field called \"%s\".",
               checker_capitalized(c, type_sum(sum)), name->display);
    const char *best = NULL;
    size_t best_distance = (strlen(name->text) <= 3 ? 1 : 2) + 1;
    for (size_t v = 0; v < c->records->variants.len; v++) {
        const VariantDef *def = variant_kind(c, v)->sum == sum ? variant_def(c, v) : NULL;
        for (size_t i = 0; def && i < def->fields.len; i++) {
            size_t d = edit_distance(c->arena, name->text, def->fields.items[i].name.text);
            if (d < best_distance) best_distance = d, best = def->fields.items[i].name.display;
        }
    }
    if (best) {
        diag_note(c->diag, "Did you mean \"%s\"?", best);
    } else {
        diag_note(c->diag, "Each variant's fields are listed where %s is defined (line %zu).",
                  checker_type_name(c, type_sum(sum)), c->records->sums.items[sum].definition->pos.line);
    }
}

// "s's radius" where s isn't checked to be a circle.
static void report_unchecked(Checker *c, const Expr *expr, const size_t *holders, size_t n) {
    const char *path = checker_path(c, expr->as.field.record);
    const char *names = join_names(c, holders, n);
    if (path) {
        const Expr *base = expr->as.field.record;
        const char *shown = base->kind == EXPR_NAME ? base->display : path;
        diag_error(c->diag, expr->pos.span, "\"%s\" might not be %s, so check which kind it is first.", shown, names);
        diag_note(c->diag, "Use it inside \"if %s is %s:\", or inside \"when it's %s:\" after \"check which kind %s "
                           "is:\".", shown, variant_name(c, holders[0]), variant_name(c, holders[0]), shown);
        return;
    }
    diag_error(c->diag, expr->pos.span, "This might not be %s, so check which kind it is first.", names);
    diag_note(c->diag, "Give it a name with \"let\", then use its fields inside \"if NAME is %s:\".",
              variant_name(c, holders[0]));
}

TypeVar checker_sum_field(Checker *c, const Expr *expr, TypeVar base, Type type) {
    (void)base;
    size_t sum = type_sum_index(type), n = 0, index;
    const char *name = expr->as.field.name.text;
    size_t *holders = arena_alloc(c->arena, (c->records->variants.len + 1) * sizeof(size_t));
    for (size_t v = 0; v < c->records->variants.len; v++) {
        if (variant_kind(c, v)->sum == sum && variant_field(c, v, name, &index)) holders[n++] = v;
    }
    if (n == 0) {
        report_no_sum_field(c, &expr->as.field.name, sum);
        return error_var(c);
    }
    if (c->setting_field) {
        diag_error(c->diag, expr->pos.span, "The fields of %s can't change.", checker_type_name(c, type_sum(sum)));
        const char *path = checker_path(c, expr->as.field.record);
        diag_note(c->diag, "Make a new one instead, like \"set %s to a new %s with %s ...\".", path ? path : "NAME",
                  variant_kind(c, holders[0])->display, expr->as.field.name.display);
        return error_var(c);
    }
    size_t variant = checker_narrowed_variant(c, expr->as.field.record);
    const Field *field = variant == NO_VARIANT ? NULL : variant_field(c, variant, name, &index);
    if (variant == NO_VARIANT) {
        report_unchecked(c, expr, holders, n);
        return types_new(c, variant_field(c, holders[0], name, &index)->type, expr->pos.line);  // as if it were
    }
    if (!field) {
        diag_error(c->diag, expr->as.field.name.pos.span, "%s doesn't have a field called \"%s\".",
                   capital_name(c, variant), expr->as.field.name.display);
        diag_note(c->diag, "Only %s has one.", join_names(c, holders, n));
        return error_var(c);
    }
    ((Expr *)expr)->as.field.variant = variant;
    ((Expr *)expr)->as.field.index = index;
    return types_new(c, field->type, expr->pos.line);
}

// --- Telling variants apart ---------------------------------------------------------

// The kind of a value whose variants are asked about: if it isn't decided
// yet, the kind the variant belongs to (or that kind or nothing).
static Type require_sum(Checker *c, TypeVar var, size_t sum, bool nothing, size_t line) {
    Type type = types_of(c, var);
    if (type == TYPE_UNKNOWN || type == TYPE_MAYBE) {
        types_require(c, var, type_sum(sum) + (nothing || type == TYPE_MAYBE ? TYPE_MAYBE : 0), line);
    }
    return types_of(c, var);
}

TypeVar checker_check_kind_test(Checker *c, const Expr *expr) {
    const Expr *operand = expr->as.kind_test.operand;
    TypeVar var = checker_check_expr(c, operand);
    size_t id = expr->as.kind_test.variant, sum = variant_kind(c, id)->sum;
    Type type = require_sum(c, var, sum, false, expr->pos.line);
    if (type != TYPE_ERROR && type_present(type) != type_sum(sum)) {
        const char *path = checker_path(c, operand);
        diag_error(c->diag, operand->pos.span, "%s is %s, so it's never %s.",
                   path ? arena_sprintf(c->arena, "\"%s\"", operand->kind == EXPR_NAME ? operand->display : path)
                        : "This",
                   checker_kind(c, var), variant_name(c, id));
        diag_note(c->diag, "Only %s can be %s.", checker_type_name(c, type_sum(sum)), variant_name(c, id));
    }
    return types_new(c, TYPE_YESNO, expr->pos.line);
}

// The kind the variants named in the "when"s belong to (the first one's), or -1.
static long first_sum(const Checker *c, const Stmt *stmt, bool *nothing) {
    long sum = -1;
    for (size_t b = 0; b < stmt->as.match.branches.len; b++) {
        const MatchBranch *branch = &stmt->as.match.branches.items[b];
        *nothing |= branch->nothing;
        if (sum < 0 && branch->variants.len > 0) sum = (long)variant_kind(c, branch->variants.items[0])->sum;
    }
    return sum;
}

// Each "when" names variants of the subject's kind, once each. Fills `seen`
// (per variant id, plus one more for nothing) with the line of the "when".
static bool check_whens(Checker *c, const Stmt *stmt, Type type, size_t *seen) {
    size_t nothing_at = c->records->variants.len;
    bool ok = true;
    for (size_t b = 0; b < stmt->as.match.branches.len; b++) {
        const MatchBranch *branch = &stmt->as.match.branches.items[b];
        for (size_t i = 0; i <= branch->variants.len; i++) {
            bool is_nothing = i == branch->variants.len;
            if (is_nothing && !branch->nothing) break;
            size_t id = is_nothing ? nothing_at : branch->variants.items[i];
            if (!is_nothing && type_present(type) != type_sum(variant_kind(c, id)->sum)) {
                diag_error(c->diag, branch->pos.span, "%s is never %s.", checker_capitalized(c, type),
                           variant_name(c, id));
                diag_note(c->diag, "%s is defined on line %zu.",
                          checker_capitalized(c, type_sum(variant_kind(c, id)->sum)),
                          c->records->sums.items[variant_kind(c, id)->sum].definition->pos.line);
                ok = false;
            } else if (is_nothing && !type_is_maybe(type)) {
                diag_error(c->diag, branch->pos.span, "%s is never nothing.", checker_capitalized(c, type));
                diag_note(c->diag, "Only a value whose kind is like \"%s or nothing\" can be nothing.",
                          checker_type_name(c, type));
                ok = false;
            } else if (seen[id]) {
                diag_error(c->diag, branch->pos.span, "There's already a \"when\" for %s, on line %zu.",
                           is_nothing ? "nothing" : variant_name(c, id), seen[id]);
                diag_note(c->diag, "Each variant can have one \"when\".");
                ok = false;
            } else {
                seen[id] = branch->pos.line;
            }
        }
    }
    return ok;
}

// No "otherwise": every variant (and nothing) needs a "when". With one: it
// has to be able to run.
static void check_complete(Checker *c, const Stmt *stmt, Type type, const size_t *seen) {
    size_t n = c->records->variants.len, missing = 0;
    size_t *ids = arena_alloc(c->arena, (n + 1) * sizeof(size_t));
    for (size_t v = 0; type_is_sum(type) && v < n; v++) {
        if (variant_kind(c, v)->sum == type_sum_index(type) && !seen[v]) ids[missing++] = v;
    }
    bool nothing_missing = type_is_maybe(type) && !seen[n];
    const MatchBranch *last = &vec_last(&stmt->as.match.branches);
    const char *subject = checker_path(c, stmt->as.match.subject);
    if (last->otherwise && missing == 0 && !nothing_missing) {
        diag_warning(c->diag, last->pos.span, "This \"otherwise\" can never run: the \"when\"s above it cover every "
                                              "kind %s can be.", subject ? subject : "the value");
        diag_note(c->diag, "Take it out, or the sentences in it will never be used.");
        return;
    }
    if (last->otherwise || (missing == 0 && !nothing_missing)) return;
    const char *names = missing ? join_names(c, ids, missing) : "nothing";
    if (missing && nothing_missing) names = arena_sprintf(c->arena, "%s or nothing", names);
    diag_error(c->diag, stmt->pos.span, "This doesn't say what to do when %s is %s.", subject ? subject : "the value",
               names);
    diag_note(c->diag, "Add \"when it's %s:\"%s, or an \"otherwise:\" at the end.",
              missing ? variant_name(c, ids[0]) : "nothing", missing + nothing_missing > 1 ? " and the others" : "");
}

// What "it" is in a branch, and what the subject's path is checked to be.
static void check_branch(Checker *c, const Stmt *stmt, const MatchBranch *branch, TypeVar subject, Type type) {
    const char *path = checker_path(c, stmt->as.match.subject);
    size_t checked = c->present.len;
    Loop loop = {true, subject, true, NO_VARIANT};
    bool present = !branch->nothing && type_is_maybe(type);  // a variant, or "otherwise" after "when it's nothing"
    if (branch->otherwise) {
        present = false;
        for (size_t b = 0; b + 1 < stmt->as.match.branches.len; b++) present |= stmt->as.match.branches.items[b].nothing;
        present &= type_is_maybe(type);
    }
    if (present) loop.it = types_new(c, type_present(type), branch->pos.line);
    if (branch->variants.len == 1 && !branch->nothing) loop.variant = branch->variants.items[0];
    if (present || loop.variant != NO_VARIANT) checker_add_narrowing(c, path, loop.variant);
    vec_push(c->arena, &c->loops, loop);
    checker_check_block(c, &branch->body, branch->pos.line, branch->otherwise ? "the \"otherwise\"" : "the \"when\"");
    c->loops.len--;
    c->present.len = checked;
}

void checker_check_match(Checker *c, const Stmt *stmt) {
    const Expr *subject_expr = stmt->as.match.subject;
    const Expr *saved = c->unnarrowed;
    c->unnarrowed = subject_expr;  // its kind as written: a value that can be nothing can be asked about it
    TypeVar subject = checker_check_expr(c, subject_expr);
    c->unnarrowed = saved;
    bool nothing = false;
    long sum = first_sum(c, stmt, &nothing);
    Type type = sum >= 0 ? require_sum(c, subject, (size_t)sum, nothing, stmt->pos.line) : types_of(c, subject);
    if (type == TYPE_UNKNOWN && nothing) {
        types_require(c, subject, TYPE_MAYBE, stmt->pos.line);
        type = types_of(c, subject);
    }
    bool ok = type == TYPE_ERROR;
    if (!ok && !type_is_sum(type) && !type_is_maybe(type)) {
        const char *path = checker_path(c, subject_expr);
        diag_error(c->diag, subject_expr->pos.span, "%s is %s, which has no variants to check.",
                   path ? arena_sprintf(c->arena, "\"%s\"", path) : "This", checker_kind(c, subject));
        diag_note(c->diag, "\"check which kind\" tells apart the variants of a kind defined with \"is one of:\" "
                           "(like \"a shape is one of:\"), or a value that can be nothing.");
    } else if (!ok) {
        size_t *seen = arena_alloc(c->arena, (c->records->variants.len + 2) * sizeof(size_t));
        if (check_whens(c, stmt, type, seen)) check_complete(c, stmt, type, seen);
    }
    for (size_t b = 0; b < stmt->as.match.branches.len; b++) {
        check_branch(c, stmt, &stmt->as.match.branches.items[b], subject, type);
    }
}
