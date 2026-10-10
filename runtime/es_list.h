/* EasyScript runtime, part 3: lists. Parts 1 (es_memory.h) and 2
 * (es_runtime.h) come first.
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
 * Operations on items come in four versions, one per kind of item: _n
 * (number), _b (yes/no), _t (text), _l (list). */
#ifndef ES_LIST_H
#define ES_LIST_H

#ifndef ES_RUNTIME_H
#include "es_runtime.h"
#endif

/* --- Making lists ------------------------------------------------------------ */

/* Room for at least `need` items. */
static void es_list_reserve(EsList *l, size_t need) {
    if (need <= l->cap) return;
    size_t cap = l->cap ? l->cap * 2 : 4;
    if (cap < need) cap = need;
    size_t size = es_item_size(l->items);
    if (cap > SIZE_MAX / size) es_out_of_memory();
    es_memory_take((cap - l->cap) * size);
    void *data = realloc(l->at.any, cap * size);
    if (!data) es_out_of_memory();
    l->at.any = data;
    l->cap = cap;
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
        if (copy->at.o[i]->refs != ES_IMMORTAL) copy->at.o[i]->refs++;
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

/* "a copy of L": a new list, with copies of the lists inside it too, so
 * changing the copy never changes the original. Only looks at l. */
static EsList *es_list_copy_of(const EsList *l) {
    if (l->items != ES_ITEMS_LIST) return es_list_shallow_copy(l);
    EsList *copy = es_list_new(ES_ITEMS_LIST, l->len);
    for (size_t i = 0; i < l->len; i++) copy->at.l[i] = es_list_copy_of(l->at.l[i]);
    copy->len = l->len;
    return copy;
}

static EsList *es_list_copy(EsList *l) {
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
        if (l->items == ES_ITEMS_LIST) {
            out->at.l[k] = es_list_copy_of(l->at.l[i]);
        } else {
            memcpy((char *)out->at.any + k * size, (const char *)l->at.any + i * size, size);
            if (l->items == ES_ITEMS_TEXT) es_retain(out->at.t[k]);
        }
    }
    out->len = count;
    return out;
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

/* "items FROM to TO of L" (to_end: "to the end"). Both positions must be in
 * the list, and FROM can't be after TO. */
static EsList *es_slice_range(int site, EsList *l, double from, double to, bool to_end) {
    size_t a = es_index(site, l, ES_AT, from);
    size_t b = to_end ? l->len - 1 : es_index(site, l, ES_AT, to);
    if (a > b) {
        es_fail(site, NULL, "I can't take items %zu to %zu: the first position is after the last one.", a + 1, b + 1);
    }
    EsList *slice = es_list_pick(l, a, b - a + 1, 1);
    es_list_release(l);
    return slice;
}

/* "the first N items" / "the last N items": all of them when there are fewer. */
static EsList *es_slice_end(int site, EsList *l, double n, bool last) {
    if (n < 0) es_fail(site, NULL, "The number of items can't be negative, but it's %s.", es_shown(n));
    if (n != floor(n)) es_fail(site, NULL, "The number of items has to be a whole number, but it's %s.", es_shown(n));
    size_t count = n >= (double)l->len ? l->len : (size_t)n;
    EsList *slice = es_list_pick(l, last ? l->len - count : 0, count, 1);
    es_list_release(l);
    return slice;
}

/* "L without the first item" / "L without the last item" */
static EsList *es_slice_without(int site, EsList *l, bool last) {
    es_index(site, l, last ? ES_LAST : ES_FIRST, 0); /* there has to be one */
    EsList *slice = es_list_pick(l, last ? 0 : 1, l->len - 1, 1);
    es_list_release(l);
    return slice;
}

/* "every second item of L": the items at positions step, 2 * step, ...;
 * "every other item of L": 1, 3, 5, ... (start is the first position). */
static EsList *es_slice_every(EsList *l, size_t step, size_t start) {
    size_t count = l->len >= start ? (l->len - start) / step + 1 : 0;
    EsList *slice = es_list_pick(l, start - 1, count, step);
    es_list_release(l);
    return slice;
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

/* "remove item N from L" (or the first or last item). */
static void es_remove(int site, EsList *l, int where, double pos) {
    size_t i = es_index(site, l, where, pos);
    size_t size = es_item_size(l->items);
    if (es_items_counted(l->items)) es_object_release(l->at.o[i]);
    char *base = l->at.any;
    memmove(base + i * size, base + (i + 1) * size, (l->len - i - 1) * size);
    l->len--;
    es_list_release(l);
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
    EsList *r = es_list_snapshot(l);
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

/* "72, 95 and 88"; a list inside a list is in parentheses: "(1 and 2) and (3)". */
static void es_list_render(EsBuf *b, const EsList *l) {
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
            es_list_render(b, l->at.l[i]);
            es_buf_add(b, ")", 1);
            break;
        }
    }
}

static EsText *es_list_text(EsList *l) {
    EsBuf b = {NULL, 0, 0};
    es_list_render(&b, l);
    EsText *t = b.len > 0 ? es_text_copy(b.data, b.len) : &es_empty_text;
    free(b.data);
    es_list_release(l);
    return t;
}

/* --- Sorting -------------------------------------------------------------------
 *
 * A stable merge sort (equal items keep their order), written once as a macro
 * so each kind of item gets its own copy with the comparison inlined, instead
 * of calling a comparison function for every pair like qsort. Short runs are
 * sorted by insertion first; runs already in order are copied straight. */

#define ES_DEFINE_SORT(NAME, T, BEFORE)                                                  \
    static void NAME##_insertion(T *a, size_t n) {                                        \
        for (size_t i = 1; i < n; i++) {                                                  \
            T x = a[i];                                                                   \
            size_t j = i;                                                                 \
            while (j > 0 && BEFORE(x, a[j - 1])) {                                        \
                a[j] = a[j - 1];                                                          \
                j--;                                                                      \
            }                                                                             \
            a[j] = x;                                                                     \
        }                                                                                 \
    }                                                                                     \
    static void NAME##_merge(const T *src, T *dst, size_t lo, size_t mid, size_t hi) {    \
        size_t i = lo, j = mid, k = lo;                                                   \
        if (!BEFORE(src[mid], src[mid - 1])) { /* already in order */                     \
            memcpy(dst + lo, src + lo, (hi - lo) * sizeof(T));                            \
            return;                                                                       \
        }                                                                                 \
        while (i < mid && j < hi) dst[k++] = BEFORE(src[j], src[i]) ? src[j++] : src[i++]; \
        while (i < mid) dst[k++] = src[i++];                                              \
        while (j < hi) dst[k++] = src[j++];                                               \
    }                                                                                     \
    static void NAME(T *a, size_t n) {                                                    \
        const size_t run = 32;                                                            \
        for (size_t lo = 0; lo < n; lo += run) NAME##_insertion(a + lo, n - lo < run ? n - lo : run); \
        if (n <= run) return;                                                             \
        T *tmp = malloc(n * sizeof(T));                                                   \
        if (!tmp) es_out_of_memory();                                                     \
        T *src = a, *dst = tmp;                                                           \
        for (size_t width = run; width < n; width *= 2) {                                 \
            for (size_t lo = 0; lo < n; lo += 2 * width) {                                \
                size_t mid = lo + width < n ? lo + width : n;                             \
                size_t hi = lo + 2 * width < n ? lo + 2 * width : n;                      \
                if (mid < hi) {                                                           \
                    NAME##_merge(src, dst, lo, mid, hi);                                  \
                } else {                                                                  \
                    memcpy(dst + lo, src + lo, (hi - lo) * sizeof(T));                    \
                }                                                                         \
            }                                                                             \
            T *swap = src;                                                                \
            src = dst;                                                                    \
            dst = swap;                                                                   \
        }                                                                                 \
        if (src != a) memcpy(a, src, n * sizeof(T));                                      \
        free(tmp);                                                                        \
    }

/* Sorting by length sorts (length, item) pairs. */
typedef struct {
    size_t key;
    EsObject *item;
} EsKeyed;

#define ES_NUMBER_UP(x, y) ((x) < (y))
#define ES_NUMBER_DOWN(x, y) ((x) > (y))
#define ES_TEXT_UP(x, y) es_text_less((x), (y))
#define ES_TEXT_DOWN(x, y) es_text_less((y), (x))
#define ES_KEY_UP(x, y) ((x).key < (y).key)
#define ES_KEY_DOWN(x, y) ((x).key > (y).key)

ES_DEFINE_SORT(es_sort_numbers_up, double, ES_NUMBER_UP)
ES_DEFINE_SORT(es_sort_numbers_down, double, ES_NUMBER_DOWN)
typedef EsText *EsTextRef; /* (the macro needs a one-word type) */
ES_DEFINE_SORT(es_sort_text_up, EsTextRef, ES_TEXT_UP)
ES_DEFINE_SORT(es_sort_text_down, EsTextRef, ES_TEXT_DOWN)
ES_DEFINE_SORT(es_sort_keyed_up, EsKeyed, ES_KEY_UP)
ES_DEFINE_SORT(es_sort_keyed_down, EsKeyed, ES_KEY_DOWN)

enum { ES_SORT_UP, ES_SORT_DOWN, ES_SORT_LENGTH_UP, ES_SORT_LENGTH_DOWN };

/* Text by its length in characters, lists by their number of items. */
static void es_sort_by_length(EsList *l, bool longest_first) {
    if (l->len < 2) return;
    EsKeyed *keyed = malloc(l->len * sizeof *keyed);
    if (!keyed) es_out_of_memory();
    for (size_t i = 0; i < l->len; i++) {
        keyed[i].item = l->at.o[i];
        keyed[i].key = l->items == ES_ITEMS_TEXT ? es_text_chars(l->at.t[i]) : l->at.l[i]->len;
    }
    if (longest_first) {
        es_sort_keyed_down(keyed, l->len);
    } else {
        es_sort_keyed_up(keyed, l->len);
    }
    for (size_t i = 0; i < l->len; i++) l->at.o[i] = keyed[i].item;
    free(keyed);
}

/* "sort L": numbers smallest first, text alphabetically (by bytes), or by
 * length; `how` is one of ES_SORT_*. The list itself changes. */
static void es_sort(EsList *l, int how) {
    bool down = how == ES_SORT_DOWN || how == ES_SORT_LENGTH_DOWN;
    if (how == ES_SORT_LENGTH_UP || how == ES_SORT_LENGTH_DOWN) {
        es_sort_by_length(l, down);
    } else if (l->items == ES_ITEMS_NUMBER) {
        (down ? es_sort_numbers_down : es_sort_numbers_up)(l->at.n, l->len);
    } else if (l->items == ES_ITEMS_TEXT) {
        (down ? es_sort_text_down : es_sort_text_up)(l->at.t, l->len);
    }
    es_list_release(l);
}

#endif
