/* EasyScript runtime, part 1: heap objects (text, lists, records and maps) and memory.
 *
 * The runtime is seven files, embedded into the compiler at build time (in
 * this order: es_memory.h, es_runtime.h, es_list.h, es_sort.h, es_text.h,
 * es_record.h, es_map.h) and copied to the top of every generated C program, so
 * programs need nothing else to build.
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
 * Runtime errors print "Line N: <plain English>", the line of the program
 * with carets under the part that failed, and the chain of function calls
 * that led there, to stderr, and exit 1 without cleaning up.
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
 * EsRecord and then has the fields. */
typedef struct {
    const char *name;
    int fields;
    const char *const *field_names;
    const char *kinds;
    const size_t *offsets;
    size_t size;
} EsRecordType;

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
typedef enum { ES_ITEMS_NUMBER, ES_ITEMS_YESNO, ES_ITEMS_TEXT, ES_ITEMS_LIST, ES_ITEMS_RECORD, ES_ITEMS_MAP } EsItems;

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
    return items == ES_ITEMS_NUMBER ? sizeof(double) : items == ES_ITEMS_YESNO ? sizeof(bool) : sizeof(void *);
}

static bool es_items_counted(EsItems items) {
    return items != ES_ITEMS_NUMBER && items != ES_ITEMS_YESNO;
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
 * variables turn on checks, used by the tests:
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

/* Records: like text and lists, except that a record can be NULL (nothing). */
static inline EsRecord *es_record_retain(EsRecord *r) {
    if (r) r->header.refs++;
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

/* --- Starting and ending ------------------------------------------------ */

/* The interactive shell reruns the whole session for every line it keeps,
 * and sets two environment variables (normal programs see neither):
 *   ES_SKIP_OUTPUT=N  don't print the first N bytes (shown on earlier runs)
 *   ES_ANSWERS=path   answers to earlier "ask"s, one per line, used before
 *                     reading the keyboard; new answers are added to it */
static size_t es_skip_output;
static FILE *es_answers;
static bool es_answers_used_up;

static void es_close_answers(void) {
    if (es_answers) fclose(es_answers);
}

/* --- Where things happen --------------------------------------------------
 *
 * A runtime call that can fail is given a site: the index of its place in
 * the program in es_sites, a table the compiler writes next to the program's
 * source lines (es_source). Errors use them to show the line and carets the
 * way compile errors do. */
typedef struct {
    int line;    /* from 1 */
    int column;  /* from 1, in characters (a tab counts as one) */
    int length;  /* in bytes */
} EsSite;

/* The call stack. Every EasyScript function call pushes a frame holding the
 * function (its name and its inputs' names and kinds: 'n' number, 'b'
 * yes/no, 't' text, 'l' list, 'r' record, 'm' map), the values it was given, and the site of the
 * call. Errors print the chain. The frames are an array as deep as the
 * recursion limit, so pushing is a few stores and popping is the depth
 * counter going down. A function with one input (the usual case) keeps its
 * value in the frame; with more, the values are copied onto es_inputs, a
 * growing array, and the frame records where they start. */
typedef union {
    double n;
    bool b;
    EsText *t;   /* retained by the function for as long as the frame lives */
    EsRecord *r; /* the same */
    void *list;  /* a list or a map */
} EsArg;

typedef struct {
    const char *name;
    int inputs;
    const char *const *input_names;
    const char *kinds;
} EsFunction;

typedef struct {
    const EsFunction *fn;
    int call; /* site of the call, in the caller */
    union {
        EsArg one;    /* a function with one input: its value */
        size_t many;  /* with more: where their values start in es_inputs */
    } in;
} EsFrame;

static EsArg *es_inputs; /* the values of the calls in progress with several inputs */
static size_t es_inputs_len, es_inputs_cap;

static EsArg es_frame_input(const EsFrame *frame, int i) {
    return frame->fn->inputs == 1 ? frame->in.one : es_inputs[frame->in.many + (size_t)i];
}

#define ES_MAX_DEPTH 10000
static EsFrame es_stack[ES_MAX_DEPTH]; /* es_stack[0] is the outermost call */
static int es_depth;                   /* calls in progress */

/* Set by es_init to the program's es_sites and es_source. */
static const EsSite *es_site_table;
static const char *const *es_source_table;
static int es_source_lines;

static void es_init(const EsSite *sites, const char *const *source, int source_lines) {
    es_site_table = sites;
    es_source_table = source;
    es_source_lines = source_lines;
    const char *skip = getenv("ES_SKIP_OUTPUT");
    if (skip) es_skip_output = (size_t)strtoull(skip, NULL, 10);
    const char *answers = getenv("ES_ANSWERS");
    if (answers) {
        es_answers = fopen(answers, "a+");
        if (es_answers) rewind(es_answers);
        atexit(es_close_answers);
        setvbuf(stdin, NULL, _IONBF, 0); /* read only our own line: the shell reads the rest */
    }
    const char *check = getenv("ES_DEBUG_MEMORY");
    es_memory.check = check && *check && strcmp(check, "0") != 0;
    const char *limit = getenv("ES_MEMORY_LIMIT");
    if (limit) es_memory.limit = (size_t)strtoull(limit, NULL, 10);
}

static bool es_report_cycles(void); /* es_record.h */

/* The end of the program (normally, or "stop the program"), once the
 * generated code has released the variables it can reach. With
 * ES_DEBUG_MEMORY, checks that nothing is left, and explains records that
 * point to each other in a circle, which counting can never free. A stop
 * inside a function skips the check: the variables of main() and of the
 * functions that called it, and values they were still working out, are out
 * of its reach; the operating system reclaims them as the program ends. */
static void es_finish(bool check) {
    fflush(stdout);
    if (check && es_memory.check && es_memory.live_objects != 0) {
        if (es_report_cycles()) exit(70);
        fprintf(stderr, "Memory check: %zu value%s still alive at the end of the program (%zu bytes).\n",
                es_memory.live_objects, es_memory.live_objects == 1 ? " was" : "s were", es_memory.live_bytes);
        exit(70);
    }
}

/* Everything the program prints goes through here. */
static void es_out(const char *s, size_t n) {
    if (es_skip_output >= n) {
        es_skip_output -= n;
        return;
    }
    s += es_skip_output;
    n -= es_skip_output;
    es_skip_output = 0;
    fwrite(s, 1, n, stdout);
}

/* --- Errors ------------------------------------------------------------- */

static bool es_continuation_byte(char c) {
    return ((unsigned char)c & 0xC0) == 0x80;
}

/* The site's source line, indented, and carets under it: the same layout as
 * compile errors (tabs copied into the padding, one caret per character,
 * clamped to the line, at least one). */
static void es_show_source(const EsSite *site) {
    if (site->line < 1 || site->line > es_source_lines) return;
    const char *text = es_source_table[site->line - 1];
    size_t end = strlen(text);
    fprintf(stderr, "    %s\n    ", text);
    size_t offset = 0;
    for (int column = 1; column < site->column && offset < end; column++) {  /* one character at a time */
        fputc(text[offset] == '\t' ? '\t' : ' ', stderr);
        offset++;
        while (offset < end && es_continuation_byte(text[offset])) offset++;
    }
    size_t length = (size_t)site->length < end - offset ? (size_t)site->length : end - offset;
    size_t carets = 0;
    for (size_t i = 0; i < length; i++) carets += !es_continuation_byte(text[offset + i]);
    if (carets == 0) carets = 1;
    while (carets--) fputc('^', stderr);
    fputc('\n', stderr);
}

/* 4998 as "4,998". */
static void es_print_count(long n) {
    if (n >= 1000) {
        es_print_count(n / 1000);
        fprintf(stderr, ",%03ld", n % 1000);
    } else {
        fprintf(stderr, "%ld", n);
    }
}

/* "(count was 0, name was "Ada")": the inputs that are numbers, yes/no,
 * text or records (long ones cut short; lists and maps are left out);
 * nothing if there are none of those. */
static EsText *es_number_text(double x);
static EsText *es_record_short_text(const EsRecord *r); /* es_record.h */

static void es_print_cut(const EsText *t, size_t most, bool quoted) {
    size_t shown = 0, chars = 0;
    while (shown < t->len && chars < most) {
        shown++;
        if (shown == t->len || !es_continuation_byte(t->chars[shown])) chars++;
    }
    fprintf(stderr, "%s%.*s%s%s", quoted ? "\"" : "", (int)shown, t->chars, shown < t->len ? "..." : "", quoted ? "\"" : "");
}

static void es_print_inputs(const EsFrame *frame) {
    bool open = false;
    for (int i = 0; i < frame->fn->inputs; i++) {
        char kind = frame->fn->kinds[i];
        if (kind == 'l' || kind == 'm') continue;
        fprintf(stderr, "%s%s was ", open ? ", " : " (", frame->fn->input_names[i]);
        open = true;
        if (kind == 'n') {
            EsText *t = es_number_text(es_frame_input(frame, i).n);
            fputs(t->chars, stderr);
            es_release(t);
        } else if (kind == 'b') {
            fputs(es_frame_input(frame, i).b ? "yes" : "no", stderr);
        } else if (kind == 'r') {
            EsText *t = es_record_short_text(es_frame_input(frame, i).r);
            es_print_cut(t, 60, false);
            es_release(t);
        } else {
            es_print_cut(es_frame_input(frame, i).t, 30, true);
        }
    }
    if (open) fputc(')', stderr);
}

/* Whether calls i and i - 1 look the same in the chain: called from the
 * same place in the same function. */
static bool es_same_call(int i) {
    return i >= 2 && es_stack[i].call == es_stack[i - 1].call && es_stack[i - 1].fn == es_stack[i - 2].fn;
}

/* The chain of calls, innermost first:
 *   while working out "average" (count was 0)
 *   called from "report" on line 5
 *   ...it called itself 4,998 more times...
 *   called from line 8: say report */
static void es_show_calls(void) {
    if (es_depth == 0) return;
    fprintf(stderr, "while working out \"%s\"", es_stack[es_depth - 1].fn->name);
    es_print_inputs(&es_stack[es_depth - 1]);
    fputc('\n', stderr);
    for (int i = es_depth - 1; i >= 0; i--) {
        int line = es_site_table[es_stack[i].call].line;
        if (i == 0) {
            fprintf(stderr, "called from line %d", line);
            if (line >= 1 && line <= es_source_lines) {
                const char *text = es_source_table[line - 1];
                while (*text == ' ' || *text == '\t') text++;
                fprintf(stderr, ": %s", text);
            }
            fputc('\n', stderr);
            break;
        }
        fprintf(stderr, "called from \"%s\" on line %d", es_stack[i - 1].fn->name, line);
        es_print_inputs(&es_stack[i - 1]);
        fputc('\n', stderr);
        long repeats = 0;
        while (es_same_call(i)) {
            i--;
            repeats++;
        }
        if (repeats == 1) { /* two the same: show both */
            fprintf(stderr, "called from \"%s\" on line %d", es_stack[i - 1].fn->name, line);
            es_print_inputs(&es_stack[i - 1]);
            fputc('\n', stderr);
        } else if (repeats > 1) {
            fputs("...it called itself ", stderr);
            es_print_count(repeats);
            fputs(" more times...\n", stderr);
        }
    }
}

/* Prints "Line N: message", the line with carets, the hint on its own line
 * if there is one, and the chain of calls. */
static _Noreturn void es_fail(int site, const char *hint, const char *fmt, ...) {
    va_list args;
    fflush(stdout);
    fprintf(stderr, "Line %d: ", es_site_table[site].line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    es_show_source(&es_site_table[site]);
    if (hint) fprintf(stderr, "%s\n", hint);
    es_show_calls();
    exit(1);
}

static const char *es_reason(int err) {
    switch (err) {
    case ENOENT: return "it doesn't exist";
    case EACCES: return "permission was denied";
    case EISDIR: return "it's a folder, not a file";
    case ENOTDIR: return "part of its path isn't a folder";
    default: return strerror(err);
    }
}

#endif
