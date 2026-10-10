#ifndef UTIL_H
#define UTIL_H

#include <stdarg.h>
#include <stddef.h>
#include "common/arena.h"

#if defined(__GNUC__) || defined(__clang__)
#define PRINTF_LIKE(fmt_index, args_index) __attribute__((format(printf, fmt_index, args_index)))
#else
#define PRINTF_LIKE(fmt_index, args_index)
#endif

// Growable string allocated from an arena. `data` is always NUL-terminated,
// so it can be used as a C string at any point.
typedef struct {
    Arena *arena;
    char *data;
    size_t len;
    size_t cap;
} StrBuf;

void sb_init(StrBuf *sb, Arena *arena);
void sb_append(StrBuf *sb, const char *s);
void sb_append_n(StrBuf *sb, const char *s, size_t n);
void sb_append_char(StrBuf *sb, char c);
void sb_append_repeat(StrBuf *sb, char c, size_t count);
void sb_appendf(StrBuf *sb, const char *fmt, ...) PRINTF_LIKE(2, 3);
void sb_vappendf(StrBuf *sb, const char *fmt, va_list args);

// Dynamic arrays allocated from an arena. Declare with Vec(T); a
// zero-initialized Vec is empty and ready to use.
//
//     Vec(int) numbers = {0};
//     vec_push(arena, &numbers, 42);
//
// The vector argument is evaluated more than once.
#define Vec(T) struct { T *items; size_t len; size_t cap; }

#define vec_push(arena, v, x)                                                        \
    ((v)->items = vec_grow_((arena), (v)->items, &(v)->cap, (v)->len + 1,           \
                            sizeof(*(v)->items)),                                    \
     (v)->items[(v)->len++] = (x))

#define vec_last(v) ((v)->items[(v)->len - 1])
#define vec_pop(v) ((v)->items[--(v)->len])

// Returns storage for at least `need` elements, copying the old items if it
// had to grow. Used by vec_push; not meant to be called directly.
void *vec_grow_(Arena *arena, void *items, size_t *cap, size_t need, size_t elem_size);

// Levenshtein distance between two strings: the number of single-byte
// insertions, deletions, and substitutions to turn one into the other.
// Case-sensitive.
size_t edit_distance(Arena *arena, const char *a, const char *b);

#endif
