/* EasyScript runtime, part 1: text objects and memory.
 *
 * The runtime is two files, embedded into the compiler at build time (in this
 * order) and copied to the top of every generated C program, so programs need
 * nothing else to build. Everything is static; generated code calls it.
 *
 * The compiler knows the kind of every value, so generated code uses plain C
 * types: a number is a double, yes/no is a bool, and text is an EsText *.
 * Only text lives on the heap. Text made while the program runs has a
 * reference count and is freed as soon as nothing uses it (see
 * docs/memory.md); text written in the program (literals, constants) is a
 * static object marked immortal, which is never counted or freed.
 *
 * Ownership: every text an expression produces is owned. Runtime functions
 * that take EsText * consume it (release it, or hand it back in their
 * result); reading a variable retains; storing into one (es_set) takes the
 * new text and releases the old one. Runtime errors print
 * "Line N: <plain English>" to stderr and exit 1 without cleaning up.
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

/* --- Heap objects --------------------------------------------------------
 *
 * Every heap object starts with this header, so later kinds (lists, records)
 * share the same retain/release machinery. A header whose count is
 * ES_IMMORTAL is never counted or freed. */

typedef enum { ES_OBJ_TEXT } EsObjectKind;

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

/* --- Memory --------------------------------------------------------------
 *
 * Live objects and bytes are always counted (it's cheap). Two environment
 * variables turn on checks, used by the tests:
 *   ES_DEBUG_MEMORY=1   at the end, fail if any heap object is still alive
 *   ES_MEMORY_LIMIT=N   fail as soon as live heap text takes more than N bytes */

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

static EsObject *es_new_object(EsObjectKind kind, size_t size) {
    EsObject *obj = malloc(size);
    if (!obj) es_out_of_memory();
    obj->kind = kind;
    obj->refs = 1;
    es_memory.live_objects++;
    es_memory.live_bytes += size;
    if (es_memory.limit && es_memory.live_bytes > es_memory.limit) {
        fflush(stdout);
        fprintf(stderr, "Memory check: the program kept more than %zu bytes of text alive at once.\n", es_memory.limit);
        exit(70);
    }
    return obj;
}

static void es_free_object(EsObject *obj) {
    switch (obj->kind) {
    case ES_OBJ_TEXT: es_memory.live_bytes -= sizeof(EsText) + ((EsText *)obj)->len + 1; break;
    }
    es_memory.live_objects--;
    free(obj);
}

static inline EsText *es_retain(EsText *t) {
    if (t->header.refs != ES_IMMORTAL) t->header.refs++;
    return t;
}

static inline void es_release(EsText *t) {
    if (t->header.refs != ES_IMMORTAL && --t->header.refs == 0) es_free_object(&t->header);
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

static void es_init(void) {
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
        fprintf(stderr, "Memory check: %zu text value%s still alive at the end of the program (%zu bytes).\n",
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

/* Prints "Line N: message", then the hint on its own line if there is one. */
static _Noreturn void es_fail(int line, const char *hint, const char *fmt, ...) {
    va_list args;
    fflush(stdout);
    fprintf(stderr, "Line %d: ", line);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
    if (hint) fprintf(stderr, "%s\n", hint);
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
