/* EasyScript runtime, part 7: records. Parts 1 to 6 come first.
 *
 * A record (EsRecord, in es_memory.h) is a header followed by its fields, laid
 * out by a C struct the compiler writes for each kind of record, with an
 * EsRecordType that says where each field is and what kind it holds. Records
 * are shared like lists: every name that holds the same record sees its
 * changes. A field that can be nothing holds NULL for nothing. Like the rest
 * of the runtime, the functions here consume the records they're given,
 * except the ones marked "only looks".
 *
 * Records can point to each other in a circle (two people who are each
 * other's friend). Counting references can't free those: each keeps the next
 * alive. With ES_DEBUG_MEMORY, the check at the end of the program finds such
 * circles and explains them (es_report_cycles). */
#ifndef ES_RECORD_H
#define ES_RECORD_H

#ifndef ES_TEXT_H
#include "es_text.h"
#endif

#include <ctype.h>

/* --- Fields -------------------------------------------------------------------- */

/* "R's F" for a record that isn't kept in a variable: the field's value
 * (owned), then R is released. */
#define ES_FIELD_GET(S, T, RETAIN)                              \
    static T es_field_##S(EsRecord *r, size_t offset) {         \
        T v = RETAIN(ES_FIELD(r, T, offset));                   \
        es_record_release(r);                                   \
        return v;                                               \
    }

ES_FIELD_GET(n, double, ES_SAME)
ES_FIELD_GET(d, double, ES_SAME)
ES_FIELD_GET(u, long long, ES_SAME)
ES_FIELD_GET(e, long long, ES_SAME)
ES_FIELD_GET(p, long long, ES_SAME)
ES_FIELD_GET(b, bool, ES_SAME)
ES_FIELD_GET(t, EsText *, es_retain)
ES_FIELD_GET(l, EsList *, es_list_retain)
ES_FIELD_GET(r, EsRecord *, es_record_retain)
ES_FIELD_GET(m, EsMap *, es_map_retain)

/* A record or nothing that the compiler saw checked ("if the node's next is
 * not nothing:"). Another name for the same record can still have changed it
 * since; then it's a runtime error instead of a crash. */
static EsRecord *es_present(int site, EsRecord *r) {
    if (!r) {
        es_fail(site, "Another name for the same record can change it. Check it again just before using it.",
                "This was checked not to be nothing, but something has set it to nothing since then.");
    }
    return r;
}

/* "X is nothing" */
static bool es_is_nothing(EsRecord *r) {
    bool nothing = r == NULL;
    es_record_release(r);
    return nothing;
}

/* "A is B" for records: whether they're the same record (two records with
 * the same fields are still two records); for values of a kind with
 * variants, whether they're equal (es_kind.h). */
static bool es_record_same(EsRecord *a, EsRecord *b) {
    bool same = es_record_equal(a, b);
    es_record_release(a);
    es_record_release(b);
    return same;
}

/* "a copy of R": a new record of the same kind whose fields hold the same
 * values. The text, lists, records and maps in them are shared, not copied. A
 * record nobody else holds is already a copy, so it's given back as it is. */
static EsRecord *es_record_copy(EsRecord *r) {
    if (r->header.refs == 1) return r;
    const EsRecordType *type = r->type;
    EsRecord *copy = es_record_new(type);
    memcpy((char *)copy + sizeof(EsRecord), (const char *)r + sizeof(EsRecord), type->size - sizeof(EsRecord));
    for (int i = 0; i < type->fields; i++) {
        char kind = type->kinds[i];
        bool counted = kind == 't' || kind == 'l' || kind == 'r' || kind == 'm';
        EsObject *field = counted ? ES_FIELD(copy, EsObject *, type->offsets[i]) : NULL;
        if (field && field->refs != ES_IMMORTAL) field->refs++;
    }
    es_record_release(r);
    return copy;
}

/* --- As text ------------------------------------------------------------------- */

/* Records inside records show their fields this many levels deep; deeper
 * ones show as "a node with ...". This also keeps a circle of records from
 * printing forever. */
#define ES_RECORD_DEPTH 3

static void es_buf_add_text(EsBuf *b, const char *s) {
    es_buf_add(b, s, strlen(s));
}

/* What a box holds, as text: in quotes when `quoted` (inside a record or a
 * map, where text is quoted), on its own otherwise. Only looks. */
static void es_box_render(EsBuf *b, const EsRecord *r, bool quoted) {
    size_t at = r->type->offsets[0];
    switch (r->type->kinds[0]) {
    case 'n': {
        EsText *t = es_number_text(ES_FIELD(r, double, at));
        es_buf_add(b, t->chars, t->len);
        es_release(t);
        break;
    }
    case 'b': es_buf_add_text(b, ES_FIELD(r, bool, at) ? "yes" : "no"); break;
    default: {
        const EsText *t = ES_FIELD(r, EsText *, at);
        if (quoted) es_buf_add(b, "\"", 1);
        es_buf_add(b, t->chars, t->len);
        if (quoted) es_buf_add(b, "\"", 1);
        break;
    }
    }
}

/* Field i of r, as "say" shows it inside a record. */
static void es_field_render(EsBuf *b, const EsRecord *r, int i, int depth) {
    const EsRecordType *type = r->type;
    size_t at = type->offsets[i];
    switch (type->kinds[i]) {
    case 'n': {
        EsText *t = es_number_text(ES_FIELD(r, double, at));
        es_buf_add(b, t->chars, t->len);
        es_release(t);
        break;
    }
    case 'd': es_buf_add_date(b, ES_FIELD(r, double, at)); break;
    case 'u': es_buf_add_money(b, (double)ES_FIELD(r, long long, at), ES_ITEMS_DOLLARS); break;
    case 'e': es_buf_add_money(b, (double)ES_FIELD(r, long long, at), ES_ITEMS_EUROS); break;
    case 'p': es_buf_add_money(b, (double)ES_FIELD(r, long long, at), ES_ITEMS_POUNDS); break;
    case 'b': es_buf_add_text(b, ES_FIELD(r, bool, at) ? "yes" : "no"); break;
    case 't': {
        const EsText *t = ES_FIELD(r, EsText *, at);
        es_buf_add(b, "\"", 1);
        es_buf_add(b, t->chars, t->len);
        es_buf_add(b, "\"", 1);
        break;
    }
    case 'l': {
        const EsList *l = ES_FIELD(r, EsList *, at);
        es_buf_add(b, "(", 1);
        if (l->len == 0) es_buf_add_text(b, "an empty list");
        es_list_render(b, l, depth + 1);
        es_buf_add(b, ")", 1);
        break;
    }
    case 'm': {
        const EsMap *m = ES_FIELD(r, EsMap *, at);
        es_buf_add(b, "(", 1);
        if (m->len == 0) es_buf_add_text(b, "an empty map");
        es_map_render(b, m, depth + 1);
        es_buf_add(b, ")", 1);
        break;
    }
    default: {
        const EsRecord *inner = ES_FIELD(r, EsRecord *, at);
        bool parenthesized = inner && inner->type->style != ES_BOX && inner->type->style != ES_CLOSURE;
        if (parenthesized) es_buf_add(b, "(", 1);
        es_record_render(b, inner, depth + 1);
        if (parenthesized) es_buf_add(b, ")", 1);
        break;
    }
    }
}

/* Whether a column's name in a table's header is a field's name: the same
 * letters, capitals or not, with "_" for a space. */
static bool es_column_is(const EsText *column, const char *field) {
    size_t n = strlen(field);
    if (column->len != n) return false;
    for (size_t i = 0; i < n; i++) {
        char x = column->chars[i], y = field[i];
        if (x == '_') x = ' ';
        if (tolower((unsigned char)x) != tolower((unsigned char)y)) return false;
    }
    return true;
}

/* A row of "read the table in F" (fields 0 and 1 hold the file's header and
 * the row's cells): every column in the file's order, the ones the program
 * uses with their values now. */
static void es_table_render(EsBuf *b, const EsRecord *r, int depth) {
    const EsRecordType *type = r->type;
    const EsList *header = ES_FIELD(r, EsList *, type->offsets[0]), *cells = ES_FIELD(r, EsList *, type->offsets[1]);
    for (size_t c = 0; c < header->len; c++) {
        es_buf_add_text(b, c == 0 ? " with " : c + 1 == header->len ? " and " : ", ");
        es_buf_add(b, header->at.t[c]->chars, header->at.t[c]->len);
        es_buf_add(b, " ", 1);
        int field = -1;
        for (int f = 2; f < type->fields && field < 0; f++) {
            if (es_column_is(header->at.t[c], type->field_names[f])) field = f;
        }
        if (field >= 0) {
            es_field_render(b, r, field, depth);
        } else {
            es_buf_add(b, "\"", 1);
            es_buf_add(b, cells->at.t[c]->chars, cells->at.t[c]->len);
            es_buf_add(b, "\"", 1);
        }
    }
}

/* `a dog with name "Rex" and age 3`; a record, list or map in a field is in
 * parentheses, text in quotes, nothing as "nothing". A box shows what it
 * holds (quoted text inside records). Only looks. */
static void es_record_render(EsBuf *b, const EsRecord *r, int depth) {
    if (!r) {
        es_buf_add_text(b, "nothing");
        return;
    }
    if (r->type->style == ES_BOX) {
        es_box_render(b, r, depth > 0);
        return;
    }
    const EsRecordType *type = r->type;
    if (type->style == ES_CLOSURE) { /* "the function double", "the function made on line 3" */
        es_buf_add_text(b, type->name);
        return;
    }
    es_buf_add_text(b, type->name);
    if (depth >= ES_RECORD_DEPTH && type->fields > 0) {
        es_buf_add_text(b, " with ...");
        return;
    }
    if (type->fields >= 2 && strcmp(type->field_names[0], "0header") == 0) {
        es_table_render(b, r, depth);
        return;
    }
    for (int i = 0; i < type->fields; i++) {
        es_buf_add_text(b, i == 0 ? " with " : i + 1 == type->fields ? " and " : ", ");
        es_buf_add_text(b, type->field_names[i]);
        es_buf_add(b, " ", 1);
        es_field_render(b, r, i, depth);
    }
}

static EsText *es_buf_text(EsBuf *b) {
    EsText *t = b->len > 0 ? es_text_copy(b->data, b->len) : &es_empty_text;
    free(b->data);
    return t;
}

/* "say rex" */
static EsText *es_record_text(EsRecord *r) {
    EsBuf b = {NULL, 0, 0};
    es_record_render(&b, r, 0);
    es_record_release(r);
    return es_buf_text(&b);
}

/* For the call chain of a runtime error: its own fields only. Only looks. */
static EsText *es_record_short_text(const EsRecord *r) {
    EsBuf b = {NULL, 0, 0};
    es_record_render(&b, r, ES_RECORD_DEPTH - 1);
    return es_buf_text(&b);
}

/* --- Records in a circle ------------------------------------------------------------
 *
 * At the end of the program everything should have been freed. Records that
 * are still alive only because records among them point to each other are a
 * circle. Finding them: every live record's count, minus the references from
 * other live records (directly, or through lists and maps only they hold), is how
 * many references come from anywhere else. Records with some, and everything
 * they lead to, are kept alive from outside; the rest keep each other alive. */

#define ES_REACHED SIZE_MAX

typedef struct {
    EsRecord **items;
    size_t len;
} EsRecordStack;

typedef void (*EsVisit)(EsRecord *, EsRecordStack *);

static void es_each_inner_value(EsItems kind, EsValue v, bool own_lists, EsVisit visit, EsRecordStack *stack);

/* Calls visit for every record a list or map holds, directly or in the lists
 * and maps inside it (only ones nobody else holds, when `own_lists`). */
static void es_each_inner_list(const EsList *l, bool own_lists, EsVisit visit, EsRecordStack *stack) {
    if (own_lists && l->header.refs != 1) return;
    for (size_t i = 0; i < l->len && es_items_counted(l->items) && l->items != ES_ITEMS_TEXT; i++) {
        EsValue v = {.o = l->at.o[i]};
        es_each_inner_value(l->items, v, own_lists, visit, stack);
    }
}

static void es_each_inner_map(const EsMap *m, bool own_lists, EsVisit visit, EsRecordStack *stack) {
    if (own_lists && m->header.refs != 1) return;
    for (size_t i = 0; i < m->used; i++) {
        if (m->entries[i].hash != 0) es_each_inner_value(m->values, m->entries[i].value, own_lists, visit, stack);
    }
}

static void es_each_inner_value(EsItems kind, EsValue v, bool own_lists, EsVisit visit, EsRecordStack *stack) {
    if (kind == ES_ITEMS_RECORD && v.r) visit(v.r, stack);
    if (kind == ES_ITEMS_LIST) es_each_inner_list(v.l, own_lists, visit, stack);
    if (kind == ES_ITEMS_MAP) es_each_inner_map(v.m, own_lists, visit, stack);
}

/* Calls visit for every record a record holds, directly or in its lists and maps. */
static void es_each_inner(const EsRecord *r, bool own_lists, EsVisit visit, EsRecordStack *stack) {
    for (int i = 0; i < r->type->fields; i++) {
        size_t at = r->type->offsets[i];
        char kind = r->type->kinds[i];
        if (kind != 'r' && kind != 'l' && kind != 'm') continue;
        EsValue v = {.o = ES_FIELD(r, EsObject *, at)};
        es_each_inner_value(kind == 'r' ? ES_ITEMS_RECORD : kind == 'l' ? ES_ITEMS_LIST : ES_ITEMS_MAP, v, own_lists,
                            visit, stack);
    }
}

static void es_count_inner(EsRecord *r, EsRecordStack *stack) {
    (void)stack;
    r->mark--;
}

static void es_reach(EsRecord *r, EsRecordStack *stack) {
    if (r->mark == ES_REACHED) return;
    r->mark = ES_REACHED;
    stack->items[stack->len++] = r;
}

/* A circle with a function in it: the function uses a name that holds it
 * (or holds something that holds it). Explained in es_closure.h. */
static bool es_report_function_cycle(size_t circled, EsRecord *oldest);

static bool es_report_cycles(void) {
    size_t live = 0;
    for (EsRecord *r = es_memory.records; r; r = r->next) {
        r->mark = r->header.refs;
        live++;
    }
    if (live == 0) return false;
    for (EsRecord *r = es_memory.records; r; r = r->next) es_each_inner(r, true, es_count_inner, NULL);
    EsRecordStack stack = {malloc(live * sizeof(EsRecord *)), 0};
    if (!stack.items) es_out_of_memory();
    for (EsRecord *r = es_memory.records; r; r = r->next) {
        if (r->mark != ES_REACHED && r->mark > 0) es_reach(r, &stack);
        while (stack.len > 0) es_each_inner(stack.items[--stack.len], false, es_reach, &stack);
    }
    free(stack.items);
    size_t circled = 0;
    EsRecord *oldest = NULL;
    bool variants = false; /* values of kinds with variants in it: the circle goes through a list or map */
    bool functions = false; /* a function in it: it keeps itself alive through a name it uses */
    for (EsRecord *r = es_memory.records; r; r = r->next) {
        if (r->mark == ES_REACHED || r->type->style == ES_CELL) continue; /* cells are the names, not values */
        circled++, oldest = r, variants |= r->type->style == ES_VARIANT;
        functions |= r->type->style == ES_CLOSURE;
    }
    if (circled == 0) return false;
    if (functions) return es_report_function_cycle(circled, oldest);
    fprintf(stderr, "Memory check: %zu %s%s still alive at the end of the program, because they point to each "
                    "other in a circle:\n", circled, variants ? "value" : "record", circled == 1 ? " was" : "s were");
    size_t shown = 0;
    for (EsRecord *r = oldest; r && shown < 3; r = r->prev) {
        if (r->mark == ES_REACHED || r->type->style == ES_CELL) continue;
        EsText *t = es_record_short_text(r);
        fputs("    ", stderr);
        es_print_cut(t, 70, false);
        fputc('\n', stderr);
        es_release(t);
        shown++;
    }
    if (circled > shown) fprintf(stderr, "    ...and %zu more\n", circled - shown);
    if (variants) {
        fputs("Values in a circle are never freed, because each one keeps the next alive. Before the program ends, "
              "take a value out of the list or map that holds it.\n", stderr);
    } else {
        fputs("Records in a circle are never freed, because each one keeps the next alive. Before the program ends, "
              "set a field in the circle to nothing, or take a record out of the list or map that holds it.\n", stderr);
    }
    return true;
}

#endif
