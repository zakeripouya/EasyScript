#ifndef DIAG_H
#define DIAG_H

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

// Adds a line of help (e.g. a "Did you mean" suggestion) to the most recent
// error. There must already be an error.
void diag_note(Diag *diag, const char *fmt, ...) PRINTF_LIKE(2, 3);

size_t diag_count(const Diag *diag);

// 1-based line number containing `offset`.
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

#endif
