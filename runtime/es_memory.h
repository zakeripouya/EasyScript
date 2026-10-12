/* EasyScript runtime, part 1: heap objects (text, lists, records and maps) and memory.
 *
 * The runtime is sixteen files, embedded into the compiler at build time (in
 * this order: es_memory.h, es_program.h, es_runtime.h, es_list.h, es_sort.h,
 * es_text.h, es_record.h, es_map.h, es_kind.h, es_closure.h, es_math.h,
 * es_time.h, es_files.h, es_csv.h, es_keys.h, es_system.h) and copied to the
 * top of every generated C program, so programs need nothing else to build.
 * Everything is static; generated code calls it.
 *
 * The compiler knows the kind of every value, so generated code uses plain C
 * types: a number is a double, yes/no is a bool, text is an EsText *, a list
 * is an EsList *, a record is an EsRecord * (NULL for nothing), and a map is
 * an EsMap *. Text, lists, records and maps live on the heap with a reference count, and are freed
 * as soon as nothing uses them (see docs/memory.md);
 * text written in the program (literals, constants) is a static object marked
 * immortal, which is never counted or freed.
 *
 * Ownership: every text or list an expression produces is owned. Runtime
 * functions that take EsText * or EsList * consume it (release it, or hand it
 * back in their result); reading a variable retains; storing into one
 * (es_set, es_list_set) takes the new value and releases the old one.
 * Runtime errors are in es_program.h.
 */
#ifndef ES_MEMORY_H
#define ES_MEMORY_H

#define _POSIX_C_SOURCE 200809L /* getline */

#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Programs use only some of these functions; that's expected. */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-function"
#endif
/* A function that only ever calls itself is a mistake EasyScript reports
 * while the program runs ("calling each other too deeply"), so it's not one
 * for the C compiler to warn about. */
#if defined(__clang__) || (defined(__GNUC__) && __GNUC__ >= 12)
#pragma GCC diagnostic ignored "-Winfinite-recursion"
#endif

/* --- Heap objects --------------------------------------------------------
 *
 * Every heap object starts with this header, so all kinds (text, lists and
 * records) share the same retain/release machinery. A header whose count is
 * ES_IMMORTAL is never counted or freed. */

typedef enum { ES_OBJ_TEXT, ES_OBJ_LIST, ES_OBJ_RECORD, ES_OBJ_MAP } EsObjectKind;

#define ES_IMMORTAL SIZE_MAX

typedef struct {
    EsObjectKind kind;
    size_t refs;
} EsObject;

typedef struct {
    EsObject header;
    size_t len;        /* bytes, not counting the NUL */
    const char *chars; /* always NUL-terminated; heap text keeps them right after this struct */
    size_t count;      /* characters (UTF-8), or 0 when not worked out yet (see es_text_chars) */
    size_t cap;        /* heap text: room for this many bytes (plus the NUL), so adding to it can grow in place */
} EsText;

/* A static text object (a literal), for the runtime's own words. */
#define ES_STATIC_TEXT(s) {{ES_OBJ_TEXT, ES_IMMORTAL}, sizeof(s) - 1, s, 0, 0}

static EsText es_empty_text = ES_STATIC_TEXT("");

/* A kind of record the program defines: its name with "a" or "an" ("a
 * dog"), and each field's name, kind ('n' number, 'b' yes/no, 't' text, 'l'
 * list, 'r' record, which may be NULL for nothing, 'm' map) and offset. The compiler
 * writes one per kind (es_rt0, ...), with a C struct that starts with
 * EsRecord and then has the fields. Each variant of a kind with variants
 * ("a circle" of "a shape is one of:") is laid out the same way, with the
 * style ES_VARIANT: such a value never changes, and is equal to another
 * with the same variant and equal fields (es_kind.h). */
typedef struct {
    const char *name;
    int fields;
    const char *const *field_names;
    const char *kinds;
    const size_t *offsets;
    size_t size;
    char style; /* ES_RECORD, ES_VARIANT, ES_BOX (es_kind.h), ES_CLOSURE or ES_CELL (es_closure.h) */
} EsRecordType;

/* A box holds a number, text or yes/no that can be nothing ("a number or
 * nothing"): NULL is nothing, like a record that can be nothing. It never
 * changes, and is equal to another box holding an equal value. A function
 * kept as a value is a closure, and a variable that functions made inside
 * share is kept in a cell (es_closure.h); both are records too, so the
 * cycle check sees them. */
enum { ES_RECORD, ES_VARIANT, ES_BOX, ES_CLOSURE, ES_CELL };

/* A record. Its fields come after this header and own the text, lists and
 * records in them. `prev` and `next` link every record alive while
 * ES_DEBUG_MEMORY is on, so the check at the end can find records that keep
 * each other alive in a circle (see es_finish). */
typedef struct EsRecord {
    EsObject header;
    const EsRecordType *type;
    struct EsRecord *prev, *next;
    size_t mark; /* used by that check */
} EsRecord;

#define ES_FIELD(r, T, offset) (*(T *)((char *)(r) + (offset)))

/* What a list holds (or a map's keys or values). Text, lists, records and
 * maps inside a list are counted references that the list owns (a record
 * item can be NULL, for nothing). */
/* Dates are kept like numbers (seconds, in at.n), but print as dates.
 * Amounts of money too (whole cents, exact below 2^53), printing as money. */
typedef enum {
    ES_ITEMS_NUMBER, ES_ITEMS_YESNO, ES_ITEMS_TEXT, ES_ITEMS_LIST, ES_ITEMS_RECORD, ES_ITEMS_MAP, ES_ITEMS_DATE,
    ES_ITEMS_DOLLARS, ES_ITEMS_EUROS, ES_ITEMS_POUNDS
} EsItems;

static inline bool es_items_money(EsItems items) {
    return items == ES_ITEMS_DOLLARS || items == ES_ITEMS_EUROS || items == ES_ITEMS_POUNDS;
}

/* Kept in at.n: numbers, dates and money. */
static inline bool es_items_numeric(EsItems items) {
    return items == ES_ITEMS_NUMBER || items == ES_ITEMS_DATE || es_items_money(items);
}

struct EsMap;

typedef struct EsList {
    EsObject header;
    EsItems items;
    size_t len;  /* items in it */
    size_t cap;  /* room for this many before growing */
    union {
        double *n;
        bool *b;
        EsText **t;
        struct EsList **l;
        EsRecord **r;
        struct EsMap **m;
        EsObject **o; /* text, lists, records or maps, any of them */
        void *any;
    } at;
} EsList;

static size_t es_item_size(EsItems items) {
    return es_items_numeric(items) ? sizeof(double)
           : items == ES_ITEMS_YESNO                         ? sizeof(bool)
                                                             : sizeof(void *);
}

static bool es_items_counted(EsItems items) {
    return !es_items_numeric(items) && items != ES_ITEMS_YESNO;
}

/* Lists and maps belong to what holds them: copying a list or a map copies
 * the lists and maps inside it too (records and text are shared). */
static bool es_items_copied(EsItems items) {
    return items == ES_ITEMS_LIST || items == ES_ITEMS_MAP;
}

/* What a list variable holds before it's given a list (never read, never
 * changed): it lets releasing need no NULL checks, like es_empty_text. */
static EsList es_no_list = {{ES_OBJ_LIST, ES_IMMORTAL}, ES_ITEMS_NUMBER, 0, 0, {NULL}};

/* A key or value in a map. The map knows what kind its keys and its values
 * are, so the union needs no tag. */
typedef union {
    double n;
    bool b;
    EsText *t;
    EsList *l;
    EsRecord *r;
    struct EsMap *m;
    EsObject *o; /* any counted one */
} EsValue;

/* One entry of a map: its key's hash (0 once it's removed), key and value. */
typedef struct {
    uint64_t hash;
    EsValue key;
    EsValue value;
} EsEntry;

/* A map (es_map.h): its entries in the order they were added, and an index
 * to find them by key. Removed entries stay in place (hash 0) until the
 * entries are packed again. Small maps have no index: looking through a few
 * entries is faster. */
typedef struct EsMap {
    EsObject header;
    EsItems keys;     /* ES_ITEMS_TEXT or ES_ITEMS_NUMBER */
    EsItems values;
    size_t len;       /* entries in the map */
    size_t used;      /* entries[0..used) hold them, with the removed ones */
    size_t cap;       /* room in entries */
    EsEntry *entries;
    uint32_t *index;  /* `slots` places: 0 empty, ES_GONE removed, or 1 + the entry's place; NULL when small */
    size_t slots;     /* a power of two, or 0 */
    size_t filled;    /* places in the index that aren't empty */
} EsMap;

#define ES_GONE UINT32_MAX

/* What a map variable holds before it's given a map, like es_no_list. */
static EsMap es_no_map = {{ES_OBJ_MAP, ES_IMMORTAL}, ES_ITEMS_TEXT, ES_ITEMS_NUMBER, 0, 0, 0, NULL, NULL, 0, 0};

static size_t es_map_bytes(const EsMap *m) { /* what it takes, as counted */
    return sizeof(EsMap) + m->cap * sizeof(EsEntry) + m->slots * sizeof(uint32_t);
}

/* --- Memory --------------------------------------------------------------
 *
 * Live objects and bytes are always counted (it's cheap). Two environment
 * variables turn on checks, used by the tests (read only in programs built
 * with ES_HOOKS; see es_init in es_program.h):
 *   ES_DEBUG_MEMORY=1   at the end, fail if any heap object is still alive
 *   ES_MEMORY_LIMIT=N   fail as soon as live text, lists, records and maps would take more than N bytes */

static struct {
    size_t live_objects;
    size_t live_bytes;
    size_t limit;
    bool check;
    EsRecord *records; /* with check on: every record alive, newest first */
} es_memory;

static _Noreturn void es_out_of_memory(void) {
    fflush(stdout);
    fprintf(stderr, "The program ran out of memory.\n");
    exit(1);
}

/* Counts `size` more live bytes, checking the limit first, so stopping
 * never leaves memory that nothing points to (LeakSanitizer would report it). */
static void es_memory_take(size_t size) {
    if (es_memory.limit && es_memory.live_bytes + size > es_memory.limit) {
        fflush(stdout);
        fprintf(stderr, "Memory check: the program kept more than %zu bytes of text and lists alive at once.\n",
                es_memory.limit);
        exit(70);
    }
    es_memory.live_bytes += size;
}

static EsObject *es_new_object(EsObjectKind kind, size_t size) {
    es_memory_take(size);
    EsObject *obj = malloc(size);
    if (!obj) es_out_of_memory();
    obj->kind = kind;
    obj->refs = 1;
    es_memory.live_objects++;
    return obj;
}

static inline void es_object_release(EsObject *obj);

/* Objects whose count reached zero and whose insides still have to be
 * released. Freeing works through this list instead of calling itself, so
 * freeing a long chain of records (a linked list of a million nodes) can't
 * run out of C stack. It's never freed: it's reused for the whole run. */
static struct {
    EsObject **items;
    size_t len, cap;
    bool busy;
} es_dying;

static void es_dying_add(EsObject *obj) {
    if (es_dying.len == es_dying.cap) {
        size_t cap = es_dying.cap ? es_dying.cap * 2 : 64;
        EsObject **items = realloc(es_dying.items, cap * sizeof(EsObject *));
        if (!items) es_out_of_memory();
        es_dying.items = items;
        es_dying.cap = cap;
    }
    es_dying.items[es_dying.len++] = obj;
}

static void es_unlink_record(EsRecord *r) {
    if (r->prev) r->prev->next = r->next; else es_memory.records = r->next;
    if (r->next) r->next->prev = r->prev;
}

/* Releases what one object holds and frees it. */
static void es_free_one(EsObject *obj) {
    size_t size = 0;
    switch (obj->kind) {
    case ES_OBJ_TEXT: size = sizeof(EsText) + ((EsText *)obj)->cap + 1; break;
    case ES_OBJ_LIST: {
        EsList *l = (EsList *)obj;
        if (es_items_counted(l->items)) {
            for (size_t i = 0; i < l->len; i++) {
                if (l->at.o[i]) es_object_release(l->at.o[i]);
            }
        }
        free(l->at.any);
        size = sizeof(EsList) + l->cap * es_item_size(l->items);
        break;
    }
    case ES_OBJ_RECORD: {
        EsRecord *r = (EsRecord *)obj;
        const EsRecordType *type = r->type;
        for (int i = 0; i < type->fields; i++) {
            char kind = type->kinds[i];
            if (kind == 't' || kind == 'l' || kind == 'r' || kind == 'm') {
                EsObject *field = ES_FIELD(r, EsObject *, type->offsets[i]);
                if (field) es_object_release(field);
            }
        }
        if (es_memory.check) es_unlink_record(r);
        size = type->size;
        break;
    }
    case ES_OBJ_MAP: {
        EsMap *m = (EsMap *)obj;
        for (size_t i = 0; i < m->used; i++) {
            if (m->entries[i].hash == 0) continue;
            if (m->keys == ES_ITEMS_TEXT) es_object_release(m->entries[i].key.o);
            if (es_items_counted(m->values) && m->entries[i].value.o) es_object_release(m->entries[i].value.o);
        }
        size = es_map_bytes(m);
        free(m->entries);
        free(m->index);
        break;
    }
    }
    es_memory.live_bytes -= size;
    es_memory.live_objects--;
    free(obj);
}

/* Frees a heap object whose count reached zero. It's never inlined (and on
 * GCC never specialized) into its callers: es_release only calls it for
 * counted objects, and keeping it out of line keeps that visible to the C
 * compiler, which otherwise sees free() reachable from releases of static
 * immortal text and warns (-Wfree-nonheap-object). Freeing is the rare path,
 * so a real call costs nothing that matters. Text holds nothing, so it's
 * freed at once; a list, record or map found while another is being freed waits
 * its turn in es_dying. */
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noipa, cold))
#elif defined(__clang__)
__attribute__((noinline, cold))
#endif
static void es_free_object(EsObject *obj) {
    if (obj->kind == ES_OBJ_TEXT) {
        es_free_one(obj);
        return;
    }
    if (es_dying.busy) {
        es_dying_add(obj);
        return;
    }
    es_dying.busy = true;
    es_free_one(obj);
    while (es_dying.len > 0) es_free_one(es_dying.items[--es_dying.len]);
    es_dying.busy = false;
}

static inline void es_object_release(EsObject *obj) {
    if (obj->refs != ES_IMMORTAL && --obj->refs == 0) es_free_object(obj);
}

static inline EsText *es_retain(EsText *t) {
    if (t->header.refs != ES_IMMORTAL) t->header.refs++;
    return t;
}

static inline void es_release(EsText *t) {
    es_object_release(&t->header);
}

static inline EsList *es_list_retain(EsList *l) {
    if (l->header.refs != ES_IMMORTAL) l->header.refs++;
    return l;
}

static inline void es_list_release(EsList *l) {
    es_object_release(&l->header);
}

/* Stores into a list variable: takes l, releases what was there. */
static inline void es_list_set(EsList **slot, EsList *l) {
    EsList *old = *slot;
    *slot = l;
    es_list_release(old);
}

/* Releases a list variable at the end of its block. */
static inline void es_list_drop(EsList **slot) {
    es_list_set(slot, &es_no_list);
}

/* Stores into a text variable: takes t, releases what was there. */
static inline void es_set(EsText **slot, EsText *t) {
    EsText *old = *slot;
    *slot = t;
    es_release(old);
}

/* Releases a text variable at the end of its block. */
static inline void es_drop(EsText **slot) {
    es_set(slot, &es_empty_text);
}

static inline EsMap *es_map_retain(EsMap *m) {
    if (m->header.refs != ES_IMMORTAL) m->header.refs++;
    return m;
}

static inline void es_map_release(EsMap *m) {
    es_object_release(&m->header);
}

static inline void es_map_set(EsMap **slot, EsMap *m) {
    EsMap *old = *slot;
    *slot = m;
    es_map_release(old);
}

static inline void es_map_drop(EsMap **slot) {
    es_map_set(slot, &es_no_map);
}

/* Records: like text and lists, except that a record can be NULL (nothing).
 * A function defined with "to" is an immortal closure (es_closure.h). */
static inline EsRecord *es_record_retain(EsRecord *r) {
    if (r && r->header.refs != ES_IMMORTAL) r->header.refs++;
    return r;
}

static inline void es_record_release(EsRecord *r) {
    if (r) es_object_release(&r->header);
}

static inline void es_record_set(EsRecord **slot, EsRecord *r) {
    EsRecord *old = *slot;
    *slot = r;
    es_record_release(old);
}

static inline void es_record_drop(EsRecord **slot) {
    es_record_set(slot, NULL);
}

/* A new record of the kind, for the generated code to fill in. */
static EsRecord *es_record_new(const EsRecordType *type) {
    EsRecord *r = (EsRecord *)es_new_object(ES_OBJ_RECORD, type->size);
    r->type = type;
    r->prev = r->next = NULL;
    if (es_memory.check) {
        r->next = es_memory.records;
        if (r->next) r->next->prev = r;
        es_memory.records = r;
    }
    return r;
}

/* New heap text of len bytes (plus a NUL), for the caller to fill in. */
static EsText *es_text_new(size_t len, char **chars) {
    EsText *t = (EsText *)es_new_object(ES_OBJ_TEXT, sizeof(EsText) + len + 1);
    char *p = (char *)(t + 1);
    p[len] = '\0';
    t->len = len;
    t->chars = p;
    t->count = 0;
    t->cap = len;
    *chars = p;
    return t;
}

/* Heap text copied from len bytes. */
static EsText *es_text_copy(const char *text, size_t len) {
    char *chars;
    EsText *t = es_text_new(len, &chars);
    memcpy(chars, text, len);
    return t;
}

#endif
