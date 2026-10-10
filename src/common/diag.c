#include <assert.h>
#include <stdarg.h>
#include "common/diag.h"

#define SOURCE_INDENT "    "

typedef struct {
    Span span;
    const char *message;
    Vec(const char *) notes;
} DiagError;

struct Diag {
    Arena *arena;
    const char *source;
    size_t source_len;
    Vec(size_t) line_starts;
    Vec(DiagError) errors;
};

static void index_lines(Diag *diag) {
    vec_push(diag->arena, &diag->line_starts, (size_t)0);
    for (size_t i = 0; i < diag->source_len; i++) {
        if (diag->source[i] == '\n') {
            vec_push(diag->arena, &diag->line_starts, i + 1);
        }
    }
}

Diag *diag_new(Arena *arena, const char *source, size_t source_len) {
    Diag *diag = arena_alloc(arena, sizeof(Diag));
    diag->arena = arena;
    diag->source = source;
    diag->source_len = source_len;
    index_lines(diag);
    return diag;
}

void diag_error(Diag *diag, Span span, const char *fmt, ...) {
    DiagError error = {0};
    error.span = span;
    va_list args;
    va_start(args, fmt);
    error.message = arena_vsprintf(diag->arena, fmt, args);
    va_end(args);
    vec_push(diag->arena, &diag->errors, error);
}

void diag_note(Diag *diag, const char *fmt, ...) {
    assert(diag->errors.len > 0 && "diag_note needs a preceding diag_error");
    va_list args;
    va_start(args, fmt);
    const char *note = arena_vsprintf(diag->arena, fmt, args);
    va_end(args);
    vec_push(diag->arena, &vec_last(&diag->errors).notes, note);
}

size_t diag_count(const Diag *diag) {
    return diag->errors.len;
}

void diag_sort(Diag *diag, size_t first) {
    DiagError *errors = diag->errors.items;
    for (size_t i = first + 1; i < diag->errors.len; i++) {  // insertion sort: stable, and lists are short
        DiagError error = errors[i];
        size_t j = i;
        while (j > first && errors[j - 1].span.offset > error.span.offset) {
            errors[j] = errors[j - 1];
            j--;
        }
        errors[j] = error;
    }
}

// Index into line_starts of the line containing `offset`.
static size_t line_index(const Diag *diag, size_t offset) {
    if (offset > diag->source_len) offset = diag->source_len;
    size_t lo = 0;
    size_t hi = diag->line_starts.len;
    while (hi - lo > 1) {
        size_t mid = lo + (hi - lo) / 2;
        if (diag->line_starts.items[mid] <= offset) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return lo;
}

size_t diag_line(const Diag *diag, size_t offset) {
    return line_index(diag, offset) + 1;
}

// Offset just past the visible text of the line starting at `start`
// (excludes the newline and a trailing carriage return).
static size_t line_end(const Diag *diag, size_t start) {
    size_t end = start;
    while (end < diag->source_len && diag->source[end] != '\n') {
        end++;
    }
    if (end > start && diag->source[end - 1] == '\r') end--;
    return end;
}

// UTF-8 continuation bytes don't start a new character, so they take no
// column of their own.
static int is_continuation_byte(char c) {
    return ((unsigned char)c & 0xC0) == 0x80;
}

static size_t count_chars(const char *s, size_t n) {
    size_t chars = 0;
    for (size_t i = 0; i < n; i++) {
        if (!is_continuation_byte(s[i])) chars++;
    }
    return chars;
}

// Writes the indentation that puts a caret under source[offset]. Tabs are
// copied so the caret lines up however wide the terminal draws them.
static void render_padding(const Diag *diag, StrBuf *out, size_t start, size_t offset) {
    for (size_t i = start; i < offset; i++) {
        char c = diag->source[i];
        if (c == '\t') {
            sb_append_char(out, '\t');
        } else if (!is_continuation_byte(c)) {
            sb_append_char(out, ' ');
        }
    }
}

static void render_error(const Diag *diag, const DiagError *error, StrBuf *out) {
    size_t index = line_index(diag, error->span.offset);
    size_t start = diag->line_starts.items[index];
    size_t end = line_end(diag, start);
    size_t offset = error->span.offset;
    if (offset > end) offset = end;

    size_t length = error->span.length;
    if (length > end - offset) length = end - offset;
    size_t carets = count_chars(diag->source + offset, length);
    if (carets == 0) carets = 1;

    sb_appendf(out, "Line %zu: %s\n", index + 1, error->message);
    sb_append(out, SOURCE_INDENT);
    sb_append_n(out, diag->source + start, end - start);
    sb_append_char(out, '\n');
    sb_append(out, SOURCE_INDENT);
    render_padding(diag, out, start, offset);
    sb_append_repeat(out, '^', carets);
    sb_append_char(out, '\n');
    for (size_t i = 0; i < error->notes.len; i++) {
        sb_appendf(out, "%s\n", error->notes.items[i]);
    }
}

void diag_render(const Diag *diag, StrBuf *out) {
    for (size_t i = 0; i < diag->errors.len; i++) {
        if (i > 0) sb_append_char(out, '\n');
        render_error(diag, &diag->errors.items[i], out);
    }
}

void diag_print(const Diag *diag, FILE *out) {
    StrBuf text;
    sb_init(&text, diag->arena);
    diag_render(diag, &text);
    fwrite(text.data, 1, text.len, out);
}
