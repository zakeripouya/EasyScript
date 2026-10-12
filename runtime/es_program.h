/* EasyScript runtime, part 2: starting and ending the program, output, the
 * places runtime calls happen (sites), the call stack, and runtime errors.
 * Part 1, es_memory.h, comes first.
 *
 * Runtime errors print "Line N: <plain English>", the line of the program
 * with carets under the part that failed, and the chain of function calls
 * that led there, to stderr, and exit 1 without cleaning up. */
#ifndef ES_PROGRAM_H
#define ES_PROGRAM_H

#ifndef ES_MEMORY_H
#include "es_memory.h"
#endif

/* --- Starting and ending ------------------------------------------------ */

/* Environment hooks, compiled in only when ES_HOOKS is defined: the
 * compiler defines it for "easyscript run" when its own environment has
 * ES_HOOKS=1, which only the tests and the interactive shell set. Programs
 * made with "easyscript build" never read these variables.
 *
 * The interactive shell reruns the whole session for every line it keeps,
 * and sets two environment variables:
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
    int file;    /* which file of the program (es_source_files), 0 when it has one */
} EsSite;

/* A file of a program made of several ("use helpers."): its name, and the
 * index in es_source of its first line. A program of one file has one,
 * named NULL, and its errors don't name it. */
typedef struct {
    const char *name;
    int first;
} EsSourceFile;

/* The call stack. Every EasyScript function call pushes a frame holding the
 * function (its name and its inputs' names and kinds: 'n' number, 'd' date, 'b'
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
static const EsSourceFile *es_files;

static void es_init(const EsSite *sites, const char *const *source, int source_lines, const EsSourceFile *files) {
    es_site_table = sites;
    es_source_table = source;
    es_source_lines = source_lines;
    es_files = files;
#ifdef ES_HOOKS
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
#endif
}

static bool es_report_cycles(void); /* es_record.h */

/* What the program tells whoever started it when it ends: 0 (it worked)
 * unless "stop the program, saying it failed" or "with code N" says otherwise. */
static int es_status;

/* "the text sent to the program", once it's been read (es_system.h). */
static EsText *es_sent;

/* The end of the program (normally, or "stop the program"), once the
 * generated code has released the variables it can reach. With
 * ES_DEBUG_MEMORY, checks that nothing is left, and explains records that
 * point to each other in a circle, which counting can never free. A stop
 * inside a function skips the check: the variables of main() and of the
 * functions that called it, and values they were still working out, are out
 * of its reach; the operating system reclaims them as the program ends. */
static void es_examples_report(void);

static void es_finish(bool check) {
    fflush(stdout);
    es_examples_report();
    if (es_sent) es_release(es_sent);
    es_sent = NULL;
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
static const char *es_site_text(const EsSite *site) {
    int index = es_files[site->file].first + site->line - 1;
    return site->line >= 1 && index < es_source_lines ? es_source_table[index] : NULL;
}

/* "line 4", or "line 4 of helpers.es" in a program of several files. */
static void es_print_line(const char *word, const EsSite *site) {
    fprintf(stderr, "%s %d", word, site->line);
    if (es_files[site->file].name) fprintf(stderr, " of %s", es_files[site->file].name);
}

static void es_show_source(const EsSite *site) {
    const char *text = es_site_text(site);
    if (!text) return;
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

/* --- Examples ("for example, X is Y.") ---------------------------------------------
 *
 * A wrong example is shown as a warning, and the program goes on. Built for
 * "easyscript test" (ES_TESTING), the program ends by saying how many were
 * right, and fails if any weren't. */

static long es_examples_checked, es_examples_wrong;

/* `got` (owned, or NULL): what the left side of "X is Y" really is (text: `quoted`). */
static void es_example(int site, bool right, EsText *got, bool quoted) {
    es_examples_checked++;
    if (right) {
        if (got) es_release(got);
        return;
    }
    es_examples_wrong++;
    fflush(stdout);
    es_print_line("Line", &es_site_table[site]);
    fprintf(stderr, " (a warning): This example isn't right.\n");
    es_show_source(&es_site_table[site]);
    if (got) {
        const char *quote = quoted ? "\"" : "";
        fprintf(stderr, "It's %s%.*s%s here.\n", quote, (int)got->len, got->chars, quote);
        es_release(got);
    }
}

static void es_examples_report(void) {
#ifdef ES_TESTING
    if (es_examples_checked == 0) {
        fprintf(stderr, "This program has no examples to check. Write one like \"for example, total is 12.\"\n");
    } else if (es_examples_wrong == 0) {
        fprintf(stderr, "%s %ld example%s right.\n", es_examples_checked == 1 ? "The" : "All", es_examples_checked,
                es_examples_checked == 1 ? " is" : "s are");
    } else {
        fprintf(stderr, "%ld of %ld example%s right.\n", es_examples_checked - es_examples_wrong, es_examples_checked,
                es_examples_checked - es_examples_wrong == 1 ? " is" : "s are");
        if (es_status == 0) es_status = 1;
    }
#endif
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
static EsText *es_date_text(double date);               /* es_time.h */

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
        if (kind == 'n' || kind == 'd') {
            EsText *t = (kind == 'd' ? es_date_text : es_number_text)(es_frame_input(frame, i).n);
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
        const EsSite *call = &es_site_table[es_stack[i].call];
        if (i == 0) {
            es_print_line("called from line", call);
            const char *text = es_site_text(call);
            if (text) {
                while (*text == ' ' || *text == '\t') text++;
                fprintf(stderr, ": %s", text);
            }
            fputc('\n', stderr);
            break;
        }
        fprintf(stderr, "called from \"%s\" on", es_stack[i - 1].fn->name);
        es_print_line(" line", call);
        es_print_inputs(&es_stack[i - 1]);
        fputc('\n', stderr);
        long repeats = 0;
        while (es_same_call(i)) {
            i--;
            repeats++;
        }
        if (repeats == 1) { /* two the same: show both */
            fprintf(stderr, "called from \"%s\" on", es_stack[i - 1].fn->name);
            es_print_line(" line", call);
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
    es_print_line("Line", &es_site_table[site]);
    fputs(": ", stderr);
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
