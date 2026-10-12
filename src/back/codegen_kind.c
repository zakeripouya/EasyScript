// C code for kinds with variants ("a shape is one of:"). A value is kept
// like a record (an EsRecord *), and each variant gets what a record kind
// gets (codegen_record.c), numbered by its place in Records.variants:
//
//   typedef struct { EsRecord base; double f0; } es_W0;
//   static const char *const es_wn0[] = {"radius"};
//   static const size_t es_wo0[] = {offsetof(es_W0, f0)};
//   static const EsRecordType es_wt0 = {"a circle", 1, es_wn0, "n", es_wo0, sizeof(es_W0), ES_VARIANT};
//   static EsRecord *es_make0(double f0) { ... }
//
// Its type pointer says which variant a value is: "s is a circle" is
// es_is_variant(s, &es_wt0), and a field is read with es_vfield_n(...), which
// checks the variant again while the program runs (the checker saw it
// checked, but a record's field holding it can have changed since).

#include <string.h>
#include "back/codegen_internal.h"

static const VariantDef *variant_def(const Codegen *g, size_t id) {
    const Stmt *def = g->records->sums.items[g->records->variants.items[id].sum].definition;
    for (size_t i = 0; i < def->as.sum.variants.len; i++) {
        if (def->as.sum.variants.items[i].id == id) return &def->as.sum.variants.items[i];
    }
    return NULL;
}

static void emit_variant(Codegen *g, const VariantDef *def, StrBuf *out) {
    size_t id = def->id, n = def->fields.len;
    sb_append(out, "typedef struct {\n    EsRecord base;\n");
    for (size_t i = 0; i < n; i++) {
        Type type = def->fields.items[i].type;
        sb_appendf(out, "    %s%sf%zu;\n", cg_ctype(type), type_counted(type) ? "" : " ", i);
    }
    sb_appendf(out, "} es_W%zu;\n", id);
    if (n > 0) {
        sb_appendf(out, "static const char *const es_wn%zu[] = {", id);
        for (size_t i = 0; i < n; i++) {
            const char *name = def->fields.items[i].name.display;
            sb_append(out, i ? ", " : "");
            cg_append_c_string(out, name, strlen(name));
        }
        sb_appendf(out, "};\nstatic const size_t es_wo%zu[] = {", id);
        for (size_t i = 0; i < n; i++) sb_appendf(out, "%soffsetof(es_W%zu, f%zu)", i ? ", " : "", id, i);
        sb_append(out, "};\n");
    }
    sb_appendf(out, "static const EsRecordType es_wt%zu = {", id);
    const char *display = def->name.display;
    const char *name = arena_sprintf(g->arena, "%s %s", strchr("aeiouAEIOU", display[0]) ? "an" : "a", display);
    cg_append_c_string(out, name, strlen(name));
    if (n > 0) {
        sb_appendf(out, ", %zu, es_wn%zu, \"", n, id);
        for (size_t i = 0; i < n; i++) sb_append_char(out, cg_field_kind(def->fields.items[i].type));
        sb_appendf(out, "\", es_wo%zu, sizeof(es_W%zu), ES_VARIANT};\n", id, id);
    } else {
        sb_appendf(out, ", 0, NULL, \"\", NULL, sizeof(es_W%zu), ES_VARIANT};\n", id);
    }
    // Making one: takes every field's value (owned).
    sb_appendf(out, "static EsRecord *es_make%zu(", id);
    for (size_t i = 0; i < n; i++) {
        Type type = def->fields.items[i].type;
        sb_appendf(out, "%s%s%sf%zu", i ? ", " : "", cg_ctype(type), type_counted(type) ? "" : " ", i);
    }
    sb_appendf(out, "%s) {\n    es_W%zu *r = (es_W%zu *)es_record_new(&es_wt%zu);\n", n ? "" : "void", id, id, id);
    for (size_t i = 0; i < n; i++) sb_appendf(out, "    r->f%zu = f%zu;\n", i, i);
    sb_append(out, "    return &r->base;\n}\n");
}

void cg_emit_variants(Codegen *g, StrBuf *out) {
    for (size_t s = 0; s < g->records->sums.len; s++) {
        const Stmt *def = g->records->sums.items[s].definition;
        for (size_t i = 0; def && i < def->as.sum.variants.len; i++) emit_variant(g, &def->as.sum.variants.items[i], out);
    }
}

// --- Values -----------------------------------------------------------------------------

// es_make0(VALUE, ...): the given values worked out in the order they're
// written; a field that can be nothing and isn't given is nothing.
void cg_emit_new_variant(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t id = expr->as.new_record.variant;
    const VariantDef *def = variant_def(g, id);
    size_t given = expr->as.new_record.values.len, n = def->fields.len;
    Expr **exprs = arena_alloc(g->arena, (given + 1) * sizeof(Expr *));
    const char **values = arena_alloc(g->arena, (given + 1) * sizeof(char *));
    for (size_t i = 0; i < given; i++) exprs[i] = expr->as.new_record.values.items[i].value;
    bool opened = cg_ordered(g, exprs, given, values, out);
    sb_appendf(out, "es_make%zu(", id);
    for (size_t f = 0; f < n; f++) {
        const char *value = "NULL";
        for (size_t i = 0; i < given; i++) {
            if (expr->as.new_record.values.items[i].index == f) value = values[i];
        }
        sb_appendf(out, "%s%s", f ? ", " : "", value);
    }
    sb_append(out, opened ? "))" : ")");
}

// es_vfield_n(SITE, R, &es_wt0, offsetof(es_W0, f0)), which releases R.
void cg_emit_variant_field(Codegen *g, const Expr *expr, StrBuf *out) {
    size_t id = expr->as.field.variant;
    Type kept = expr->narrowed ? expr->type + TYPE_MAYBE : expr->type;  // as the field keeps it
    sb_appendf(out, "es_vfield_%c(%zu, %s, &es_wt%zu, offsetof(es_W%zu, f%zu))", cg_field_kind(kept),
               cg_site(g, expr->pos), cg_operand(g, expr->as.field.record, false), id, id, expr->as.field.index);
}

void cg_emit_kind_test(Codegen *g, const Expr *expr, StrBuf *out) {
    sb_appendf(out, "%ses_is_variant(%s, &es_wt%zu)", expr->as.kind_test.negated ? "!" : "",
               cg_operand(g, expr->as.kind_test.operand, false), expr->as.kind_test.variant);
}

// --- check which kind S is -----------------------------------------------------------------

// The condition of one "when": (S && S->type == &es_wt0) || !S ...
static const char *condition(Codegen *g, const MatchBranch *branch, const char *subject) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    for (size_t i = 0; i < branch->variants.len; i++) {
        sb_appendf(&sb, "%s(%s && %s->type == &es_wt%zu)", i ? " || " : "", subject, subject, branch->variants.items[i]);
    }
    if (branch->nothing) sb_appendf(&sb, "%s!%s", branch->variants.len ? " || " : "", subject);
    return sb.data;
}

// { S is kept in a hidden variable (released with the block, or by "stop the
// loop" and "give back" in a "when");
//   if (S is a circle) { ... } else if (...) { ... } else { ... } }
// "it" in a "when" reads that variable. Not a C switch, so "stop the loop"
// in a "when" still leaves the loop around it.
void cg_emit_match(Codegen *g, const Stmt *stmt) {
    size_t id = ++g->loop_ids;
    Type type = stmt->as.match.subject->type;
    const char *hidden = arena_sprintf(g->arena, "0match%zu", id);
    StrBuf var;
    sb_init(&var, g->arena);
    cg_append_var(g, &var, hidden, type);
    cg_indent(g);
    sb_append(&g->body, "{\n");
    g->depth++;
    cg_push_scope(g);
    cg_declare_name(g, hidden, type);
    StrBuf value;
    sb_init(&value, g->arena);
    cg_emit_expr(g, stmt->as.match.subject, &value);
    cg_emit_store(g, hidden, type, value.data);
    for (size_t i = 0; i < stmt->as.match.branches.len; i++) {
        const MatchBranch *branch = &stmt->as.match.branches.items[i];
        if (i == 0) {
            cg_indent(g);
        } else {
            sb_append(&g->body, " else ");
        }
        LoopCode when = {stmt, id, g->scopes.len, hidden, type};
        vec_push(g->arena, &g->loops, when);
        if (!branch->otherwise && branch->guard) {  // "whose radius is over 10": tested once it's a circle
            sb_appendf(&g->body, "if ((%s) && %s) ", condition(g, branch, var.data), cg_operand(g, branch->guard, false));
        } else if (!branch->otherwise) {
            sb_appendf(&g->body, "if (%s) ", condition(g, branch, var.data));
        }
        sb_append(&g->body, "{\n");
        cg_emit_block(g, &branch->body);
        g->loops.len--;
        cg_indent(g);
        sb_append_char(&g->body, '}');
    }
    sb_append_char(&g->body, '\n');
    cg_pop_scope(g);
    g->depth--;
    cg_indent(g);
    sb_append(&g->body, "}\n");
}

// "X, or D": X out of its box (or the record itself) when it isn't nothing;
// otherwise D, worked out only then.
void cg_emit_default(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *value = expr->as.lookup.map;
    const char *v = cg_operand(g, value, false);
    const char *d = cg_operand(g, expr->as.lookup.fallback, false);
    size_t t = cg_new_temp(g, value->type);
    if (type_boxed(expr->type)) {
        char letter = expr->type == TYPE_TEXT ? 't' : expr->type == TYPE_YESNO ? 'b' : 'n';
        sb_appendf(out, "((es_t%zu = %s) != NULL ? es_unbox_%c(0, es_t%zu) : %s)", t, v, letter, t, d);
    } else {
        sb_appendf(out, "((es_t%zu = %s) != NULL ? es_t%zu : %s)", t, v, t, d);
    }
}

// A "depends on" table: the subject in a hidden name ("it" in the
// conditions), then the first value whose condition is yes:
// (es_vn_0table1 = SUBJECT, C1 ? V1 : C2 ? V2 : OTHERWISE).
void cg_emit_table(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *subject = expr->as.table.subject;
    Type type = subject->type;
    const char *name = arena_sprintf(g->arena, "0table%zu", ++g->loop_ids);
    cg_declare_name(g, name, type);
    const char *value = cg_operand(g, subject, false);
    sb_append(out, "(");
    if (type_counted(type)) {
        sb_appendf(out, "%s(&", cg_set(type));
        cg_append_var(g, out, name, type);
        sb_appendf(out, ", %s), ", value);
    } else {
        cg_append_var(g, out, name, type);
        sb_appendf(out, " = %s, ", value);
    }
    LoopCode loop = {NULL, 0, 0, name, type};
    vec_push(g->arena, &g->loops, loop);
    for (size_t i = 0; i < expr->as.table.values.len; i++) {
        const Expr *condition = expr->as.table.conditions.items[i];
        const char *v = cg_operand(g, expr->as.table.values.items[i], false);
        if (condition) {
            sb_appendf(out, "%s ? %s : ", cg_operand(g, condition, false), v);
        } else {
            sb_append(out, v);
        }
    }
    g->loops.len--;
    sb_append(out, ")");
}

// "an age must be between 0 and 150": a C function that gives back the value
// it's given, after checking it ("it" in the condition; only looked at):
// static double es_rule1(int es_site, double es_vn_0rule) {...}
void cg_emit_rule(Codegen *g, const Stmt *stmt) {
    const char *item = "0rule";
    Type type = TYPE_UNKNOWN;  // the kind of the fields it's about (the checker made them the same)
    for (size_t r = 0; r < g->records->len && type == TYPE_UNKNOWN; r++) {
        const Stmt *def = g->records->items[r].definition;
        for (size_t f = 0; def && f < def->as.record.fields.len; f++) {
            const Field *field = &def->as.record.fields.items[f];
            for (size_t k = 0; k < field->rules.len; k++) {
                if (field->rules.items[k] == stmt) type = field->type;
            }
        }
    }
    Codegen helper;
    cg_helper_init(g, &helper);
    LoopCode loop = {NULL, 0, 0, item, type};
    vec_push(g->arena, &helper.loops, loop);
    const char *test = cg_operand(&helper, stmt->as.rule.condition, false);
    StrBuf head;
    sb_init(&head, g->arena);
    sb_appendf(&head, "static %s%ses_rule%zu(int es_site, %s%s", cg_ctype(type), type_counted(type) ? "" : " ",
               stmt->as.rule.id, cg_ctype(type), type_counted(type) ? "" : " ");
    cg_append_raw_var(&head, item, type);
    sb_append(&head, ")");
    sb_appendf(&g->helpers->heads, "%s;\n", head.data);
    StrBuf *out = &g->helpers->code;
    sb_appendf(out, "%s {\n", head.data);
    cg_emit_declarations(&helper, out);
    sb_appendf(out, "    if (!(%s)) ", test);
    const char *suffix = type == TYPE_NUMBER ? "n" : type == TYPE_TEXT ? "t" : "x";
    sb_appendf(out, "es_rule_broken_%s(es_site, ", suffix);
    cg_append_c_string(out, stmt->as.rule.message, strlen(stmt->as.rule.message));
    if (suffix[0] != 'x') {
        sb_append(out, ", ");
        cg_append_raw_var(out, item, type);
    }
    sb_append(out, ");\n");
    cg_drop_helper_names(&helper, out);
    sb_append(out, "    return ");
    cg_append_raw_var(out, item, type);
    sb_append(out, ";\n}\n\n");
}

const char *cg_follow_rules(Codegen *g, const Field *field, const char *value, SourcePos pos) {
    for (size_t i = 0; i < field->rules.len; i++) {
        value = arena_sprintf(g->arena, "es_rule%zu(%zu, %s)", field->rules.items[i]->as.rule.id, cg_site(g, pos), value);
    }
    return value;
}

// A definition written as a sentence ("an order is big if ...") called on one
// value: its value written in place, with "it" that value, so a question that
// uses it costs what the same test written by hand does. A definition that
// uses itself is called instead (it would never end).
bool cg_emit_inline(Codegen *g, const Stmt *fn, const Expr *arg, StrBuf *out) {
    if (!fn->as.function.defines || fn->as.function.body.len != 1) return false;
    const Stmt *ret = fn->as.function.body.items[0];
    if (ret->kind != STMT_RETURN || !ret->as.returned) return false;
    for (size_t i = 0; i < g->inlining.len; i++) {
        if (g->inlining.items[i] == fn) return false;
    }
    Type type = fn->as.function.param_types.items[0];
    const char *name = NULL;
    const char *setup = "";
    if (arg->kind == EXPR_IT && !arg->narrowed && !arg->lifted) {  // the item of the question around: used as it is
        for (size_t i = g->loops.len; i > 0 && !name; i--) {
            const LoopCode *loop = &g->loops.items[i - 1];
            if (loop->item) name = loop->item_type == type ? loop->item : "";
        }
        if (name && !*name) name = NULL;
    }
    if (arg->kind == EXPR_NAME && !arg->narrowed && !arg->lifted && !cg_is_constant(g, arg->as.name) &&
        !cg_find_function(g, arg->as.name) && !cg_is_cell(g, arg->as.name) && arg->type == type) {
        name = arg->as.name;  // a name is used as it is
    }
    if (!name) {
        name = arena_sprintf(g->arena, "0inline%zu", ++g->loop_ids);
        cg_declare_name(g, name, type);
        StrBuf set;
        sb_init(&set, g->arena);
        if (type_counted(type)) {
            sb_appendf(&set, "%s(&", cg_set(type));
            cg_append_var(g, &set, name, type);
            sb_appendf(&set, ", %s), ", cg_operand(g, arg, false));
        } else {
            cg_append_var(g, &set, name, type);
            sb_appendf(&set, " = %s, ", cg_operand(g, arg, false));
        }
        setup = set.data;
    }
    LoopCode it = {NULL, 0, 0, name, type};
    vec_push(g->arena, &g->loops, it);
    vec_push(g->arena, &g->inlining, fn);
    const char *value = cg_operand(g, ret->as.returned, false);
    g->inlining.len--;
    g->loops.len--;
    sb_appendf(out, "(%s%s)", setup, value);
    return true;
}
