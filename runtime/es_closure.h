/* EasyScript runtime, part 10: functions as values. Parts 1 to 9 come first.
 *
 * A function kept as a value ("the function double", "a function taking x
 * that gives back x times 2") is a closure: a record (EsClosure) holding a
 * pointer to its C code, and, as its fields, the cells of the names it uses
 * from around it. The compiler writes one C function per function value,
 * taking the call's site, the closure itself and the inputs, and calls it
 * through the code pointer cast to that exact type (the checker knows every
 * kind, so the cast is always right).
 *
 * A name that a function made inside uses is kept in a cell: a record with
 * one field (es_cell_type_n, ...). The code around it and every closure that
 * uses it share the cell, so a change either one makes is seen by both (a
 * counter made by "to make counter" counts on). Each "let" makes a new cell,
 * so closures made in different rounds of a loop each keep their own.
 *
 * A function defined with "to" is a static immortal closure (es_fv_NAME)
 * with no cells, whose code (es_w_NAME) calls the function. */
#ifndef ES_CLOSURE_H
#define ES_CLOSURE_H

#ifndef ES_KIND_H
#include "es_kind.h"
#endif

typedef void (*EsCode)(void);

typedef struct {
    EsRecord base;
    EsCode code;
} EsClosure;

/* The C function a closure runs, as the function pointer type T. Only looks. */
#define ES_CODE(f, T) ((T)((const EsClosure *)(f))->code)

/* A new closure; the compiler puts its cells (retained) in its fields. */
static EsRecord *es_closure_new(const EsRecordType *type, EsCode code) {
    EsRecord *r = es_record_new(type);
    ((EsClosure *)r)->code = code;
    return r;
}

/* --- Cells ----------------------------------------------------------------------- */

/* What a cell holds, as a C lvalue of type T. */
#define ES_CELL(c, T) ES_FIELD(c, T, sizeof(EsRecord))

static const char *const es_cell_names[] = {"value"};
static const size_t es_cell_at[] = {sizeof(EsRecord)};

/* es_cell_n(v): a new cell holding v (taken). */
#define ES_CELL_KIND(S, T, KIND)                                                                             \
    static const EsRecordType es_cell_type_##S = {"a name", 1, es_cell_names, KIND, es_cell_at,               \
                                                  sizeof(EsRecord) + sizeof(T), ES_CELL};                   \
    static EsRecord *es_cell_##S(T v) {                                                                      \
        EsRecord *c = es_record_new(&es_cell_type_##S);                                                      \
        ES_CELL(c, T) = v;                                                                                   \
        return c;                                                                                            \
    }

ES_CELL_KIND(n, double, "n")
ES_CELL_KIND(u, long long, "u")
ES_CELL_KIND(e, long long, "e")
ES_CELL_KIND(p, long long, "p")
ES_CELL_KIND(b, bool, "b")
ES_CELL_KIND(t, EsText *, "t")
ES_CELL_KIND(l, EsList *, "l")
ES_CELL_KIND(r, EsRecord *, "r")
ES_CELL_KIND(m, EsMap *, "m")

/* --- Built-ins that take a function ------------------------------------------------ *
 *
 * The compiler writes a C function for each use ("double applied to every
 * number in scores"), which goes through the list calling the function
 * directly (a "to" function) or through its closure. These are what they
 * share. */

/* "apply F to every X in L", "keep only ...", "sort L by F": L's items
 * become `items`' (worked out from L), so every name for L sees the change.
 * Takes both. */
static void es_list_replace(EsList *l, EsList *items) {
    if (l == items) {
        es_list_release(items);
        es_list_release(l);
        return;
    }
    items = es_list_copy(items); /* lists inside belong to one list */
    if (es_items_counted(l->items)) {
        for (size_t i = 0; i < l->len; i++) {
            if (l->at.o[i]) es_object_release(l->at.o[i]);
        }
    }
    /* Copied into the room the list already has (moving the new items' block
     * over instead made macOS keep both: the peak went from 36 to 100 MB). */
    l->len = 0;
    es_list_reserve(l, items->len);
    if (items->len > 0) memcpy(l->at.any, items->at.any, items->len * es_item_size(l->items)); /* (an empty list may have no items block) */
    l->len = items->len;
    if (es_items_counted(l->items)) { /* items keeps its own references until it's released */
        for (size_t i = 0; i < l->len; i++) {
            if (l->at.o[i] && l->at.o[i]->refs != ES_IMMORTAL) l->at.o[i]->refs++;
        }
    }
    es_list_release(items);
    es_list_release(l);
}

/* An item's place and the key it's sorted by ("names sorted by length of"). */
typedef struct {
    double number;
    EsText *text;
    size_t at;
} EsByKey;

#define ES_BY_NUMBER_UP(x, y) ((x).number < (y).number)
#define ES_BY_NUMBER_DOWN(x, y) ((x).number > (y).number)
#define ES_BY_TEXT_UP(x, y) es_text_less((x).text, (y).text)
#define ES_BY_TEXT_DOWN(x, y) es_text_less((y).text, (x).text)

ES_DEFINE_SORT(es_sort_by_number_up, EsByKey, ES_BY_NUMBER_UP)
ES_DEFINE_SORT(es_sort_by_number_down, EsByKey, ES_BY_NUMBER_DOWN)
ES_DEFINE_SORT(es_sort_by_text_up, EsByKey, ES_BY_TEXT_UP)
ES_DEFINE_SORT(es_sort_by_text_down, EsByKey, ES_BY_TEXT_DOWN)

/* Puts l's items in the order of their keys (numbers, dates or text, one per item,
 * worked out by the function), keeping items with equal keys in the order
 * they were in. "not a number" keys go last. Takes the keys, only looks at l. */
static void es_sort_by_keys(EsList *l, EsList *keys, bool down) {
    size_t n = l->len, size = es_item_size(l->items);
    if (n > 1) {
        EsByKey *by = malloc(n * sizeof *by);
        char *moved = malloc(n * size);
        if (!by || !moved) es_out_of_memory();
        size_t m = 0, last = n;
        for (size_t i = 0; i < n; i++) { /* "not a number" keys last, in order */
            bool numeric = keys->items != ES_ITEMS_TEXT;
            bool nan = numeric && isnan(keys->at.n[i]);
            EsByKey *slot = nan ? &by[--last] : &by[m++];
            slot->number = numeric ? keys->at.n[i] : 0;
            slot->text = keys->items == ES_ITEMS_TEXT ? keys->at.t[i] : NULL;
            slot->at = i;
        }
        for (size_t i = last, j = n - 1; i < j; i++, j--) { /* they were put there backwards */
            EsByKey swap = by[i];
            by[i] = by[j];
            by[j] = swap;
        }
        if (keys->items == ES_ITEMS_TEXT) {
            (down ? es_sort_by_text_down : es_sort_by_text_up)(by, m);
        } else {
            (down ? es_sort_by_number_down : es_sort_by_number_up)(by, m);
        }
        for (size_t i = 0; i < n; i++) memcpy(moved + i * size, (char *)l->at.any + by[i].at * size, size);
        memcpy(l->at.any, moved, n * size);
        free(moved);
        free(by);
    }
    es_list_release(keys);
}

/* --- The memory check ------------------------------------------------------------- */

static bool es_report_function_cycle(size_t circled, EsRecord *oldest) {
    fprintf(stderr, "Memory check: %zu value%s still alive at the end of the program, because a function made "
                    "in a value keeps itself alive through a name it uses:\n", circled, circled == 1 ? " was" : "s were");
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
    fputs("A function made in a value that uses the name it's kept in (or a record, list or map that holds it) is "
          "never freed. "
          "Define it with \"to\" instead, or set that name to something else before the program ends.\n",
          stderr);
    return true;
}

#endif
