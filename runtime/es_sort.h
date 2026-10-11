/* EasyScript runtime, part 5: sorting lists. Parts 1 to 4 (es_memory.h,
 * es_program.h, es_runtime.h, es_list.h) come first.
 *
 * Numbers are sorted in place with pdqsort (pattern-defeating quicksort, the
 * algorithm by Orson Peters): no extra memory, and fast on lists that are
 * already partly in order. Equal numbers can't be told apart, so it doesn't
 * matter that it isn't stable. Text, and anything sorted by its length, use a
 * stable merge sort instead (equal items keep their order), which needs a
 * second buffer the size of the list.
 *
 * Both are written once as macros, so each kind of item gets its own copy
 * with the comparison inlined instead of calling a comparison function for
 * every pair like C's qsort. */
#ifndef ES_SORT_H
#define ES_SORT_H

#ifndef ES_LIST_H
#include "es_list.h"
#endif

/* --- pdqsort (numbers) ------------------------------------------------------
 *
 * Quicksort with: insertion sort for short ranges; the median of 3 (or, for
 * big ranges, of 9) as the pivot; a partition that sends many equal items to
 * one side at once; a check for ranges that are already sorted; and heapsort
 * after too many unbalanced partitions, so it's never worse than n log n.
 * LESS must be a strict order (no "not a number": es_sort moves those out). */

#define ES_PDQ_INSERTION 24 /* below this many items, insertion sort */
#define ES_PDQ_NINTHER 128  /* above this many, the pivot is a median of 9 */
#define ES_PDQ_PARTIAL 8    /* moves allowed when checking a range is nearly sorted */

#define ES_DEFINE_PDQSORT(NAME, T, LESS)                                                        \
    static void NAME##_swap(T *a, size_t i, size_t j) {                                         \
        T t = a[i];                                                                             \
        a[i] = a[j];                                                                            \
        a[j] = t;                                                                               \
    }                                                                                           \
    static void NAME##_sort2(T *a, size_t i, size_t j) {                                        \
        if (LESS(a[j], a[i])) NAME##_swap(a, i, j);                                             \
    }                                                                                           \
    static void NAME##_sort3(T *a, size_t i, size_t j, size_t k) {                              \
        NAME##_sort2(a, i, j);                                                                  \
        NAME##_sort2(a, j, k);                                                                  \
        NAME##_sort2(a, i, j);                                                                  \
    }                                                                                           \
    /* [lo, hi); with `guarded` false, a[lo - 1] is known to be no bigger than any of them */   \
    static void NAME##_insertion(T *a, size_t lo, size_t hi, bool guarded) {                    \
        for (size_t i = lo + 1; i < hi; i++) {                                                  \
            T x = a[i];                                                                         \
            size_t j = i;                                                                       \
            while ((!guarded || j > lo) && LESS(x, a[j - 1])) {                                 \
                a[j] = a[j - 1];                                                                \
                j--;                                                                            \
            }                                                                                   \
            a[j] = x;                                                                           \
        }                                                                                       \
    }                                                                                           \
    /* Insertion sort that gives up after a few moves: false if the range wasn't nearly sorted */ \
    static bool NAME##_partial(T *a, size_t lo, size_t hi) {                                    \
        size_t moves = 0;                                                                       \
        for (size_t i = lo + 1; i < hi; i++) {                                                  \
            if (!LESS(a[i], a[i - 1])) continue;                                                \
            T x = a[i];                                                                         \
            size_t j = i;                                                                       \
            do {                                                                                \
                a[j] = a[j - 1];                                                                \
                j--;                                                                            \
            } while (j > lo && LESS(x, a[j - 1]));                                              \
            a[j] = x;                                                                           \
            moves += i - j;                                                                     \
            if (moves > ES_PDQ_PARTIAL) return false;                                           \
        }                                                                                       \
        return true;                                                                            \
    }                                                                                           \
    /* Partitions around the pivot a[lo]: smaller items left, the rest right. Gives the     */  \
    /* pivot's new position, and whether the range was already partitioned. A median of 3   */  \
    /* (or 9) put an item at least as big as the pivot near the end, so the scans stop.     */  \
    static size_t NAME##_partition_right(T *a, size_t lo, size_t hi, bool *already) {           \
        T pivot = a[lo];                                                                        \
        size_t first = lo, last = hi;                                                           \
        while (LESS(a[++first], pivot)) {}                                                      \
        if (first - 1 == lo) {                                                                  \
            while (first < last && !LESS(a[--last], pivot)) {}                                  \
        } else {                                                                                \
            while (!LESS(a[--last], pivot)) {}                                                  \
        }                                                                                       \
        *already = first >= last;                                                               \
        while (first < last) {                                                                  \
            NAME##_swap(a, first, last);                                                        \
            while (LESS(a[++first], pivot)) {}                                                  \
            while (!LESS(a[--last], pivot)) {}                                                  \
        }                                                                                       \
        size_t p = first - 1;                                                                   \
        a[lo] = a[p];                                                                           \
        a[p] = pivot;                                                                           \
        return p;                                                                               \
    }                                                                                           \
    /* For many items equal to the pivot: those equal go left in one pass */                    \
    static size_t NAME##_partition_left(T *a, size_t lo, size_t hi) {                           \
        T pivot = a[lo];                                                                        \
        size_t first = lo, last = hi;                                                           \
        while (LESS(pivot, a[--last])) {}                                                       \
        if (last + 1 == hi) {                                                                   \
            while (first < last && !LESS(pivot, a[++first])) {}                                 \
        } else {                                                                                \
            while (!LESS(pivot, a[++first])) {}                                                 \
        }                                                                                       \
        while (first < last) {                                                                  \
            NAME##_swap(a, first, last);                                                        \
            while (LESS(pivot, a[--last])) {}                                                   \
            while (!LESS(pivot, a[++first])) {}                                                 \
        }                                                                                       \
        a[lo] = a[last];                                                                        \
        a[last] = pivot;                                                                        \
        return last;                                                                            \
    }                                                                                           \
    static void NAME##_sift(T *a, size_t lo, size_t root, size_t n) {                           \
        for (;;) {                                                                              \
            size_t child = 2 * root + 1;                                                        \
            if (child >= n) return;                                                             \
            if (child + 1 < n && LESS(a[lo + child], a[lo + child + 1])) child++;               \
            if (!LESS(a[lo + root], a[lo + child])) return;                                     \
            NAME##_swap(a, lo + root, lo + child);                                              \
            root = child;                                                                       \
        }                                                                                       \
    }                                                                                           \
    static void NAME##_heapsort(T *a, size_t lo, size_t hi) {                                   \
        size_t n = hi - lo;                                                                     \
        for (size_t i = n / 2; i-- > 0;) NAME##_sift(a, lo, i, n);                              \
        for (size_t end = n; end-- > 1;) {                                                      \
            NAME##_swap(a, lo, lo + end);                                                       \
            NAME##_sift(a, lo, 0, end);                                                         \
        }                                                                                       \
    }                                                                                           \
    /* Unbalanced partition: shuffle a few items so the next pivot is likely better */          \
    static void NAME##_shuffle(T *a, size_t lo, size_t p, size_t hi) {                          \
        size_t l = p - lo, r = hi - (p + 1);                                                    \
        if (l >= ES_PDQ_INSERTION) {                                                            \
            NAME##_swap(a, lo, lo + l / 4);                                                     \
            NAME##_swap(a, p - 1, p - l / 4);                                                   \
        }                                                                                       \
        if (r >= ES_PDQ_INSERTION) {                                                            \
            NAME##_swap(a, p + 1, p + 1 + r / 4);                                               \
            NAME##_swap(a, hi - 1, hi - r / 4);                                                 \
        }                                                                                       \
    }                                                                                           \
    static void NAME##_loop(T *a, size_t lo, size_t hi, int bad_allowed, bool leftmost) {       \
        for (;;) {                                                                              \
            size_t size = hi - lo;                                                              \
            if (size < ES_PDQ_INSERTION) {                                                      \
                NAME##_insertion(a, lo, hi, leftmost);                                          \
                return;                                                                         \
            }                                                                                   \
            size_t half = size / 2;                                                             \
            if (size > ES_PDQ_NINTHER) {                                                        \
                NAME##_sort3(a, lo, lo + half, hi - 1);                                         \
                NAME##_sort3(a, lo + 1, lo + half - 1, hi - 2);                                 \
                NAME##_sort3(a, lo + 2, lo + half + 1, hi - 3);                                 \
                NAME##_sort3(a, lo + half - 1, lo + half, lo + half + 1);                       \
                NAME##_swap(a, lo, lo + half);                                                  \
            } else {                                                                            \
                NAME##_sort3(a, lo + half, lo, hi - 1);                                         \
            }                                                                                   \
            /* The item before this range is no bigger than the pivot: equal ones go left */    \
            if (!leftmost && !LESS(a[lo - 1], a[lo])) {                                         \
                lo = NAME##_partition_left(a, lo, hi) + 1;                                      \
                continue;                                                                       \
            }                                                                                   \
            bool already;                                                                       \
            size_t p = NAME##_partition_right(a, lo, hi, &already);                             \
            size_t l = p - lo, r = hi - (p + 1);                                                \
            if (l < size / 8 || r < size / 8) {                                                 \
                if (--bad_allowed == 0) {                                                       \
                    NAME##_heapsort(a, lo, hi);                                                 \
                    return;                                                                     \
                }                                                                               \
                NAME##_shuffle(a, lo, p, hi);                                                   \
            } else if (already && NAME##_partial(a, lo, p) && NAME##_partial(a, p + 1, hi)) {   \
                return;                                                                         \
            }                                                                                   \
            NAME##_loop(a, lo, p, bad_allowed, leftmost);                                       \
            lo = p + 1;                                                                         \
            leftmost = false;                                                                   \
        }                                                                                       \
    }                                                                                           \
    static void NAME(T *a, size_t n) {                                                          \
        int bad_allowed = 1;                                                                    \
        for (size_t m = n; m > 1; m >>= 1) bad_allowed++;                                       \
        if (n > 1) NAME##_loop(a, 0, n, bad_allowed, true);                                     \
    }

/* --- Stable merge sort (text, and sorting by length) --------------------------
 *
 * Short runs are sorted by insertion first; runs already in order are copied
 * straight. */

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

ES_DEFINE_PDQSORT(es_sort_numbers_up, double, ES_NUMBER_UP)
ES_DEFINE_PDQSORT(es_sort_numbers_down, double, ES_NUMBER_DOWN)
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

/* "sort L": numbers smallest first (with "not a number" last), text
 * alphabetically (by bytes), or by length; `how` is one of ES_SORT_*. The
 * list itself changes. */
static void es_sort(EsList *l, int how) {
    bool down = how == ES_SORT_DOWN || how == ES_SORT_LENGTH_DOWN;
    if (how == ES_SORT_LENGTH_UP || how == ES_SORT_LENGTH_DOWN) {
        es_sort_by_length(l, down);
    } else if (l->items == ES_ITEMS_NUMBER) {
        /* "not a number" isn't smaller or bigger than anything, so it goes last */
        size_t n = 0;
        for (size_t i = 0; i < l->len; i++) {
            if (isnan(l->at.n[i])) continue;
            double x = l->at.n[i];
            l->at.n[i] = l->at.n[n];
            l->at.n[n++] = x;
        }
        (down ? es_sort_numbers_down : es_sort_numbers_up)(l->at.n, n);
    } else if (l->items == ES_ITEMS_TEXT) {
        (down ? es_sort_text_down : es_sort_text_up)(l->at.t, l->len);
    }
    es_list_release(l);
}

#endif
