/* EasyScript runtime, part 17: questions about a list ("how many", "the
 * average of", "the median of", "the top 3", "the most common", "per") and
 * sorting a map by its values.
 *
 * The compiler turns each question into one C function that goes through the
 * list once (see codegen_aggregate.c); these are the pieces it calls. */
#ifndef ES_TOTALS_H
#define ES_TOTALS_H

#ifndef ES_SYSTEM_H
#include "es_system.h"
#endif

/* "the average of L": the total divided by how many were added up. */
static double es_average(int site, double total, double count) {
    if (count == 0) es_fail(site, NULL, "There's nothing to average: the list is empty.");
    return total / count;
}

/* "the median of L": the middle number, or halfway between the two middle
 * ones (taking the list of numbers). */
static double es_median(int site, EsList *l) {
    size_t n = l->len;
    if (n == 0) {
        es_list_release(l);
        es_fail(site, NULL, "There's no middle number: the list is empty.");
    }
    es_sort(es_list_retain(l), ES_SORT_UP); /* "not a number" goes last */
    double middle = n % 2 ? l->at.n[n / 2] : (l->at.n[n / 2 - 1] + l->at.n[n / 2]) / 2;
    es_list_release(l);
    return middle;
}

/* "the ITEM with the highest ...", for an empty list. */
static _Noreturn void es_no_choice(int site) {
    es_fail(site, "Check that the list isn't empty first, like \"if dogs is not empty:\".",
            "There's nothing to choose from: the list is empty.");
}

/* --- Rules ("an age must be between 0 and 150") ------------------------------------- */

/* A value set where a rule says it can't be: the rule as written, and the value. */
static _Noreturn void es_rule_broken_n(int site, const char *rule, double value) {
    es_fail(site, NULL, "%s, but this one is %s.", rule, es_shown(value));
}

static _Noreturn void es_rule_broken_t(int site, const char *rule, const EsText *value) {
    size_t shown = 0;
    for (size_t chars = 0; shown < value->len && chars < 60; chars++) {
        shown++;
        while (shown < value->len && es_continuation_byte(value->chars[shown])) shown++;
    }
    es_fail(site, NULL, "%s, but this one is \"%.*s%s\".", rule, (int)shown, value->chars, shown < value->len ? "..." : "");
}

static _Noreturn void es_rule_broken_x(int site, const char *rule) {
    es_fail(site, NULL, "%s, but this one doesn't follow it.", rule);
}

/* --- Grouping ("per") ------------------------------------------------------------- */

/* Where the key's value is, adding the key with the value 0 (or nothing) if
 * it isn't there yet; *added (when given) says whether it was. Takes the key.
 * The place is only good until the map changes again. */
static EsValue *es_map_slot(EsMap *m, EsValue key, bool *added) {
    key = es_key(m, key);
    uint64_t hash = es_key_hash(m, key);
    size_t i = es_map_find(m, key, hash);
    if (added) *added = i == ES_NOT_FOUND;
    if (i != ES_NOT_FOUND) {
        es_value_release(m->keys, key);
        return &m->entries[i].value;
    }
    EsValue zero = {0};
    es_map_add(m, key, hash, zero);
    return &m->entries[m->used - 1].value;
}

/* "per month", "per day", "per weekday": the group a date is in, as text:
 * "October 2026", "Sunday, October 11, 2026", "Sunday". `part` is the
 * compiler's DatePart. ("per year" groups by the year, a number.) */
static EsText *es_period_text(double date, int part) {
    if (part == 3) return es_weekday_text(date);
    if (part == 2) return es_date_text(floor(date / ES_DAY) * ES_DAY);
    long year;
    int month, day;
    es_civil_from_days(floor(date / ES_DAY), &year, &month, &day);
    const char *name = es_months[month - 1];
    int n = snprintf(NULL, 0, "%s %ld", name, year);
    char *chars;
    EsText *t = es_text_new((size_t)n, &chars);
    snprintf(chars, (size_t)n + 1, "%s %ld", name, year);
    return t;
}

/* "the average ... per ...": each total divided by its count. Both maps got
 * the same keys in the same order. Takes counts. */
static EsMap *es_map_divide(EsMap *totals, EsMap *counts) {
    for (size_t i = 0; i < totals->used; i++) totals->entries[i].value.n /= counts->entries[i].value.n;
    es_map_release(counts);
    return totals;
}

/* The averages or medians per group of amounts of money: rounded to the cent,
 * and kept as money. Numbers are left as they are. Takes the map. */
static EsMap *es_map_as(EsMap *m, EsItems values) {
    if (!es_items_money(values)) return m;
    for (size_t i = 0; i < m->used; i++) m->entries[i].value.n = round(m->entries[i].value.n);
    m->values = values;
    return m;
}

/* "the median ... per ...": a map of the middle numbers of each list. Takes the map. */
static EsMap *es_map_medians(int site, EsMap *lists) {
    EsMap *m = es_map_new(lists->keys, ES_ITEMS_NUMBER, lists->len);
    for (size_t i = 0; i < lists->used; i++) {
        EsValue key = lists->entries[i].key, value;
        if (lists->keys == ES_ITEMS_TEXT) es_retain(key.t);
        value.n = es_median(site, es_list_retain(lists->entries[i].value.l));
        es_map_store(m, key, value);
    }
    es_map_release(lists);
    return m;
}

/* --- The top N ----------------------------------------------------------------------
 *
 * A heap of the best N found so far, its worst at the top: each item is
 * compared with the worst kept and replaces it if it's better, so finding the
 * top 5 of a million items keeps 5, never sorting the million. Ties keep the
 * item that came first, and "not a number" is worse than any number. */

typedef struct {
    double key;
    size_t at;
} EsRank;

typedef struct {
    EsRank *ranks;
    size_t len, cap;
    bool lowest; /* "the lowest 3": smaller keys are better */
} EsTop;

static bool es_rank_better(const EsTop *t, EsRank a, EsRank b) {
    bool a_nan = isnan(a.key), b_nan = isnan(b.key);
    if (a_nan != b_nan) return b_nan;
    if (!a_nan && a.key != b.key) return t->lowest ? a.key < b.key : a.key > b.key;
    return a.at < b.at;
}

/* How many to take (a whole number, 0 or more), of at most `most` items. */
static EsTop es_top_start(int site, double n, size_t most, bool lowest) {
    if (!(n >= 0) || n != floor(n)) {
        es_fail(site, NULL, "How many to take has to be a whole number, 0 or more, but it's %s.", es_shown(n));
    }
    EsTop t = {NULL, 0, n < (double)most ? (size_t)n : most, lowest};
    if (t.cap > 0) {
        t.ranks = malloc(t.cap * sizeof *t.ranks);
        if (!t.ranks) es_out_of_memory();
    }
    return t;
}

static void es_top_swap(EsTop *t, size_t i, size_t j) {
    EsRank swap = t->ranks[i];
    t->ranks[i] = t->ranks[j];
    t->ranks[j] = swap;
}

/* Moves rank i down to its place (each rank is worse than the ones below it). */
static void es_top_sink(EsTop *t, size_t i, size_t len) {
    for (;;) {
        size_t worst = i, left = 2 * i + 1, right = left + 1;
        if (left < len && es_rank_better(t, t->ranks[worst], t->ranks[left])) worst = left;
        if (right < len && es_rank_better(t, t->ranks[worst], t->ranks[right])) worst = right;
        if (worst == i) return;
        es_top_swap(t, i, worst);
        i = worst;
    }
}

static void es_top_offer(EsTop *t, double key, size_t at) {
    EsRank r = {key, at};
    if (t->len < t->cap) {
        size_t i = t->len++;
        t->ranks[i] = r;
        while (i > 0 && es_rank_better(t, t->ranks[(i - 1) / 2], t->ranks[i])) {
            es_top_swap(t, i, (i - 1) / 2);
            i = (i - 1) / 2;
        }
    } else if (t->cap > 0 && es_rank_better(t, r, t->ranks[0])) {
        t->ranks[0] = r;
        es_top_sink(t, 0, t->len);
    }
}

/* Puts the ranks kept in order, best first, and frees nothing yet. */
static void es_top_order(EsTop *t) {
    for (size_t len = t->len; len > 1; len--) { /* the worst left goes to the end */
        es_top_swap(t, 0, len - 1);
        es_top_sink(t, 0, len - 1);
    }
}

/* The items of l at the kept places, best first, like a slice: text shared,
 * lists and maps copied. Frees the heap; only looks at l. */
static EsList *es_top_take(EsTop *t, const EsList *l) {
    es_top_order(t);
    EsList *out = es_list_new(l->items, t->len);
    size_t size = es_item_size(l->items);
    for (size_t k = 0; k < t->len; k++) {
        size_t i = t->ranks[k].at;
        if (es_items_copied(l->items)) {
            out->at.o[k] = es_copy_inner(l->items, l->at.o[i]);
        } else {
            memcpy((char *)out->at.any + k * size, (const char *)l->at.any + i * size, size);
            if (l->items == ES_ITEMS_TEXT) es_retain(out->at.t[k]);
            if (l->items == ES_ITEMS_RECORD) es_record_retain(out->at.r[k]);
        }
    }
    out->len = t->len;
    free(t->ranks);
    return out;
}

/* "the N most common L": the keys of the counts with the biggest counts,
 * ties in the order they first came up. Takes the counts. */
static EsList *es_most_common(EsTop *t, EsMap *counts) {
    if (t->cap > counts->len) t->cap = counts->len;
    for (size_t i = 0; i < counts->used; i++) es_top_offer(t, counts->entries[i].value.n, i);
    es_top_order(t);
    EsList *out = es_list_new(counts->keys, t->len);
    for (size_t k = 0; k < t->len; k++) {
        EsValue key = counts->entries[t->ranks[k].at].key;
        if (counts->keys == ES_ITEMS_TEXT) {
            out->at.t[k] = es_retain(key.t);
        } else {
            out->at.n[k] = key.n;
        }
    }
    out->len = t->len;
    free(t->ranks);
    es_map_release(counts);
    return out;
}

/* --- Sorting a map by its values ------------------------------------------------------ */

/* "sort M biggest first": the entries in the order of their values (numbers,
 * dates or text), keeping the order of equal ones. The map itself changes. */
static void es_map_sort(EsMap *m, int how) {
    es_map_reindex(m); /* no removed entries left between them */
    size_t n = m->used;
    if (n > 1) {
        EsByKey *by = malloc(n * sizeof *by);
        EsEntry *moved = malloc(n * sizeof *moved);
        if (!by || !moved) es_out_of_memory();
        bool numeric = m->values != ES_ITEMS_TEXT, down = how == ES_SORT_DOWN;
        size_t kept = 0, last = n;
        for (size_t i = 0; i < n; i++) { /* "not a number" last, in order */
            bool nan = numeric && isnan(m->entries[i].value.n);
            EsByKey *slot = nan ? &by[--last] : &by[kept++];
            slot->number = numeric ? m->entries[i].value.n : 0;
            slot->text = numeric ? NULL : m->entries[i].value.t;
            slot->at = i;
        }
        for (size_t i = last, j = n - 1; i < j; i++, j--) { /* they were put there backwards */
            EsByKey swap = by[i];
            by[i] = by[j];
            by[j] = swap;
        }
        if (numeric) {
            (down ? es_sort_by_number_down : es_sort_by_number_up)(by, kept);
        } else {
            (down ? es_sort_by_text_down : es_sort_by_text_up)(by, kept);
        }
        for (size_t i = 0; i < n; i++) moved[i] = m->entries[by[i].at];
        memcpy(m->entries, moved, n * sizeof *moved);
        free(moved);
        free(by);
        es_map_reindex(m);
    }
    es_map_release(m);
}

/* "M sorted biggest first": a sorted copy, like "a copy of". */
static EsMap *es_map_sorted(EsMap *m, int how) {
    EsMap *r = es_map_copy(m);
    es_map_sort(es_map_retain(r), how);
    return r;
}

/* --- "as a table" ------------------------------------------------------------------
 *
 * A list of records (or a table's rows) as lines with a column per field and
 * the fields' names on top; a map as two columns (keys, values); any other
 * list as one item per line. Columns are padded with spaces to their widest
 * cell, numbers on the right. Text isn't quoted. */

/* A cell holding a value of a list or map. */
static void es_cell_value(EsBuf *b, EsItems kind, EsValue v) {
    switch (kind) {
    case ES_ITEMS_NUMBER: {
        EsText *t = es_number_text(v.n);
        es_buf_add(b, t->chars, t->len);
        es_release(t);
        break;
    }
    case ES_ITEMS_DATE: {
        EsText *t = es_date_text(v.n);
        es_buf_add(b, t->chars, t->len);
        es_release(t);
        break;
    }
    case ES_ITEMS_DOLLARS:
    case ES_ITEMS_EUROS:
    case ES_ITEMS_POUNDS: es_buf_add_money(b, v.n, kind); break;
    case ES_ITEMS_YESNO: es_buf_add_text(b, v.b ? "yes" : "no"); break;
    case ES_ITEMS_TEXT: es_buf_add(b, v.t->chars, v.t->len); break;
    default: es_value_render(b, kind, v, 1); break;
    }
}

/* A cell holding field i of r. */
static void es_cell_field(EsBuf *b, const EsRecord *r, int i) {
    size_t at = r->type->offsets[i];
    EsValue v;
    switch (r->type->kinds[i]) {
    case 'n': v.n = ES_FIELD(r, double, at); es_cell_value(b, ES_ITEMS_NUMBER, v); break;
    case 'd': v.n = ES_FIELD(r, double, at); es_cell_value(b, ES_ITEMS_DATE, v); break;
    case 'u': v.n = (double)ES_FIELD(r, long long, at); es_cell_value(b, ES_ITEMS_DOLLARS, v); break;
    case 'e': v.n = (double)ES_FIELD(r, long long, at); es_cell_value(b, ES_ITEMS_EUROS, v); break;
    case 'p': v.n = (double)ES_FIELD(r, long long, at); es_cell_value(b, ES_ITEMS_POUNDS, v); break;
    case 'b': v.b = ES_FIELD(r, bool, at); es_cell_value(b, ES_ITEMS_YESNO, v); break;
    case 't': v.t = ES_FIELD(r, EsText *, at); es_cell_value(b, ES_ITEMS_TEXT, v); break;
    default: es_field_render(b, r, i, 1); break;
    }
}

typedef struct {
    EsText **cells; /* row by row, each owned */
    size_t count, cap, cols;
    bool *right;    /* per column: numbers, put on the right */
} EsGrid;

static EsGrid es_grid_new(size_t cols) {
    EsGrid g = {NULL, 0, 0, cols, calloc(cols, sizeof(bool))};
    if (!g.right) es_out_of_memory();
    return g;
}

/* Adds the text in b (then emptied) as the next cell. */
static void es_grid_add(EsGrid *g, EsBuf *b) {
    if (g->count == g->cap) {
        size_t cap = g->cap ? g->cap * 2 : 16;
        EsText **cells = realloc(g->cells, cap * sizeof *cells);
        if (!cells) es_out_of_memory();
        g->cells = cells;
        g->cap = cap;
    }
    g->cells[g->count++] = es_buf_text(b);
    b->data = NULL;
    b->len = b->cap = 0;
}

/* The cells, laid out. Frees the grid. */
static EsText *es_grid_text(EsGrid *g) {
    size_t count = g->count;
    size_t *widths = calloc(g->cols ? g->cols : 1, sizeof *widths);
    if (!widths) es_out_of_memory();
    for (size_t i = 0; i < count; i++) {
        size_t w = es_text_chars(g->cells[i]);
        if (w > widths[i % g->cols]) widths[i % g->cols] = w;
    }
    EsBuf b = {NULL, 0, 0};
    for (size_t i = 0; i < count; i++) {
        size_t col = i % g->cols, w = es_text_chars(g->cells[i]);
        bool last = col + 1 == g->cols;
        if (col == 0 && i > 0) es_buf_add(&b, "\n", 1);
        if (col > 0) es_buf_add(&b, "  ", 2);
        if (g->right[col]) {
            for (size_t k = w; k < widths[col]; k++) es_buf_add(&b, " ", 1);
        }
        es_buf_add(&b, g->cells[i]->chars, g->cells[i]->len);
        if (!g->right[col] && !last) {
            for (size_t k = w; k < widths[col]; k++) es_buf_add(&b, " ", 1);
        }
        es_release(g->cells[i]);
    }
    free(widths);
    free(g->cells);
    free(g->right);
    return es_buf_text(&b);
}
/* The columns of a list of records: a table's rows show every column of the
 * file; other records their fields. */
static void es_record_rows(EsGrid *g, const EsList *l, const EsRecord *first) {
    const EsRecordType *type = first->type;
    bool table = type->fields >= 2 && strcmp(type->field_names[0], "0header") == 0;
    const EsList *header = table ? ES_FIELD(first, EsList *, type->offsets[0]) : NULL;
    EsBuf b = {NULL, 0, 0};
    for (size_t c = 0; c < g->cols; c++) {  /* the names on top */
        if (table) {
            es_buf_add(&b, header->at.t[c]->chars, header->at.t[c]->len);
        } else {
            es_buf_add_text(&b, type->field_names[c]);
        }
        es_grid_add(g, &b);
    }
    for (size_t i = 0; i < l->len; i++) {
        const EsRecord *r = l->at.r[i];
        for (size_t c = 0; c < g->cols; c++) {
            if (!r || r->type != type) {  /* nothing, or another variant: the whole value in the first column */
                if (c == 0) es_record_render(&b, r, 1);
            } else if (table) {
                int field = -1;
                for (int f = 2; f < type->fields && field < 0; f++) {
                    if (es_column_is(header->at.t[c], type->field_names[f])) field = f;
                }
                if (field >= 0) {
                    es_cell_field(&b, r, field);
                    g->right[c] |= type->kinds[field] == 'n' || type->kinds[field] == 'u' || type->kinds[field] == 'e' ||
                                   type->kinds[field] == 'p';
                } else {
                    const EsList *cells = ES_FIELD(r, EsList *, type->offsets[1]);
                    es_buf_add(&b, cells->at.t[c]->chars, cells->at.t[c]->len);
                }
            } else {
                es_cell_field(&b, r, (int)c);
                g->right[c] |= type->kinds[c] == 'n' || type->kinds[c] == 'u' || type->kinds[c] == 'e' || type->kinds[c] == 'p';
            }
            es_grid_add(g, &b);
        }
    }
}

/* "L as a table". Consumes L. */
static EsText *es_list_table(EsList *l) {
    const EsRecord *first = NULL;
    for (size_t i = 0; l->items == ES_ITEMS_RECORD && i < l->len && !first; i++) {
        const EsRecord *r = l->at.r[i];
        if (r && r->type->style != ES_BOX && r->type->style != ES_CLOSURE) first = r;
    }
    size_t cols = 1;
    if (first) {
        const EsRecordType *type = first->type;
        bool table = type->fields >= 2 && strcmp(type->field_names[0], "0header") == 0;
        cols = table ? ES_FIELD(first, EsList *, type->offsets[0])->len : (size_t)type->fields;
    }
    EsGrid g = es_grid_new(cols ? cols : 1);
    if (first && cols > 0) {
        es_record_rows(&g, l, first);
    } else {
        EsBuf b = {NULL, 0, 0};
        for (size_t i = 0; i < l->len; i++) {
            EsValue v;
            switch (l->items) {
            case ES_ITEMS_NUMBER:
            case ES_ITEMS_DATE:
            case ES_ITEMS_DOLLARS:
            case ES_ITEMS_EUROS:
            case ES_ITEMS_POUNDS: v.n = l->at.n[i]; break;
            case ES_ITEMS_YESNO: v.b = l->at.b[i]; break;
            case ES_ITEMS_TEXT: v.t = l->at.t[i]; break;
            case ES_ITEMS_LIST: v.l = l->at.l[i]; break;
            case ES_ITEMS_RECORD: v.r = l->at.r[i]; break;
            case ES_ITEMS_MAP: v.m = l->at.m[i]; break;
            }
            es_cell_value(&b, l->items, v);
            es_grid_add(&g, &b);
        }
        g.right[0] = l->items == ES_ITEMS_NUMBER || es_items_money(l->items);
    }
    es_list_release(l);
    return es_grid_text(&g);
}

/* "M as a table": its keys and values, in order. Consumes M. */
static EsText *es_map_table(EsMap *m) {
    EsGrid g = es_grid_new(2);
    g.right[1] = m->values == ES_ITEMS_NUMBER || es_items_money(m->values);
    EsBuf b = {NULL, 0, 0};
    for (size_t i = 0; i < m->used; i++) {
        const EsEntry *e = &m->entries[i];
        if (e->hash == 0) continue;
        es_cell_value(&b, m->keys, e->key);
        es_grid_add(&g, &b);
        es_cell_value(&b, m->values, e->value);
        es_grid_add(&g, &b);
    }
    es_map_release(m);
    return es_grid_text(&g);
}

#endif
