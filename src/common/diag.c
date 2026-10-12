#include <assert.h>
#include <stdarg.h>
#include "common/diag.h"

#define SOURCE_INDENT "    "

typedef struct {
    Span span;
    const char *message;
    Vec(const char *) notes;
    bool warning;
    DiagProblem problem;  // DIAG_NO_INFO unless diag_info was called
    const char *missing;
    const char *kind;
    Vec(const char *) choices;
} DiagError;

struct Diag {
    Arena *arena;
    const char *source;
    size_t source_len;
    Vec(size_t) line_starts;
    Vec(DiagError) errors;
    Vec(DiagError) warnings;
    bool warned_last;  // diag_note adds to the latest warning, not the latest error
    const char *const *file_names;  // diag_set_files: several files joined into one source
    const size_t *file_starts;
    size_t files;
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
    diag->warned_last = false;
}

void diag_warning(Diag *diag, Span span, const char *fmt, ...) {
    DiagError warning = {0};
    warning.span = span;
    warning.warning = true;
    va_list args;
    va_start(args, fmt);
    warning.message = arena_vsprintf(diag->arena, fmt, args);
    va_end(args);
    vec_push(diag->arena, &diag->warnings, warning);
    diag->warned_last = true;
}

void diag_note(Diag *diag, const char *fmt, ...) {
    assert((diag->warned_last || diag->errors.len > 0) && "diag_note needs a preceding diag_error");
    va_list args;
    va_start(args, fmt);
    const char *note = arena_vsprintf(diag->arena, fmt, args);
    va_end(args);
    DiagError *last = diag->warned_last ? &vec_last(&diag->warnings) : &vec_last(&diag->errors);
    vec_push(diag->arena, &last->notes, note);
}

static DiagError *latest(Diag *diag) {
    assert((diag->warned_last || diag->errors.len > 0) && "diag_info needs a preceding diag_error");
    return diag->warned_last ? &vec_last(&diag->warnings) : &vec_last(&diag->errors);
}

void diag_info(Diag *diag, DiagProblem problem, const char *missing, const char *kind) {
    DiagError *last = latest(diag);
    last->problem = problem;
    last->missing = missing ? arena_strdup(diag->arena, missing) : NULL;
    last->kind = kind ? arena_strdup(diag->arena, kind) : NULL;
}

void diag_choice(Diag *diag, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    const char *choice = arena_vsprintf(diag->arena, fmt, args);
    va_end(args);
    vec_push(diag->arena, &latest(diag)->choices, choice);
}

size_t diag_warning_count(const Diag *diag) {
    return diag->warnings.len;
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

void diag_set_files(Diag *diag, const char *const *names, const size_t *starts, size_t files) {
    diag->file_names = names;
    diag->file_starts = starts;
    diag->files = files;
}

// Which file holds `offset` (0 when there's one).
static size_t file_of(const Diag *diag, size_t offset) {
    size_t file = 0;
    while (file + 1 < diag->files && diag->file_starts[file + 1] <= offset) file++;
    return file;
}

size_t diag_file(const Diag *diag, size_t offset) {
    return file_of(diag, offset);
}

const char *diag_file_name(const Diag *diag, size_t file) {
    return file < diag->files ? diag->file_names[file] : NULL;
}

size_t diag_line(const Diag *diag, size_t offset) {
    size_t first = diag->files ? line_index(diag, diag->file_starts[file_of(diag, offset)]) : 0;
    return line_index(diag, offset) - first + 1;
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

    sb_appendf(out, "Line %zu", diag_line(diag, error->span.offset));
    const char *file = diag->files > 1 ? diag->file_names[file_of(diag, error->span.offset)] : NULL;
    if (file) sb_appendf(out, " of %s", file);  // NULL: a name not shown
    sb_appendf(out, "%s: %s\n", error->warning ? " (a warning)" : "", error->message);
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

void diag_render_warnings(const Diag *diag, size_t from_line, StrBuf *out) {
    bool first = true;
    for (size_t i = 0; i < diag->warnings.len; i++) {
        const DiagError *warning = &diag->warnings.items[i];
        if (diag_line(diag, warning->span.offset) < from_line) continue;
        if (!first) sb_append_char(out, '\n');
        render_error(diag, warning, out);
        first = false;
    }
}

void diag_print_warnings(const Diag *diag, size_t from_line, FILE *out) {
    StrBuf text;
    sb_init(&text, diag->arena);
    diag_render_warnings(diag, from_line, &text);
    fwrite(text.data, 1, text.len, out);
}

void diag_print(const Diag *diag, FILE *out) {
    StrBuf text;
    sb_init(&text, diag->arena);
    diag_render(diag, &text);
    fwrite(text.data, 1, text.len, out);
}

// --- Errors as data ------------------------------------------------------------------

static void json_string(StrBuf *out, const char *s) {
    if (!s) {
        sb_append(out, "null");
        return;
    }
    sb_append_char(out, '"');
    for (; *s; s++) {
        unsigned char c = (unsigned char)*s;
        if (c == '"' || c == '\\') {
            sb_append_char(out, '\\');
            sb_append_char(out, (char)c);
        } else if (c < 0x20) {
            sb_appendf(out, "\\u%04x", c);
        } else {
            sb_append_char(out, (char)c);
        }
    }
    sb_append_char(out, '"');
}

static void json_strings(StrBuf *out, const char *const *items, size_t n) {
    sb_append_char(out, '[');
    for (size_t i = 0; i < n; i++) {
        if (i > 0) sb_append(out, ", ");
        json_string(out, items[i]);
    }
    sb_append_char(out, ']');
}

static const char *problem_name(DiagProblem problem) {
    static const char *const names[] = {NULL, "missing", "unknown", "two meanings", "wrong kind", "not allowed"};
    return names[problem];
}

static void render_data(const Diag *diag, const DiagError *error, StrBuf *out) {
    size_t start = diag->line_starts.items[line_index(diag, error->span.offset)];
    size_t offset = error->span.offset < diag->source_len ? error->span.offset : diag->source_len;
    sb_appendf(out, "{\"line\": %zu, \"column\": %zu, \"file\": ", diag_line(diag, error->span.offset),
               count_chars(diag->source + start, offset - start) + 1);
    json_string(out, diag->files > 1 ? diag->file_names[file_of(diag, error->span.offset)] : NULL);
    sb_appendf(out, ", \"warning\": %s, \"message\": ", error->warning ? "true" : "false");
    json_string(out, error->message);
    sb_append(out, ", \"notes\": ");
    json_strings(out, error->notes.items, error->notes.len);
    sb_append(out, ", \"problem\": ");
    json_string(out, problem_name(error->problem));
    sb_append(out, ", \"missing\": ");
    json_string(out, error->missing);
    sb_append(out, ", \"kind\": ");
    json_string(out, error->kind);
    sb_append(out, ", \"choices\": ");
    json_strings(out, error->choices.items, error->choices.len);
    sb_append(out, "}\n");
}

void diag_render_data(const Diag *diag, StrBuf *out) {
    for (size_t i = 0; i < diag->errors.len; i++) render_data(diag, &diag->errors.items[i], out);
    for (size_t i = 0; i < diag->warnings.len; i++) render_data(diag, &diag->warnings.items[i], out);
}
