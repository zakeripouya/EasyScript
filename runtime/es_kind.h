/* EasyScript runtime, part 9: kinds with variants. Parts 1 to 8 come first.
 *
 * A value of a kind with variants ("a shape is one of: a circle with a
 * radius ...") is kept like a record (EsRecord, in es_memory.h): the
 * compiler writes an EsRecordType for each variant, with the style
 * ES_VARIANT, and the type says which variant a value is. Unlike a record,
 * such a value never changes after it's made, so two of them are equal
 * when they're the same variant with equal fields, and sharing one can't
 * be seen (except through a list or map in a field, which "a copy of"
 * copies). Like the rest of the runtime, these functions consume the
 * values they're given, except the ones marked "only looks". */
#ifndef ES_KIND_H
#define ES_KIND_H

#ifndef ES_MAP_H
#include "es_map.h"
#endif

/* The compiler saw the value checked to be this variant ("if s is a
 * circle:"), but it was reached through a record's field, which another name
 * for the record can have changed since. */
static _Noreturn void es_wrong_variant(int site, const EsRecord *r, const EsRecordType *type) {
    es_fail(site, "Another name for the same record can change it. Check it again just before using it.",
            "This was checked to be %s, but something has set it to %s since then.", type->name,
            r ? r->type->name : "nothing");
}

/* "s's radius": the field's value (owned), then s is released. */
#define ES_VARIANT_FIELD(S, T, RETAIN)                                                           \
    static T es_vfield_##S(int site, EsRecord *r, const EsRecordType *type, size_t offset) {      \
        if (!r || r->type != type) es_wrong_variant(site, r, type);                              \
        T v = RETAIN(ES_FIELD(r, T, offset));                                                    \
        es_record_release(r);                                                                    \
        return v;                                                                                \
    }

ES_VARIANT_FIELD(n, double, ES_SAME)
ES_VARIANT_FIELD(d, double, ES_SAME)
ES_VARIANT_FIELD(u, long long, ES_SAME)
ES_VARIANT_FIELD(e, long long, ES_SAME)
ES_VARIANT_FIELD(p, long long, ES_SAME)
ES_VARIANT_FIELD(b, bool, ES_SAME)
ES_VARIANT_FIELD(t, EsText *, es_retain)
ES_VARIANT_FIELD(l, EsList *, es_list_retain)
ES_VARIANT_FIELD(r, EsRecord *, es_record_retain)
ES_VARIANT_FIELD(m, EsMap *, es_map_retain)

/* "s is a circle" (nothing is never a circle) */
static bool es_is_variant(EsRecord *r, const EsRecordType *type) {
    bool is = r && r->type == type;
    es_record_release(r);
    return is;
}

/* Records and functions: the same one. Values of kinds with variants: the
 * same variant with equal fields (records in them the same record). Only looks. */
static bool es_record_equal(const EsRecord *a, const EsRecord *b) {
    if (a == b) return true;
    if (!a || !b || a->type != b->type || a->type->style == ES_RECORD || a->type->style == ES_CLOSURE) return false;
    const EsRecordType *type = a->type;
    for (int i = 0; i < type->fields; i++) {
        size_t at = type->offsets[i];
        bool same = true;
        switch (type->kinds[i]) {
        case 'n':
        case 'd': same = es_close(ES_FIELD(a, double, at), ES_FIELD(b, double, at)); break;
        case 'u':
        case 'e':
        case 'p': same = ES_FIELD(a, long long, at) == ES_FIELD(b, long long, at); break;
        case 'b': same = ES_FIELD(a, bool, at) == ES_FIELD(b, bool, at); break;
        case 't': same = es_text_equal(ES_FIELD(a, EsText *, at), ES_FIELD(b, EsText *, at)); break;
        case 'l': same = es_list_equal(ES_FIELD(a, EsList *, at), ES_FIELD(b, EsList *, at)); break;
        case 'm': same = es_map_equal(ES_FIELD(a, EsMap *, at), ES_FIELD(b, EsMap *, at)); break;
        default: same = es_record_equal(ES_FIELD(a, EsRecord *, at), ES_FIELD(b, EsRecord *, at)); break;
        }
        if (!same) return false;
    }
    return true;
}

/* --- Boxes: a number, text or yes/no that can be nothing ---------------------- */

typedef struct {
    EsRecord base;
    double v;
} EsBoxN;

typedef struct {
    EsRecord base;
    EsText *v;
} EsBoxT;

typedef struct {
    EsRecord base;
    bool v;
} EsBoxB;

static const char *const es_box_names[] = {"value"};
static const size_t es_box_at_n[] = {offsetof(EsBoxN, v)};
static const size_t es_box_at_t[] = {offsetof(EsBoxT, v)};
static const size_t es_box_at_b[] = {offsetof(EsBoxB, v)};
static const EsRecordType es_box_type_n = {"a number", 1, es_box_names, "n", es_box_at_n, sizeof(EsBoxN), ES_BOX};
static const EsRecordType es_box_type_t = {"text", 1, es_box_names, "t", es_box_at_t, sizeof(EsBoxT), ES_BOX};
static const EsRecordType es_box_type_b = {"a yes/no value", 1, es_box_names, "b", es_box_at_b, sizeof(EsBoxB), ES_BOX};

/* A value given where "or nothing" is kept: put in a box (takes text). */
static EsRecord *es_box_n(double v) {
    EsBoxN *r = (EsBoxN *)es_record_new(&es_box_type_n);
    r->v = v;
    return &r->base;
}

static EsRecord *es_box_t(EsText *v) {
    EsBoxT *r = (EsBoxT *)es_record_new(&es_box_type_t);
    r->v = v;
    return &r->base;
}

static EsRecord *es_box_b(bool v) {
    EsBoxB *r = (EsBoxB *)es_record_new(&es_box_type_b);
    r->v = v;
    return &r->base;
}

/* A box the compiler saw checked not to be nothing: what it holds (owned),
 * then the box is released. Another name for a record holding it can have
 * set it to nothing since; then it's a runtime error, as for records. */
static double es_unbox_n(int site, EsRecord *r) {
    double v = ((EsBoxN *)es_present(site, r))->v;
    es_record_release(r);
    return v;
}

static EsText *es_unbox_t(int site, EsRecord *r) {
    EsText *v = es_retain(((EsBoxT *)es_present(site, r))->v);
    es_record_release(r);
    return v;
}

static bool es_unbox_b(int site, EsRecord *r) {
    bool v = ((EsBoxB *)es_present(site, r))->v;
    es_record_release(r);
    return v;
}

/* "the position of X in T": the runtime's searches give 0 when X isn't
 * there, which is nothing. */
static EsRecord *es_found(double position) {
    return position > 0 ? es_box_n(position) : NULL;
}

/* "a copy of s": the same variant, with its own copies of the lists and maps
 * in its fields. Text, records and values of kinds in it are shared (text
 * and those values never change; records are shared by every copy). */
static EsRecord *es_kind_copy(EsRecord *r) {
    if (!r) return NULL;
    const EsRecordType *type = r->type;
    EsRecord *copy = es_record_new(type);
    memcpy((char *)copy + sizeof(EsRecord), (const char *)r + sizeof(EsRecord), type->size - sizeof(EsRecord));
    for (int i = 0; i < type->fields; i++) {
        size_t at = type->offsets[i];
        switch (type->kinds[i]) {
        case 't': es_retain(ES_FIELD(copy, EsText *, at)); break;
        case 'l': ES_FIELD(copy, EsList *, at) = es_list_copy_of(ES_FIELD(r, EsList *, at)); break;
        case 'm': ES_FIELD(copy, EsMap *, at) = es_map_copy_of(ES_FIELD(r, EsMap *, at)); break;
        case 'r': es_record_retain(ES_FIELD(copy, EsRecord *, at)); break;
        default: break;
        }
    }
    es_record_release(r);
    return copy;
}

#endif
