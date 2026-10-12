#ifndef DIAG_H
#define DIAG_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include "common/arena.h"
#include "common/util.h"

// A range of source bytes. A zero-length span is shown as a single caret.
typedef struct {
    size_t offset;
    size_t length;
} Span;

// Collects every error from one compiler run so they can be printed together.
typedef struct Diag Diag;

// `source` must outlive the Diag; it is not copied.
Diag *diag_new(Arena *arena, const char *source, size_t source_len);

void diag_error(Diag *diag, Span span, const char *fmt, ...) PRINTF_LIKE(3, 4);

// Something that's allowed but probably not meant ("this can never run").
// Warnings are kept apart from errors: they don't count in diag_count, and
// the program still compiles.
void diag_warning(Diag *diag, Span span, const char *fmt, ...) PRINTF_LIKE(3, 4);

// Adds a line of help (e.g. a "Did you mean" suggestion) to the most recent
// error or warning. There must already be one.
void diag_note(Diag *diag, const char *fmt, ...) PRINTF_LIKE(2, 3);

// What an error is about, as data as well as words, so a notebook can later
// ask it as a question with choices: the problem, what's missing or unclear,
// the kind of value it should be (NULL if that doesn't apply), and the
// possible answers (diag_choice, any number). Both add to the most recent
// error or warning, like diag_note.
typedef enum {
    DIAG_NO_INFO,
    DIAG_MISSING,       // something has to be added
    DIAG_UNKNOWN,       // a name or word EasyScript doesn't know
    DIAG_TWO_MEANINGS,  // it could mean two (or more) things: the choices are the readings
    DIAG_WRONG_KIND,    // a value of another kind was needed
    DIAG_NOT_ALLOWED,   // it can't be written that way here
} DiagProblem;

void diag_info(Diag *diag, DiagProblem problem, const char *missing, const char *kind);
void diag_choice(Diag *diag, const char *fmt, ...) PRINTF_LIKE(2, 3);

// Every error and warning as data, one JSON object per line: where it is,
// its message and notes, and its info if it has any.
void diag_render_data(const Diag *diag, StrBuf *out);

size_t diag_count(const Diag *diag);  // errors only
size_t diag_warning_count(const Diag *diag);

// Puts the errors reported from index `first` on into source order (by where
// their spans start), keeping the order of errors at the same place.
void diag_sort(Diag *diag, size_t first);

// The source is several files joined (modules): file i, called names[i]
// ("helpers.es"), starts at starts[i]. Errors then say which file they're in
// ("Line 4 of helpers.es: ..."), and lines count from each file's start.
void diag_set_files(Diag *diag, const char *const *names, const size_t *starts, size_t files);
size_t diag_file(const Diag *diag, size_t offset);            // 0 when there's one file
const char *diag_file_name(const Diag *diag, size_t file);    // NULL when there's one file

// 1-based line number containing `offset` (in its file).
size_t diag_line(const Diag *diag, size_t offset);

// Formats all errors in the order they were reported:
//
//     Line 4: I don't know anything called "totl".
//         add 5 to totl.
//                  ^^^^
//     Did you mean "total"? You made it on line 1.
//
// Errors are separated by a blank line.
void diag_render(const Diag *diag, StrBuf *out);
void diag_print(const Diag *diag, FILE *out);
// The warnings, laid out like errors ("Line 7 (a warning): ..."), leaving
// out those before line `from_line` (1 for all of them).
void diag_render_warnings(const Diag *diag, size_t from_line, StrBuf *out);
void diag_print_warnings(const Diag *diag, size_t from_line, FILE *out);

#endif
