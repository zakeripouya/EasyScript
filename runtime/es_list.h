/* EasyScript runtime, part 4: lists. Parts 1 to 3 (es_memory.h,
 * es_program.h, es_runtime.h) come first.
 *
 * A list (EsList, in es_memory.h) holds one kind of item in an array that
 * grows; text and lists inside it are counted references it owns. Lists are
 * shared: every variable that holds the same list sees its changes. Like the
 * rest of the runtime, every function here consumes the lists and text it's
 * given (releases them or hands them back), except es_push_* (the caller is
 * still building the list) and the helpers marked "only looks".
 *
 * Positions are 1-based, as in English. The mistakes only the values can show
 * (a position past the end, the biggest item of an empty list) stop the
 * program with its line, like the other runtime errors.
 *
 * Operations on items come in six versions, one per kind of item: _n
 * (number), _b (yes/no), _t (text), _l (list), _r (record; NULL for
 * nothing), _m (map). Records in a list are shared, never copied; lists and
 * maps in a list belong to it, so copies and slices copy them too. */
#ifndef ES_LIST_H
#define ES_LIST_H

#ifndef ES_RUNTIME_H
#include "es_runtime.h"
#endif

/* Maps (es_map.h), for lists that hold maps. */
static EsMap *es_map_copy_of(const EsMap *m);
static void es_map_own_deep(EsMap *m);
static bool es_map_equal(const EsMap *a, const EsMap *b);

/* --- Making lists ------------------------------------------------------------ */

/* Room for exactly `cap` items (at least the ones it has). */
static void es_list_resize(EsList *l, size_t cap) {
    size_t size = es_item_size(l->items);
    if (cap > SIZE_MAX / size) es_out_of_memory();
    es_memory_take((cap - l->cap) * size);
    void *data = realloc(l->at.any, cap * size);
    if (!data) es_out_of_memory();
    l->at.any = data;
    l->cap = cap;
}

/* Room for at least `need` items, doubling each time. (Growing by 1.5x was
 * tried: on macOS, realloc then moves the last big block with a deferred
 * copy that's paid at the next big write, such as sorting, so the sort
 * benchmark peaked 5 MB higher. Doubling lets realloc grow in place there.) */
static void es_list_reserve(EsList *l, size_t need) {
    if (need <= l->cap) return;
    size_t cap = l->cap ? l->cap * 2 : 4;
    if (cap < need) cap = need;
    es_list_resize(l, cap);
}

/* Gives back the room the list doesn't use, so it takes exactly its size. */
static void es_list_shrink(EsList *l) {
    if (l->cap == l->len) return;
    size_t size = es_item_size(l->items);
    es_memory.live_bytes -= (l->cap - l->len) * size;
    if (l->len == 0) {
        free(l->at.any);
        l->at.any = NULL;
    } else {
        void *data = realloc(l->at.any, l->len * size);
        if (data) l->at.any = data; /* if it can't shrink, it keeps the old block */
    }
    l->cap = l->len;
}

static EsList *es_list_new(EsItems items, size_t cap) {
    EsList *l = (EsList *)es_new_object(ES_OBJ_LIST, sizeof(EsList));
    l->items = items;
    l->len = 0;
    l->cap = 0;
    l->at.any = NULL;
    es_list_reserve(l, cap);
    return l;
}

/* "a list of A, B and C": the n items (taking the text and lists among them). */
static EsList *es_list_from(EsItems items, size_t n, const void *values) {
    EsList *l = es_list_new(items, n);
    if (n > 0) memcpy(l->at.any, values, n * es_item_size(items));
    l->len = n;
    return l;
}

/* A new list with the same items, sharing the text and lists in it. Only looks at l. */
static EsList *es_list_shallow_copy(const EsList *l) {
    EsList *copy = es_list_new(l->items, l->len);
    if (l->len > 0) memcpy(copy->at.any, l->at.any, l->len * es_item_size(l->items));
    copy->len = l->len;
    for (size_t i = 0; es_items_counted(l->items) && i < l->len; i++) {
        if (copy->at.o[i] && copy->at.o[i]->refs != ES_IMMORTAL) copy->at.o[i]->refs++;
    }
    return copy;
}

/* The list as it is now, for a loop to go through: changes the loop makes to
 * the list don't affect which items it visits. A list nobody else holds is
 * given back as it is, because nobody could change it. */
static EsList *es_list_snapshot(EsList *l) {
    if (l->header.refs == 1) return l;
    EsList *copy = es_list_shallow_copy(l);
    es_list_release(l);
    return copy;
}

/* "a copy of L": a new list, with copies of the lists and maps inside it
 * too, so changing the copy never changes the original. Only looks at l. */
static EsList *es_list_copy_of(const EsList *l) {
    if (!es_items_copied(l->items)) return es_list_shallow_copy(l);
    EsList *copy = es_list_new(l->items, l->len);
    for (size_t i = 0; i < l->len; i++) {
        if (l->items == ES_ITEMS_LIST) {
            copy->at.l[i] = es_list_copy_of(l->at.l[i]);
        } else {
            copy->at.m[i] = es_map_copy_of(l->at.m[i]);
        }
    }
    copy->len = l->len;
    return copy;
}

/* A copy of a list or map that belongs to a list or map. Only looks. */
static EsObject *es_copy_inner(EsItems items, const EsObject *inner) {
    if (items == ES_ITEMS_LIST) return &es_list_copy_of((const EsList *)inner)->header;
    return &es_map_copy_of((const EsMap *)inner)->header;
}

/* Makes every list and map inside l (and inside those) belong to l alone,
 * copying the ones something else holds: for reusing a list nobody else
 * holds while keeping the promise that copies and slices share no lists. */
static void es_list_own_deep(EsList *l) {
    if (!es_items_copied(l->items)) return;
    for (size_t i = 0; i < l->len; i++) {
        EsObject *inner = l->at.o[i];
        if (inner->refs != 1) {
            l->at.o[i] = es_copy_inner(l->items, inner);
            es_object_release(inner);
        } else if (l->items == ES_ITEMS_LIST) {
            es_list_own_deep(l->at.l[i]);
        } else {
            es_map_own_deep(l->at.m[i]);
        }
    }
}

/* "a copy of L". A list nobody else holds (a value just worked out) is
 * already a copy, so it's reused. */
static EsList *es_list_copy(EsList *l) {
    if (l->header.refs == 1) {
        es_list_own_deep(l);
        return l;
    }
    EsList *copy = es_list_copy_of(l);
    es_list_release(l);
    return copy;
}

/* --- Slices ------------------------------------------------------------------
 *
 * Every slice is a new list. Text in it is shared (text never changes); lists
 * in it are copied, so changing a slice never changes the list it came from. */

/* `count` items of l, starting at index `start`, `step` apart. Only looks. */
static EsList *es_list_pick(const EsList *l, size_t start, size_t count, size_t step) {
    EsList *out = es_list_new(l->items, count);
    size_t size = es_item_size(l->items);
    for (size_t k = 0; k < count; k++) {
        size_t i = start + k * step;
        if (es_items_copied(l->items)) {
            out->at.o[k] = es_copy_inner(l->items, l->at.o[i]);
        } else {
            memcpy((char *)out->at.any + k * size, (const char *)l->at.any + i * size, size);
            if (l->items == ES_ITEMS_TEXT) es_retain(out->at.t[k]);
            if (l->items == ES_ITEMS_RECORD) es_record_retain(out->at.r[k]);
        }
    }
    out->len = count;
    return out;
}

/* Adds copies of src's items to the end of dst (which has room): text is
 * shared, lists inside are copied. Only looks at src. */
static void es_list_append_copies(EsList *dst, const EsList *src) {
    size_t size = es_item_size(src->items);
    for (size_t i = 0; i < src->len; i++, dst->len++) {
        if (es_items_copied(src->items)) {
            dst->at.o[dst->len] = es_copy_inner(src->items, src->at.o[i]);
        } else {
            memcpy((char *)dst->at.any + dst->len * size, (const char *)src->at.any + i * size, size);
            if (src->items == ES_ITEMS_TEXT) es_retain(dst->at.t[dst->len]);
            if (src->items == ES_ITEMS_RECORD) es_record_retain(dst->at.r[dst->len]);
        }
    }
}

/* "A followed by B" for two lists: a new list of exactly A's items then
 * B's, sharing no lists with them. If nobody else holds A, A grows into it. */
static EsList *es_list_join(EsList *a, EsList *b) {
    EsList *joined;
    if (a->header.refs == 1) {
        joined = a;
        es_list_own_deep(joined);
        if (joined->cap < a->len + b->len) es_list_resize(joined, a->len + b->len);
    } else {
        joined = es_list_new(a->items, a->len + b->len);
        es_list_append_copies(joined, a);
        es_list_release(a);
    }
    es_list_append_copies(joined, b);
    es_list_release(b);
    return joined;
}

/* --- Positions ---------------------------------------------------------------- */

enum { ES_AT, ES_FIRST, ES_LAST }; /* item N, the first item, the last item */

static _Noreturn void es_bad_position(int site, double pos) {
    es_fail(site, NULL, "The position of an item has to be a whole number, but it's %s.", es_shown(pos));
}

/* The index (from 0) of item `pos`, the first item, or the last item. */
static size_t es_index(int site, const EsList *l, int where, double pos) {
    if (where != ES_AT) {
        if (l->len == 0) es_fail(site, NULL, "There's no %s item: the list is empty.", where == ES_FIRST ? "first" : "last");
        return where == ES_FIRST ? 0 : l->len - 1;
    }
    if (pos != floor(pos)) es_bad_position(site, pos);
    if (!(pos >= 1 && pos <= (double)l->len)) {
        const char *hint = pos < 1 ? "Positions start at 1: the first item is item 1." : NULL;
        if (l->len == 0) es_fail(site, hint, "There's no item %s in the list: it's empty.", es_shown(pos));
        es_fail(site, hint, "There's no item %s in the list: it has %zu item%s.", es_shown(pos), l->len,
                l->len == 1 ? "" : "s");
    }
    return (size_t)pos - 1;
}

/* Where "insert ... at position N" puts the item: 1 to one past the end. */
static size_t es_insert_index(int site, const EsList *l, double pos) {
    if (pos != floor(pos)) es_bad_position(site, pos);
    if (!(pos >= 1 && pos <= (double)l->len + 1)) {
        es_fail(site, NULL, "I can't insert at position %s: the position has to be from 1 to %zu (the list has %zu item%s).",
                es_shown(pos), l->len + 1, l->len, l->len == 1 ? "" : "s");
    }
    return (size_t)pos - 1;
}

/* The slice of `count` items from index `start`, `step` apart, as a new list
 * of exactly that size. If nobody else holds l, it becomes the slice itself:
 * the other items are released and the kept ones moved down. */
static EsList *es_list_keep(EsList *l, size_t start, size_t count, size_t step) {
    if (l->header.refs != 1) {
        EsList *slice = es_list_pick(l, start, count, step);
        es_list_release(l);
        return slice;
    }
    size_t size = es_item_size(l->items);
    for (size_t i = 0; es_items_counted(l->items) && i < l->len; i++) {
        bool kept = i >= start && (i - start) % step == 0 && (i - start) / step < count;
        if (!kept && l->at.o[i]) es_object_release(l->at.o[i]);
    }
    char *base = l->at.any;
    for (size_t k = 0; k < count; k++) memmove(base + k * size, base + (start + k * step) * size, size);
    l->len = count;
    es_list_own_deep(l);
    es_list_shrink(l);
    return l;
}

/* "items FROM to TO of L" (to_end: "to the end"). Both positions must be in
 * the list, and FROM can't be after TO. */
static EsList *es_slice_range(int site, EsList *l, double from, double to, bool to_end) {
    size_t a = es_index(site, l, ES_AT, from);
    size_t b = to_end ? l->len - 1 : es_index(site, l, ES_AT, to);
    if (a > b) {
        es_fail(site, NULL, "I can't take items %zu to %zu: the first position is after the last one.", a + 1, b + 1);
    }
    return es_list_keep(l, a, b - a + 1, 1);
}

/* "the first N items" / "the last N items": all of them when there are fewer. */
static EsList *es_slice_end(int site, EsList *l, double n, bool last) {
    if (n < 0) es_fail(site, NULL, "The number of items can't be negative, but it's %s.", es_shown(n));
    if (n != floor(n)) es_fail(site, NULL, "The number of items has to be a whole number, but it's %s.", es_shown(n));
    size_t count = n >= (double)l->len ? l->len : (size_t)n;
    return es_list_keep(l, last ? l->len - count : 0, count, 1);
}

/* "L without the first item" / "L without the last item" */
static EsList *es_slice_without(int site, EsList *l, bool last) {
    es_index(site, l, last ? ES_LAST : ES_FIRST, 0); /* there has to be one */
    return es_list_keep(l, last ? 0 : 1, l->len - 1, 1);
}

/* "every second item of L": the items at positions step, 2 * step, ...;
 * "every other item of L": 1, 3, 5, ... (start is the first position). */
static EsList *es_slice_every(EsList *l, size_t step, size_t start) {
    size_t count = l->len >= start ? (l->len - start) / step + 1 : 0;
    return es_list_keep(l, start - 1, count, step);
}

/* --- Items -------------------------------------------------------------------- */

#define ES_SAME(x) (x)
#define ES_IGNORE(x) ((void)(x))

/* get: item N's value (owned). put: "set item N of L to V" (takes V).
 * push: adds to the end of a list being built. add: "add V to the end of L".
 * insert: "insert V at position N of L". */
#define ES_ITEM_OPS(S, T, FIELD, RETAIN, RELEASE)                                  \
    static T es_get_##S(int site, EsList *l, int where, double pos) {               \
        T v = RETAIN(l->at.FIELD[es_index(site, l, where, pos)]);                    \
        es_list_release(l);                                                          \
        return v;                                                                    \
    }                                                                                \
    static void es_put_##S(int site, EsList *l, int where, double pos, T v) {        \
        T *slot = &l->at.FIELD[es_index(site, l, where, pos)];                       \
        T old = *slot;                                                               \
        *slot = v;                                                                   \
        RELEASE(old);                                                                \
        es_list_release(l);                                                          \
    }                                                                                \
    static void es_push_##S(EsList *l, T v) {                                        \
        es_list_reserve(l, l->len + 1);                                              \
        l->at.FIELD[l->len++] = v;                                                   \
    }                                                                                \
    static void es_add_##S(EsList *l, T v) {                                         \
        es_push_##S(l, v);                                                           \
        es_list_release(l);                                                          \
    }                                                                                \
    static void es_insert_##S(int site, EsList *l, double pos, T v) {                \
        size_t i = es_insert_index(site, l, pos);                                    \
        es_list_reserve(l, l->len + 1);                                              \
        memmove(&l->at.FIELD[i + 1], &l->at.FIELD[i], (l->len - i) * sizeof(T));    \
        l->at.FIELD[i] = v;                                                          \
        l->len++;                                                                    \
        es_list_release(l);                                                          \
    }

ES_ITEM_OPS(n, double, n, ES_SAME, ES_IGNORE)
ES_ITEM_OPS(b, bool, b, ES_SAME, ES_IGNORE)
ES_ITEM_OPS(t, EsText *, t, es_retain, es_release)
ES_ITEM_OPS(l, EsList *, l, es_list_retain, es_list_release)
ES_ITEM_OPS(r, EsRecord *, r, es_record_retain, es_record_release)
ES_ITEM_OPS(m, EsMap *, m, es_map_retain, es_map_release)

/* "remove item N from L" (or the first or last item). */
static void es_remove(int site, EsList *l, int where, double pos) {
    size_t i = es_index(site, l, where, pos);
    size_t size = es_item_size(l->items);
    if (es_items_counted(l->items) && l->at.o[i]) es_object_release(l->at.o[i]);
    char *base = l->at.any;
    memmove(base + i * size, base + (i + 1) * size, (l->len - i - 1) * size);
    l->len--;
    es_list_release(l);
}

/* "L without item N": a new list (reusing L when nobody else holds it), with
 * copies of the lists inside it, like the other slices. */
static EsList *es_slice_without_at(int site, EsList *l, double pos) {
    es_index(site, l, ES_AT, pos); /* it has to be there */
    EsList *r = es_list_copy(l);
    es_remove(site, es_list_retain(r), ES_AT, pos);
    return r;
}

/* --- Questions ----------------------------------------------------------------- */

static double es_list_length(EsList *l) {
    size_t len = l->len;
    es_list_release(l);
    return (double)len;
}

static bool es_list_is_empty(EsList *l) {
    bool empty = l->len == 0;
    es_list_release(l);
    return empty;
}

static bool es_text_equal(const EsText *a, const EsText *b) { /* only looks */
    return a->len == b->len && memcmp(a->chars, b->chars, a->len) == 0;
}

/* Same items in the same order, compared like "is" compares them (numbers
 * with es_close). Only looks. */
static bool es_list_equal(const EsList *a, const EsList *b) {
    if (a == b) return true;
    if (a->len != b->len) return false;
    for (size_t i = 0; i < a->len; i++) {
        bool same = true;
        switch (a->items) {
        case ES_ITEMS_NUMBER: same = es_close(a->at.n[i], b->at.n[i]); break;
        case ES_ITEMS_YESNO: same = a->at.b[i] == b->at.b[i]; break;
        case ES_ITEMS_TEXT: same = es_text_equal(a->at.t[i], b->at.t[i]); break;
        case ES_ITEMS_LIST: same = es_list_equal(a->at.l[i], b->at.l[i]); break;
        case ES_ITEMS_RECORD: same = a->at.r[i] == b->at.r[i]; break; /* the same record */
        case ES_ITEMS_MAP: same = es_map_equal(a->at.m[i], b->at.m[i]); break;
        }
        if (!same) return false;
    }
    return true;
}

static bool es_list_same(EsList *a, EsList *b) {
    bool same = es_list_equal(a, b);
    es_list_release(a);
    es_list_release(b);
    return same;
}

/* "L contains X" */
static bool es_contains_n(EsList *l, double v) {
    bool found = false;
    for (size_t i = 0; i < l->len && !found; i++) found = es_close(l->at.n[i], v);
    es_list_release(l);
    return found;
}

static bool es_contains_b(EsList *l, bool v) {
    bool found = false;
    for (size_t i = 0; i < l->len && !found; i++) found = l->at.b[i] == v;
    es_list_release(l);
    return found;
}

static bool es_contains_t(EsList *l, EsText *v) {
    bool found = false;
    for (size_t i = 0; i < l->len && !found; i++) found = es_text_equal(l->at.t[i], v);
    es_release(v);
    es_list_release(l);
    return found;
}

static bool es_contains_l(EsList *l, EsList *v) {
    bool found = false;
    for (size_t i = 0; i < l->len && !found; i++) found = es_list_equal(l->at.l[i], v);
    es_list_release(v);
    es_list_release(l);
    return found;
}

static bool es_contains_m(EsList *l, EsMap *v) {
    bool found = false;
    for (size_t i = 0; i < l->len && !found; i++) found = es_map_equal(l->at.m[i], v);
    es_map_release(v);
    es_list_release(l);
    return found;
}

/* A record is in the list when that same record is (not a record with the same fields). */
static bool es_contains_r(EsList *l, EsRecord *v) {
    bool found = false;
    for (size_t i = 0; i < l->len && !found; i++) found = l->at.r[i] == v;
    es_record_release(v);
    es_list_release(l);
    return found;
}

/* "the position of X in L": the position of the first item equal to X, or 0
 * if none is. */
#define ES_POSITION(S, T, SAME, RELEASE)                                   \
    static double es_position_##S(T v, EsList *l) {                        \
        size_t i = 0;                                                      \
        while (i < l->len && !SAME(l->at.S[i], v)) i++;                    \
        double position = i < l->len ? (double)i + 1 : 0;                  \
        RELEASE(v);                                                        \
        es_list_release(l);                                                \
        return position;                                                   \
    }

#define ES_SAME_YESNO(a, b) ((a) == (b))
ES_POSITION(n, double, es_close, ES_IGNORE)
ES_POSITION(b, bool, ES_SAME_YESNO, ES_IGNORE)
ES_POSITION(t, EsText *, es_text_equal, es_release)
ES_POSITION(l, EsList *, es_list_equal, es_list_release)
ES_POSITION(r, EsRecord *, ES_SAME_YESNO, es_record_release)
ES_POSITION(m, EsMap *, es_map_equal, es_map_release)

/* "L added together" */
static double es_list_sum(EsList *l) {
    double total = 0;
    for (size_t i = 0; i < l->len; i++) total += l->at.n[i];
    es_list_release(l);
    return total;
}

static inline bool es_text_less(const EsText *a, const EsText *b) { /* alphabetical, by bytes; only looks */
    size_t n = a->len < b->len ? a->len : b->len;
    int c = memcmp(a->chars, b->chars, n);
    return c != 0 ? c < 0 : a->len < b->len;
}

/* "the biggest of L" / "the smallest of L": the first such item. */
static double es_extreme_n(int site, EsList *l, bool biggest) {
    if (l->len == 0) es_fail(site, NULL, "There's no %s item: the list is empty.", biggest ? "biggest" : "smallest");
    double best = l->at.n[0];
    for (size_t i = 1; i < l->len; i++) {
        double v = l->at.n[i];
        if (biggest ? v > best : v < best) best = v;
    }
    es_list_release(l);
    return best;
}

static EsText *es_extreme_t(int site, EsList *l, bool biggest) {
    if (l->len == 0) es_fail(site, NULL, "There's no %s item: the list is empty.", biggest ? "biggest" : "smallest");
    EsText *best = l->at.t[0];
    for (size_t i = 1; i < l->len; i++) {
        EsText *v = l->at.t[i];
        if (biggest ? es_text_less(best, v) : es_text_less(v, best)) best = v;
    }
    es_retain(best);
    es_list_release(l);
    return best;
}

/* "L reversed": a new list, unless nobody else holds this one. */
static EsList *es_list_reversed(EsList *l) {
    EsList *r = es_list_copy(l); /* lists inside belong to the new list */
    size_t size = es_item_size(r->items);
    char *a = r->at.any;
    for (size_t i = 0, j = r->len; i + 1 < j; i++, j--) {
        char tmp[sizeof(double)]; /* the biggest item */
        memcpy(tmp, a + i * size, size);
        memcpy(a + i * size, a + (j - 1) * size, size);
        memcpy(a + (j - 1) * size, tmp, size);
    }
    return r;
}

/* --- As text ------------------------------------------------------------------- */

typedef struct {
    char *data;
    size_t len, cap;
} EsBuf;

static void es_buf_add(EsBuf *b, const char *s, size_t n) {
    if (n == 0) return; /* nothing to add (and b->data may still be NULL) */
    if (b->len + n > b->cap) {
        size_t cap = b->cap ? b->cap * 2 : 64;
        while (cap < b->len + n) cap *= 2;
        char *data = realloc(b->data, cap);
        if (!data) es_out_of_memory();
        b->data = data;
        b->cap = cap;
    }
    memcpy(b->data + b->len, s, n);
    b->len += n;
}

static void es_record_render(EsBuf *b, const EsRecord *r, int depth); /* es_record.h */
static void es_map_render(EsBuf *b, const EsMap *m, int depth);       /* es_map.h */

/* "72, 95 and 88"; a list inside a list is in parentheses: "(1 and 2) and
 * (3)". `depth` is how many records it's inside (they stop showing their
 * fields after a few levels). */
static void es_list_render(EsBuf *b, const EsList *l, int depth) {
    for (size_t i = 0; i < l->len; i++) {
        if (i > 0) es_buf_add(b, i + 1 == l->len ? " and " : ", ", i + 1 == l->len ? 5 : 2);
        switch (l->items) {
        case ES_ITEMS_NUMBER: {
            EsText *t = es_number_text(l->at.n[i]);
            es_buf_add(b, t->chars, t->len);
            es_release(t);
            break;
        }
        case ES_ITEMS_YESNO: es_buf_add(b, l->at.b[i] ? "yes" : "no", l->at.b[i] ? 3 : 2); break;
        case ES_ITEMS_TEXT: es_buf_add(b, l->at.t[i]->chars, l->at.t[i]->len); break;
        case ES_ITEMS_LIST:
            es_buf_add(b, "(", 1);
            es_list_render(b, l->at.l[i], depth);
            es_buf_add(b, ")", 1);
            break;
        case ES_ITEMS_RECORD:  /* "(a dog with name "Rex" and age 3) and (...)" */
            if (l->at.r[i]) es_buf_add(b, "(", 1);
            es_record_render(b, l->at.r[i], depth);
            if (l->at.r[i]) es_buf_add(b, ")", 1);
            break;
        case ES_ITEMS_MAP: /* "("a" is 1) and ("b" is 2)" */
            es_buf_add(b, "(", 1);
            es_map_render(b, l->at.m[i], depth);
            es_buf_add(b, ")", 1);
            break;
        }
    }
}

static EsText *es_list_text(EsList *l) {
    EsBuf b = {NULL, 0, 0};
    es_list_render(&b, l, 0);
    EsText *t = b.len > 0 ? es_text_copy(b.data, b.len) : &es_empty_text;
    free(b.data);
    es_list_release(l);
    return t;
}

#endif
