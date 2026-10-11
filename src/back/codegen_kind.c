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
    sb_appendf(out, "es_vfield_%c(%zu, %s, &es_wt%zu, offsetof(es_W%zu, f%zu))", cg_field_kind(expr->type),
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
    cg_append_var(&var, hidden, type);
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
        if (!branch->otherwise) sb_appendf(&g->body, "if (%s) ", condition(g, branch, var.data));
        sb_append(&g->body, "{\n");
        LoopCode when = {stmt, id, g->scopes.len, hidden, type};
        vec_push(g->arena, &g->loops, when);
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
