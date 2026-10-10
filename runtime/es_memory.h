/* EasyScript runtime, part 1: heap objects (text and lists) and memory.
 *
 * The runtime is three files, embedded into the compiler at build time (in
 * this order: es_memory.h, es_runtime.h, es_list.h) and copied to the top of
 * every generated C program, so programs need nothing else to build.
 * Everything is static; generated code calls it.
 *
 * The compiler knows the kind of every value, so generated code uses plain C
 * types: a number is a double, yes/no is a bool, text is an EsText *, and a
 * list is an EsList *. Text and lists live on the heap with a reference
 * count, and are freed as soon as nothing uses them (see docs/memory.md);
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
 * Every heap object starts with this header, so all kinds (text, lists, and
 * later records) share the same retain/release machinery. A header whose
 * count is ES_IMMORTAL is never counted or freed. */

typedef enum { ES_OBJ_TEXT, ES_OBJ_LIST } EsObjectKind;

#define ES_IMMORTAL SIZE_MAX

typedef struct {
    EsObjectKind kind;
    size_t refs;
} EsObject;

typedef struct {
    EsObject header;
    size_t len;        /* bytes, not counting the NUL */
    const char *chars; /* always NUL-terminated; heap text keeps them right after this struct */
} EsText;

/* A static text object (a literal), for the runtime's own words. */
#define ES_STATIC_TEXT(s) {{ES_OBJ_TEXT, ES_IMMORTAL}, sizeof(s) - 1, s}

static EsText es_empty_text = ES_STATIC_TEXT("");

/* What a list holds. Text and lists inside a list are counted references
 * that the list owns. */
typedef enum { ES_ITEMS_NUMBER, ES_ITEMS_YESNO, ES_ITEMS_TEXT, ES_ITEMS_LIST } EsItems;

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
        EsObject **o; /* text or lists, either way */
        void *any;
    } at;
} EsList;

static size_t es_item_size(EsItems items) {
    return items == ES_ITEMS_NUMBER ? sizeof(double) : items == ES_ITEMS_YESNO ? sizeof(bool) : sizeof(void *);
}

static bool es_items_counted(EsItems items) {
    return items == ES_ITEMS_TEXT || items == ES_ITEMS_LIST;
}

/* What a list variable holds before it's given a list (never read, never
 * changed): it lets releasing need no NULL checks, like es_empty_text. */
static EsList es_no_list = {{ES_OBJ_LIST, ES_IMMORTAL}, ES_ITEMS_NUMBER, 0, 0, {NULL}};

/* --- Memory --------------------------------------------------------------
 *
 * Live objects and bytes are always counted (it's cheap). Two environment
 * variables turn on checks, used by the tests:
 *   ES_DEBUG_MEMORY=1   at the end, fail if any heap object is still alive
 *   ES_MEMORY_LIMIT=N   fail as soon as live text and lists would take more than N bytes */

static struct {
    size_t live_objects;
    size_t live_bytes;
    size_t limit;
    bool check;
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

/* Frees a heap object whose count reached zero. It's never inlined (and on
 * GCC never specialized) into its callers: es_release only calls it for
 * counted objects, and keeping it out of line keeps that visible to the C
 * compiler, which otherwise sees free() reachable from releases of static
 * immortal text and warns (-Wfree-nonheap-object). Freeing is the rare path,
 * so a real call costs nothing that matters. */
#if defined(__GNUC__) && !defined(__clang__)
__attribute__((noipa, cold))
#elif defined(__clang__)
__attribute__((noinline, cold))
#endif
static void es_free_object(EsObject *obj) {
    size_t size = 0;
    switch (obj->kind) {
    case ES_OBJ_TEXT: size = sizeof(EsText) + ((EsText *)obj)->len + 1; break;
    case ES_OBJ_LIST: {
        EsList *l = (EsList *)obj;
        if (es_items_counted(l->items)) {
            for (size_t i = 0; i < l->len; i++) es_object_release(l->at.o[i]);
        }
        free(l->at.any);
        size = sizeof(EsList) + l->cap * es_item_size(l->items);
        break;
    }
    }
    es_memory.live_bytes -= size;
    es_memory.live_objects--;
    free(obj);
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

/* New heap text of len bytes (plus a NUL), for the caller to fill in. */
static EsText *es_text_new(size_t len, char **chars) {
    EsText *t = (EsText *)es_new_object(ES_OBJ_TEXT, sizeof(EsText) + len + 1);
    char *p = (char *)(t + 1);
    p[len] = '\0';
    t->len = len;
    t->chars = p;
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
 * yes/no, 't' text, 'l' list), the values it was given, and the site of the
 * call. Errors print the chain. The frames are an array as deep as the
 * recursion limit, so pushing is a few stores and popping is the depth
 * counter going down. A function with one input (the usual case) keeps its
 * value in the frame; with more, the values are copied onto es_inputs, a
 * growing array, and the frame records where they start. */
typedef union {
    double n;
    bool b;
    EsText *t; /* retained by the function for as long as the frame lives */
    void *list;
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

/* The end of the program (normally, or "stop the program"), once the
 * generated code has released the variables it can reach. With
 * ES_DEBUG_MEMORY, checks that nothing is left. A stop inside a function
 * skips the check: the variables of main() and of the functions that called
 * it, and values they were still working out, are out of its reach; the
 * operating system reclaims them as the program ends. */
static void es_finish(bool check) {
    fflush(stdout);
    if (check && es_memory.check && es_memory.live_objects != 0) {
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

/* "(count was 0, name was "Ada")": the inputs that are numbers, yes/no or
 * text (long text cut short); nothing if there are none of those. */
static EsText *es_number_text(double x);
static void es_print_inputs(const EsFrame *frame) {
    bool open = false;
    for (int i = 0; i < frame->fn->inputs; i++) {
        char kind = frame->fn->kinds[i];
        if (kind == 'l') continue;
        fprintf(stderr, "%s%s was ", open ? ", " : " (", frame->fn->input_names[i]);
        open = true;
        if (kind == 'n') {
            EsText *t = es_number_text(es_frame_input(frame, i).n);
            fputs(t->chars, stderr);
            es_release(t);
        } else if (kind == 'b') {
            fputs(es_frame_input(frame, i).b ? "yes" : "no", stderr);
        } else {
            const EsText *t = es_frame_input(frame, i).t;
            size_t shown = 0, chars = 0;
            while (shown < t->len && chars < 30) {
                shown++;
                if (shown == t->len || !es_continuation_byte(t->chars[shown])) chars++;
            }
            fprintf(stderr, "\"%.*s%s\"", (int)shown, t->chars, shown < t->len ? "..." : "");
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
