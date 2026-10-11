/* EasyScript runtime, part 8: maps. Parts 1 to 7 come first.
 *
 * A map (EsMap, in es_memory.h) finds a value by its key. Keys are text or
 * numbers; values can be any kind, one kind per map. The entries stay in the
 * order they were first added, so going through a map, printing it, or
 * taking its keys always gives the same order. Changing a key's value keeps
 * its place; removing a key and adding it again puts it at the end.
 *
 * Finding a key: small maps (up to ES_MAP_SMALL entries) look through their
 * entries; bigger ones have an index, an array of places (a power of two,
 * kept at most two thirds full) that a key's hash points into, holding where
 * its entry is (linear probing; a removed key leaves ES_GONE behind until
 * the index is rebuilt).
 *
 * Number keys are the same key when they're the same to 12 significant
 * digits (es_key_number), so 0.1 plus 0.2 finds the value kept for 0.3, as
 * "is" (equal within a relative 1e-12) says they're equal.
 *
 * Maps are shared like lists: every name that holds the same map sees its
 * changes. Text and records in a map are shared; lists and maps in a map
 * belong to it, so a copy of the map copies them too. Like the rest of the
 * runtime, every function here consumes the maps, keys and values it's
 * given (releases them, or keeps them in the map), except the ones marked
 * "only looks". */
#ifndef ES_MAP_H
#define ES_MAP_H

#ifndef ES_RECORD_H
#include "es_record.h"
#endif

#define ES_MAP_SMALL 8 /* maps with up to this many entries have no index */
#define ES_NOT_FOUND SIZE_MAX

/* --- Keys ------------------------------------------------------------------------ */

/* The number a key is kept as: whole numbers exactly, others rounded to 12
 * significant digits, close to what "is" treats as equal (a relative 1e-12),
 * so numbers that differ only by rounding errors are the same key. (Printing
 * rounds further, to 10 digits; see es_number_text.) */
static double es_key_number(double x) {
    if (isnan(x)) return NAN;
    if (isinf(x)) return x;
    if (x == floor(x) && fabs(x) <= ES_EXACT_WHOLE) return x == 0 ? 0.0 : x; /* no -0 */
    char digits[32]; /* "%.12g" of a double takes at most 19 characters */
    snprintf(digits, sizeof digits, "%.12g", x);
    return strtod(digits, NULL);
}

/* Spreads the bits of h; never 0, which marks a removed entry. */
static uint64_t es_mix(uint64_t h) {
    h ^= h >> 30;
    h *= 0xbf58476d1ce4e5b9ULL;
    h ^= h >> 27;
    h *= 0x94d049bb133111ebULL;
    h ^= h >> 31;
    return h ? h : 1;
}

static uint64_t es_hash_text(const EsText *t) {
    const char *p = t->chars;
    size_t n = t->len;
    uint64_t h = 0x9e3779b97f4a7c15ULL ^ n;
    for (; n >= 8; p += 8, n -= 8) {
        uint64_t word;
        memcpy(&word, p, 8);
        h = (h ^ word) * 0xff51afd7ed558ccdULL;
        h ^= h >> 32;
    }
    uint64_t rest = 0;
    memcpy(&rest, p, n);
    return es_mix((h ^ rest) * 0xc4ceb9fe1a85ec53ULL);
}

static uint64_t es_key_hash(const EsMap *m, EsValue key) {
    if (m->keys == ES_ITEMS_TEXT) return es_hash_text(key.t);
    uint64_t bits;
    memcpy(&bits, &key.n, sizeof bits);
    return es_mix(bits);
}

static bool es_key_same(const EsMap *m, EsValue a, EsValue b) {
    if (m->keys == ES_ITEMS_TEXT) return a.t == b.t || es_text_equal(a.t, b.t);
    return a.n == b.n || (isnan(a.n) && isnan(b.n));
}

/* A key as the map keeps it. */
static EsValue es_key(const EsMap *m, EsValue key) {
    if (m->keys == ES_ITEMS_NUMBER) key.n = es_key_number(key.n);
    return key;
}

static void es_value_retain(EsItems kind, EsValue v) {
    if (es_items_counted(kind) && v.o && v.o->refs != ES_IMMORTAL) v.o->refs++;
}

static void es_value_release(EsItems kind, EsValue v) {
    if (es_items_counted(kind) && v.o) es_object_release(v.o);
}

/* --- The table --------------------------------------------------------------------- */

/* Where the key's entry is, or ES_NOT_FOUND. Only looks. */
static size_t es_map_find(const EsMap *m, EsValue key, uint64_t hash) {
    if (!m->index) {
        for (size_t i = 0; i < m->used; i++) {
            if (m->entries[i].hash == hash && es_key_same(m, m->entries[i].key, key)) return i;
        }
        return ES_NOT_FOUND;
    }
    size_t mask = m->slots - 1;
    for (size_t s = (size_t)hash & mask;; s = (s + 1) & mask) {
        uint32_t at = m->index[s];
        if (at == 0) return ES_NOT_FOUND;
        if (at == ES_GONE) continue;
        const EsEntry *e = &m->entries[at - 1];
        if (e->hash == hash && es_key_same(m, e->key, key)) return at - 1;
    }
}

/* Room for `cap` entries. */
static void es_map_grow(EsMap *m, size_t cap) {
    if (cap > SIZE_MAX / sizeof(EsEntry) || cap >= ES_GONE) es_out_of_memory();
    es_memory_take((cap - m->cap) * sizeof(EsEntry));
    EsEntry *entries = realloc(m->entries, cap * sizeof(EsEntry));
    if (!entries) es_out_of_memory();
    m->entries = entries;
    m->cap = cap;
}

/* Puts entry i into the index (which has room). */
static void es_map_index_entry(EsMap *m, size_t i) {
    size_t mask = m->slots - 1;
    size_t s = (size_t)m->entries[i].hash & mask;
    while (m->index[s] != 0 && m->index[s] != ES_GONE) s = (s + 1) & mask;
    if (m->index[s] == 0) m->filled++;
    m->index[s] = (uint32_t)(i + 1);
}

/* Packs the entries (dropping removed ones, keeping the order) and, for a map
 * that isn't small, builds a new index with room to grow. */
static void es_map_reindex(EsMap *m) {
    size_t kept = 0;
    for (size_t i = 0; i < m->used; i++) {
        if (m->entries[i].hash != 0) m->entries[kept++] = m->entries[i];
    }
    m->used = kept;
    size_t slots = 0;
    if (m->len > ES_MAP_SMALL) {
        slots = 16;
        while (slots < 2 * m->len + 2) slots *= 2;
    }
    if (slots != m->slots) {
        es_memory.live_bytes -= m->slots * sizeof(uint32_t);
        es_memory_take(slots * sizeof(uint32_t));
        free(m->index);
        m->index = NULL;
        if (slots > 0) {
            m->index = calloc(slots, sizeof(uint32_t));
            if (!m->index) es_out_of_memory();
        }
        m->slots = slots;
    } else if (slots > 0) {
        memset(m->index, 0, slots * sizeof(uint32_t));
    }
    m->filled = 0;
    for (size_t i = 0; m->index && i < m->used; i++) es_map_index_entry(m, i);
}

/* Adds a key that isn't in the map yet, with its value (taking both). */
static void es_map_add(EsMap *m, EsValue key, uint64_t hash, EsValue value) {
    if (m->used == m->cap && m->used - m->len > m->used / 4) es_map_reindex(m); /* many removed: pack */
    if (m->used == m->cap) es_map_grow(m, m->cap ? 2 * m->cap : 4);
    EsEntry *e = &m->entries[m->used];
    e->hash = hash;
    e->key = key;
    e->value = value;
    m->used++;
    m->len++;
    if (!m->index) {
        if (m->used > ES_MAP_SMALL) es_map_reindex(m);
        return;
    }
    es_map_index_entry(m, m->used - 1);
    if (3 * m->filled > 2 * m->slots) es_map_reindex(m);
}

/* Sets the key's value, adding the key if it isn't there (taking both). */
static void es_map_store(EsMap *m, EsValue key, EsValue value) {
    key = es_key(m, key);
    uint64_t hash = es_key_hash(m, key);
    size_t i = es_map_find(m, key, hash);
    if (i == ES_NOT_FOUND) {
        es_map_add(m, key, hash, value);
        return;
    }
    EsValue old = m->entries[i].value;
    m->entries[i].value = value;
    es_value_release(m->values, old);
    es_value_release(m->keys, key);
}

/* Takes the key's entry out; false if it isn't there. Only looks at key. */
static bool es_map_take_out(EsMap *m, EsValue key) {
    uint64_t hash = es_key_hash(m, key);
    size_t i = es_map_find(m, key, hash);
    if (i == ES_NOT_FOUND) return false;
    if (m->index) {
        size_t mask = m->slots - 1;
        size_t s = (size_t)hash & mask;
        while (m->index[s] != i + 1) s = (s + 1) & mask;
        m->index[s] = ES_GONE;
    }
    EsEntry *e = &m->entries[i];
    EsValue old_key = e->key, old_value = e->value;
    e->hash = 0;
    m->len--;
    while (m->used > 0 && m->entries[m->used - 1].hash == 0) m->used--;
    es_value_release(m->keys, old_key);
    es_value_release(m->values, old_value);
    return true;
}

/* --- Making maps ------------------------------------------------------------------- */

static EsMap *es_map_new(EsItems keys, EsItems values, size_t cap) {
    EsMap *m = (EsMap *)es_new_object(ES_OBJ_MAP, sizeof(EsMap));
    m->keys = keys;
    m->values = values;
    m->len = m->used = m->cap = 0;
    m->entries = NULL;
    m->index = NULL;
    m->slots = m->filled = 0;
    if (cap > 0) es_map_grow(m, cap);
    return m;
}

/* "a map of K1 to V1, K2 to V2": n keys and values, one after the other
 * (taking the text, lists, records and maps among them). A key given twice
 * keeps its first place and its last value. */
static EsMap *es_map_from(EsItems keys, EsItems values, size_t n, const EsValue *pairs) {
    EsMap *m = es_map_new(keys, values, n);
    for (size_t i = 0; i < n; i++) es_map_store(m, pairs[2 * i], pairs[2 * i + 1]);
    return m;
}

/* A fixed map (a constant): marked immortal, so it's never counted, freed,
 * or changed (the compiler rejects changes and copies it wherever it could
 * be kept and changed). Its keys and values are immortal text, numbers or yes/no. */
static EsMap *es_map_fix(EsMap *m) {
    es_memory.live_bytes -= es_map_bytes(m);
    es_memory.live_objects--;
    m->header.refs = ES_IMMORTAL;
    return m;
}

/* A new map with the same entries, in order. When `deep`, lists and maps in
 * it are copied too; text and records are always shared. Only looks. */
static EsMap *es_map_copy_with(const EsMap *m, bool deep) {
    EsMap *copy = es_map_new(m->keys, m->values, m->len);
    for (size_t i = 0; i < m->used; i++) {
        const EsEntry *e = &m->entries[i];
        if (e->hash == 0) continue;
        EsValue value = e->value;
        if (deep && es_items_copied(m->values)) {
            value.o = es_copy_inner(m->values, value.o);
        } else {
            es_value_retain(m->values, value);
        }
        es_value_retain(m->keys, e->key);
        copy->entries[copy->used].hash = e->hash;
        copy->entries[copy->used].key = e->key;
        copy->entries[copy->used].value = value;
        copy->used++;
    }
    copy->len = copy->used;
    if (copy->len > ES_MAP_SMALL) es_map_reindex(copy);
    return copy;
}

/* A copy that shares nothing that could change. Only looks. */
static EsMap *es_map_copy_of(const EsMap *m) {
    return es_map_copy_with(m, true);
}

/* Makes every list and map inside m belong to m alone (see es_list_own_deep). */
static void es_map_own_deep(EsMap *m) {
    if (!es_items_copied(m->values)) return;
    for (size_t i = 0; i < m->used; i++) {
        if (m->entries[i].hash == 0) continue;
        EsObject *inner = m->entries[i].value.o;
        if (inner->refs != 1) {
            m->entries[i].value.o = es_copy_inner(m->values, inner);
            es_object_release(inner);
        } else if (m->values == ES_ITEMS_LIST) {
            es_list_own_deep((EsList *)inner);
        } else {
            es_map_own_deep((EsMap *)inner);
        }
    }
}

/* "a copy of M". A map nobody else holds (a value just worked out) is
 * already a copy, so it's reused. */
static EsMap *es_map_copy(EsMap *m) {
    if (m->header.refs == 1) {
        es_map_own_deep(m);
        return m;
    }
    EsMap *copy = es_map_copy_of(m);
    es_map_release(m);
    return copy;
}

/* The map as it is now, for a "for each" loop to go through: changes the
 * loop makes don't affect which entries it visits. A map nobody else holds
 * (or a fixed one) can't change, so it's given back as it is. */
static EsMap *es_map_snapshot(EsMap *m) {
    if (m->header.refs == 1 || m->header.refs == ES_IMMORTAL) return m;
    EsMap *copy = es_map_copy_with(m, false);
    es_map_release(m);
    return copy;
}

/* --- Values ----------------------------------------------------------------------- */

static const char *const es_hint_missing =
    "To check first, use \"has\". To use another value when it's missing, add \", or\" and that value after it.";

static _Noreturn void es_map_missing(int site, const EsMap *m, EsValue key, const char *hint, const char *before,
                                     const char *after) {
    if (m->keys == ES_ITEMS_NUMBER) es_fail(site, hint, "%s%s%s", before, es_shown(key.n), after);
    const EsText *t = key.t;
    size_t shown = 0;
    for (size_t chars = 0; shown < t->len && chars < 60; chars++) {
        shown++;
        while (shown < t->len && es_continuation_byte(t->chars[shown])) shown++;
    }
    es_fail(site, hint, "%s\"%.*s%s\"%s", before, (int)shown, t->chars, shown < t->len ? "..." : "", after);
}

/* The key's entry; a runtime error if it isn't there. Only looks. */
static EsEntry *es_map_entry(int site, EsMap *m, EsValue key, const char *hint) {
    size_t i = es_map_find(m, key, es_key_hash(m, key));
    if (i == ES_NOT_FOUND) es_map_missing(site, m, key, hint, "There's no value for ", " in the map.");
    return &m->entries[i];
}

/* get: "the value for K in M" (owned). lookup: the same, but false instead
 * of an error when K isn't there, for ", or D". put: "set the value for K in
 * M to V" (takes V). */
#define ES_MAP_OPS(S, T, FIELD, RETAIN)                                          \
    static T es_map_get_##S(int site, EsMap *m, EsValue key) {                   \
        key = es_key(m, key);                                                    \
        T v = RETAIN(es_map_entry(site, m, key, es_hint_missing)->value.FIELD);  \
        es_value_release(m->keys, key);                                          \
        es_map_release(m);                                                       \
        return v;                                                                \
    }                                                                            \
    static bool es_map_lookup_##S(EsMap *m, EsValue key, T *out) {               \
        key = es_key(m, key);                                                    \
        size_t i = es_map_find(m, key, es_key_hash(m, key));                     \
        if (i != ES_NOT_FOUND) *out = RETAIN(m->entries[i].value.FIELD);         \
        es_value_release(m->keys, key);                                          \
        es_map_release(m);                                                       \
        return i != ES_NOT_FOUND;                                                \
    }                                                                            \
    static void es_map_put_##S(EsMap *m, EsValue key, T v) {                     \
        EsValue value = {0};                                                     \
        value.FIELD = v;                                                         \
        es_map_store(m, key, value);                                             \
        es_map_release(m);                                                       \
    }

ES_MAP_OPS(n, double, n, ES_SAME)
ES_MAP_OPS(b, bool, b, ES_SAME)
ES_MAP_OPS(t, EsText *, t, es_retain)
ES_MAP_OPS(l, EsList *, l, es_list_retain)
ES_MAP_OPS(r, EsRecord *, r, es_record_retain)
ES_MAP_OPS(m, EsMap *, m, es_map_retain)

/* "add 1 to the value for K in M" and the other changes: where the value
 * is, to change it in place (it has to be there). Only looks at m. */
static EsValue *es_map_at(int site, EsMap *m, EsValue key) {
    key = es_key(m, key);
    EsEntry *e = es_map_entry(site, m, key, "It has to be in the map before it can change. To put it there, write "
                                            "\"set the value for ... to ...\".");
    es_value_release(m->keys, key);
    return &e->value;
}

/* "M has K" */
static bool es_map_has(EsMap *m, EsValue key) {
    key = es_key(m, key);
    bool found = es_map_find(m, key, es_key_hash(m, key)) != ES_NOT_FOUND;
    es_value_release(m->keys, key);
    es_map_release(m);
    return found;
}

/* "remove K from M": it has to be there. */
static void es_map_remove(int site, EsMap *m, EsValue key) {
    key = es_key(m, key);
    if (!es_map_take_out(m, key)) {
        es_map_missing(site, m, key, "To check first, use \"has\".", "I can't remove ",
                       ": the map has no value for it.");
    }
    es_value_release(m->keys, key);
    es_map_release(m);
}

/* "M without K": a new map (reusing M when nobody else holds it), with copies
 * of the lists and maps inside it, like "a copy of". K has to be there. */
static EsMap *es_map_without(int site, EsMap *m, EsValue key) {
    EsMap *r = es_map_copy(m);
    key = es_key(r, key);
    if (!es_map_take_out(r, key)) {
        es_map_missing(site, r, key, "To check first, use \"has\".", "I can't leave out ", ": the map has no value for it.");
    }
    es_value_release(r->keys, key);
    return r;
}

/* --- Questions -------------------------------------------------------------------- */

static double es_map_length(EsMap *m) {
    size_t len = m->len;
    es_map_release(m);
    return (double)len;
}

static bool es_map_is_empty(EsMap *m) {
    bool empty = m->len == 0;
    es_map_release(m);
    return empty;
}

/* Adds v to the end of a list being built (which has room): text and
 * records are shared, lists and maps copied. Only looks at v. */
static void es_list_put_value(EsList *l, EsValue v) {
    size_t k = l->len++;
    switch (l->items) {
    case ES_ITEMS_NUMBER: l->at.n[k] = v.n; break;
    case ES_ITEMS_YESNO: l->at.b[k] = v.b; break;
    case ES_ITEMS_TEXT: l->at.t[k] = es_retain(v.t); break;
    case ES_ITEMS_RECORD: l->at.r[k] = es_record_retain(v.r); break;
    case ES_ITEMS_LIST:
    case ES_ITEMS_MAP: l->at.o[k] = es_copy_inner(l->items, v.o); break;
    }
}

/* "the keys of M" / "the values of M": a new list, in the map's order. */
static EsList *es_map_list(EsMap *m, bool keys) {
    EsList *l = es_list_new(keys ? m->keys : m->values, m->len);
    for (size_t i = 0; i < m->used; i++) {
        if (m->entries[i].hash != 0) es_list_put_value(l, keys ? m->entries[i].key : m->entries[i].value);
    }
    es_map_release(m);
    return l;
}

static bool es_value_equal(EsItems kind, EsValue a, EsValue b) {
    switch (kind) {
    case ES_ITEMS_NUMBER: return es_close(a.n, b.n);
    case ES_ITEMS_YESNO: return a.b == b.b;
    case ES_ITEMS_TEXT: return es_text_equal(a.t, b.t);
    case ES_ITEMS_LIST: return es_list_equal(a.l, b.l);
    case ES_ITEMS_RECORD: return a.r == b.r; /* the same record */
    case ES_ITEMS_MAP: return es_map_equal(a.m, b.m);
    }
    return false;
}

/* The same keys with equal values (compared like "is" compares them), in
 * any order. Only looks. */
static bool es_map_equal(const EsMap *a, const EsMap *b) {
    if (a == b) return true;
    if (a->len != b->len) return false;
    for (size_t i = 0; i < a->used; i++) {
        const EsEntry *e = &a->entries[i];
        if (e->hash == 0) continue;
        size_t j = es_map_find(b, e->key, e->hash);
        if (j == ES_NOT_FOUND || !es_value_equal(a->values, e->value, b->entries[j].value)) return false;
    }
    return true;
}

static bool es_map_same(EsMap *a, EsMap *b) {
    bool same = es_map_equal(a, b);
    es_map_release(a);
    es_map_release(b);
    return same;
}

/* --- As text ----------------------------------------------------------------------- */

/* A key or value inside a map: text in quotes; lists, maps and records in
 * parentheses. Only looks. */
static void es_value_render(EsBuf *b, EsItems kind, EsValue v, int depth) {
    switch (kind) {
    case ES_ITEMS_NUMBER: {
        EsText *t = es_number_text(v.n);
        es_buf_add(b, t->chars, t->len);
        es_release(t);
        break;
    }
    case ES_ITEMS_YESNO: es_buf_add_text(b, v.b ? "yes" : "no"); break;
    case ES_ITEMS_TEXT:
        es_buf_add(b, "\"", 1);
        es_buf_add(b, v.t->chars, v.t->len);
        es_buf_add(b, "\"", 1);
        break;
    case ES_ITEMS_LIST:
        es_buf_add(b, "(", 1);
        if (v.l->len == 0) es_buf_add_text(b, "an empty list");
        es_list_render(b, v.l, depth);
        es_buf_add(b, ")", 1);
        break;
    case ES_ITEMS_RECORD:
        if (v.r) es_buf_add(b, "(", 1);
        es_record_render(b, v.r, depth);
        if (v.r) es_buf_add(b, ")", 1);
        break;
    case ES_ITEMS_MAP:
        es_buf_add(b, "(", 1);
        if (v.m->len == 0) es_buf_add_text(b, "an empty map");
        es_map_render(b, v.m, depth);
        es_buf_add(b, ")", 1);
        break;
    }
}

/* `"Paris" is 19 and "Junie" is 24`; an empty map is empty text. Only looks. */
static void es_map_render(EsBuf *b, const EsMap *m, int depth) {
    size_t shown = 0;
    for (size_t i = 0; i < m->used; i++) {
        const EsEntry *e = &m->entries[i];
        if (e->hash == 0) continue;
        if (shown > 0) es_buf_add_text(b, shown + 1 == m->len ? " and " : ", ");
        es_value_render(b, m->keys, e->key, depth);
        es_buf_add_text(b, " is ");
        es_value_render(b, m->values, e->value, depth);
        shown++;
    }
}

/* "say ages" */
static EsText *es_map_text(EsMap *m) {
    EsBuf b = {NULL, 0, 0};
    es_map_render(&b, m, 0);
    es_map_release(m);
    return es_buf_text(&b);
}

#endif
