// C code for records. Each kind of record the program defines becomes a C
// struct that starts with the runtime's EsRecord header, a description the
// runtime uses to free, copy and print it (runtime/es_record.h), and a
// function that makes one:
//
//   typedef struct { EsRecord base; EsText *f0; double f1; } es_R0;
//   static const char *const es_rn0[] = {"name", "age"};
//   static const size_t es_ro0[] = {offsetof(es_R0, f0), offsetof(es_R0, f1)};
//   static const EsRecordType es_rt0 = {"a dog", 2, es_rn0, "tn", es_ro0, sizeof(es_R0)};
//   static EsRecord *es_new0(EsText *f0, double f1) { ... }
//
// A record value is an EsRecord * (NULL for nothing); a field is reached
// through its kind's struct: ((es_R0 *)r)->f1. Like text and lists, every
// record expression gives an owned reference, and fields own what they hold.

#include <string.h>
#include "back/codegen_internal.h"

char cg_field_kind(Type type) {
    if (type_is_list(type)) return 'l';
    if (type_is_map(type)) return 'm';
    if (type_is_object(type)) return 'r';
    return cg_kind_letter(type);
}

// The static list a list field starts as: es_k_0d<kind>__<field>.
static const char *default_list_name(Codegen *g, size_t kind, size_t field) {
    return arena_sprintf(g->arena, "0d%zu_%zu", kind, field);
}

static void emit_kind(Codegen *g, size_t index, const RecordKind *kind, StrBuf *out) {
    const Stmt *def = kind->definition;
    size_t n = def->as.record.fields.len;
    sb_append(out, "typedef struct {\n    EsRecord base;\n");
    for (size_t i = 0; i < n; i++) {
        Type type = def->as.record.fields.items[i].type;
        sb_appendf(out, "    %s%sf%zu;\n", cg_ctype(type), type_counted(type) ? "" : " ", i);
    }
    sb_appendf(out, "} es_R%zu;\n", index);
    sb_appendf(out, "static const char *const es_rn%zu[] = {", index);
    for (size_t i = 0; i < n; i++) {
        const char *name = def->as.record.fields.items[i].name.display;
        sb_append(out, i ? ", " : "");
        cg_append_c_string(out, name, strlen(name));
    }
    sb_appendf(out, "};\nstatic const size_t es_ro%zu[] = {", index);
    for (size_t i = 0; i < n; i++) sb_appendf(out, "%soffsetof(es_R%zu, f%zu)", i ? ", " : "", index, i);
    sb_appendf(out, "};\nstatic const EsRecordType es_rt%zu = {", index);
    const char *name = arena_sprintf(g->arena, "%s %s", strchr("aeiouAEIOU", kind->display[0]) ? "an" : "a",
                                     kind->display);
    cg_append_c_string(out, name, strlen(name));
    sb_appendf(out, ", %zu, es_rn%zu, \"", n, index);
    for (size_t i = 0; i < n; i++) sb_append_char(out, cg_field_kind(def->as.record.fields.items[i].type));
    sb_appendf(out, "\", es_ro%zu, sizeof(es_R%zu), ES_RECORD};\n", index, index);
    // Making one: takes every field's value (owned).
    sb_appendf(out, "static EsRecord *es_new%zu(", index);
    for (size_t i = 0; i < n; i++) {
        Type type = def->as.record.fields.items[i].type;
        sb_appendf(out, "%s%s%sf%zu", i ? ", " : "", cg_ctype(type), type_counted(type) ? "" : " ", i);
    }
    sb_appendf(out, ") {\n    es_R%zu *r = (es_R%zu *)es_record_new(&es_rt%zu);\n", index, index, index);
    for (size_t i = 0; i < n; i++) sb_appendf(out, "    r->f%zu = f%zu;\n", i, i);
    sb_append(out, "    return &r->base;\n}\n");
    for (size_t i = 0; i < n; i++) {  // lists that fields start as
        const Field *field = &def->as.record.fields.items[i];
        if (field->initial && field->folded.kind == CONST_LIST) {
            cg_emit_fixed_list(g, default_list_name(g, index, i), &field->folded, out);
        }
        if (field->initial && field->folded.kind == CONST_MAP) {
            cg_emit_fixed_map(g, default_list_name(g, index, i), &field->folded, out);
        }
    }
}

void cg_emit_records(Codegen *g, StrBuf *out) {
    for (size_t i = 0; i < g->records->len; i++) {
        if (g->records->items[i].definition) emit_kind(g, i, &g->records->items[i], out);
    }
}

// --- Values -------------------------------------------------------------------------

// What a field holds when "new" doesn't give it a value.
static const char *starting_value(Codegen *g, size_t kind, size_t index, const Field *field) {
    if (!field->initial || field->initial->kind == EXPR_NOTHING) return "NULL";
    const ConstValue *v = &field->folded;
    StrBuf sb;
    sb_init(&sb, g->arena);
    switch (v->kind) {
    case CONST_NUMBER: cg_append_number(&sb, v->number); break;
    case CONST_YESNO: sb_append(&sb, v->yes ? "true" : "false"); break;
    case CONST_TEXT: sb_appendf(&sb, "&es_s%zu", cg_static_text(g, v->text, v->len)); break;
    case CONST_LIST:  // each record gets its own copy
        cg_append_prefixed(&sb, "es_list_copy(&es_k_", default_list_name(g, kind, index));
        sb_append_char(&sb, ')');
        break;
    case CONST_MAP:
        cg_append_prefixed(&sb, "es_map_copy(es_k_", default_list_name(g, kind, index));
        sb_append_char(&sb, ')');
        break;
    }
    return sb.data;
}

// es_new0(VALUE, ...), the given values worked out in the order they're written.
void cg_emit_new(Codegen *g, const Expr *expr, StrBuf *out) {
    Type type = expr->as.new_record.type;
    if (type_is_sum(type)) {
        cg_emit_new_variant(g, expr, out);
        return;
    }
    size_t kind = type_record_index(type);
    const Stmt *def = g->records->items[kind].definition;
    size_t given = expr->as.new_record.values.len, n = def->as.record.fields.len;
    Expr **exprs = arena_alloc(g->arena, (given + 1) * sizeof(Expr *));
    const char **values = arena_alloc(g->arena, (given + 1) * sizeof(char *));
    for (size_t i = 0; i < given; i++) exprs[i] = expr->as.new_record.values.items[i].value;
    bool opened = cg_ordered(g, exprs, given, values, out);
    sb_appendf(out, "es_new%zu(", kind);
    for (size_t f = 0; f < n; f++) {
        const char *value = NULL;
        SourcePos at = expr->pos;  // where a rule that's broken points: the value, if it's given
        for (size_t i = 0; i < given; i++) {
            if (expr->as.new_record.values.items[i].index == f) value = values[i], at = exprs[i]->pos;
        }
        const Field *field = &def->as.record.fields.items[f];
        value = value ? value : starting_value(g, kind, f, field);
        sb_appendf(out, "%s%s", f ? ", " : "", cg_follow_rules(g, field, value, at));
    }
    sb_append(out, opened ? "))" : ")");
}

// A variable that can be looked at where it is, without a reference of its own.
static bool is_plain_variable(const Codegen *g, const Expr *expr) {
    return expr->kind == EXPR_NAME && !expr->narrowed && !cg_is_constant(g, expr->as.name) &&
           !cg_find_function(g, expr->as.name);
}

// R's NAME: "((es_R0 *)es_vr0_rex)->f1" for a variable (retained if it's
// counted), otherwise es_field_n(R, offsetof(es_R0, f1)), which releases R.
void cg_emit_field(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *record = expr->as.field.record;
    if (type_is_sum(record->type)) {
        cg_emit_variant_field(g, expr, out);
        return;
    }
    size_t kind = type_record_index(record->type), index = expr->as.field.index;
    Type type = expr->narrowed ? expr->type + TYPE_MAYBE : expr->type;  // as the field keeps it
    if (is_plain_variable(g, record)) {
        if (type_counted(type)) sb_appendf(out, "%s(", cg_retain(type));
        sb_appendf(out, "((es_R%zu *)", kind);
        cg_append_var(g, out, record->as.name, record->type);
        sb_appendf(out, ")->f%zu", index);
        if (type_counted(type)) sb_append_char(out, ')');
        return;
    }
    sb_appendf(out, "es_field_%c(%s, offsetof(es_R%zu, f%zu))", cg_field_kind(type), cg_operand(g, record, false), kind,
               index);
}

// --- Sentences ------------------------------------------------------------------------

// set R's F to V / add V to R's F / increase R's F by V ...: one C block that
// works out R, then changes the field, then releases R.
const char *cg_field_stmt(Codegen *g, const Stmt *stmt) {
    bool set = stmt->kind == STMT_SET;
    const Expr *field = set ? stmt->as.assign.field : stmt->as.change.field;
    if (field->kind == EXPR_MAP_GET) return cg_map_stmt(g, stmt);  // "the value for K in M"
    const Expr *value = set ? stmt->as.assign.value : stmt->as.change.amount;
    Type type = field->type;
    size_t kind = type_record_index(field->as.field.record->type);
    const char *slot = arena_sprintf(g->arena, "((es_R%zu *)es_r)->f%zu", kind, field->as.field.index);
    StrBuf out;
    sb_init(&out, g->arena);
    const char *code = cg_operand(g, value, false);
    const Field *def = &g->records->items[kind].definition->as.record.fields.items[field->as.field.index];
    bool value_first = !set && value->pos.span.offset < field->pos.span.offset;
    if (value_first) {  // "add V to R's F": V is written first, so it's worked out first
        Type value_type = cg_value_type(value);
        sb_appendf(&out, "{ %s%ses_value = %s; ", cg_ctype(value_type), type_counted(value_type) ? "" : " ", code);
        code = "es_value";
    } else {
        sb_append(&out, "{ ");
    }
    sb_appendf(&out, "EsRecord *es_r = %s; ", cg_operand(g, field->as.field.record, false));
    if (set && type_counted(type)) {
        sb_appendf(&out, "%s(&%s, %s); ", cg_set(type), slot, cg_follow_rules(g, def, code, stmt->pos));
    } else if (set) {
        sb_appendf(&out, "%s = %s; ", slot, cg_follow_rules(g, def, code, stmt->pos));
    } else if (stmt->as.change.append) {
        sb_appendf(&out, "es_add_%s(es_list_retain(%s), %s); ", cg_item_suffix(type), slot, code);
    } else {
        sb_appendf(&out, "%s = %s; ", slot, cg_changed(g, stmt, type, slot, code));
    }
    if (!set && def->rules.len > 0) sb_appendf(&out, "(void)%s; ", cg_follow_rules(g, def, slot, stmt->pos));
    sb_append(&out, "es_record_release(es_r); }");
    return out.data;
}
