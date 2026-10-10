#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/util.h"

#define SB_INITIAL_CAP 16

static void overflow(void) {
    fprintf(stderr, "Error: Out of memory\n");
    exit(1);
}

// Returns a capacity of at least `need`, doubling from `cap`.
static size_t grow_cap(size_t cap, size_t need) {
    size_t new_cap = cap ? cap : 1;
    while (new_cap < need) {
        if (new_cap > SIZE_MAX / 2) return need;
        new_cap *= 2;
    }
    return new_cap;
}

void *vec_grow_(Arena *arena, void *items, size_t *cap, size_t need, size_t elem_size) {
    if (need <= *cap) return items;
    size_t new_cap = grow_cap(*cap, need);
    if (new_cap > SIZE_MAX / elem_size) overflow();
    void *grown = arena_alloc(arena, new_cap * elem_size);
    if (*cap) memcpy(grown, items, *cap * elem_size);
    *cap = new_cap;
    return grown;
}

void sb_init(StrBuf *sb, Arena *arena) {
    sb->arena = arena;
    sb->len = 0;
    sb->cap = SB_INITIAL_CAP;
    sb->data = arena_alloc(arena, sb->cap);
}

// Makes room for `extra` more bytes plus the terminating NUL.
static void sb_reserve(StrBuf *sb, size_t extra) {
    if (extra > SIZE_MAX - sb->len - 1) overflow();
    size_t need = sb->len + extra + 1;
    if (need <= sb->cap) return;
    size_t new_cap = grow_cap(sb->cap, need);
    char *grown = arena_alloc(sb->arena, new_cap);
    memcpy(grown, sb->data, sb->len + 1);
    sb->data = grown;
    sb->cap = new_cap;
}

void sb_append_n(StrBuf *sb, const char *s, size_t n) {
    sb_reserve(sb, n);
    memcpy(sb->data + sb->len, s, n);
    sb->len += n;
    sb->data[sb->len] = '\0';
}

void sb_append(StrBuf *sb, const char *s) {
    sb_append_n(sb, s, strlen(s));
}

void sb_append_char(StrBuf *sb, char c) {
    sb_append_n(sb, &c, 1);
}

void sb_append_repeat(StrBuf *sb, char c, size_t count) {
    sb_reserve(sb, count);
    memset(sb->data + sb->len, c, count);
    sb->len += count;
    sb->data[sb->len] = '\0';
}

void sb_vappendf(StrBuf *sb, const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int n = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (n < 0) overflow();

    sb_reserve(sb, (size_t)n);
    vsnprintf(sb->data + sb->len, (size_t)n + 1, fmt, args);
    sb->len += (size_t)n;
}

void sb_appendf(StrBuf *sb, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    sb_vappendf(sb, fmt, args);
    va_end(args);
}

void sb_append_quoted(StrBuf *sb, const char *s, size_t n) {
    sb_append_char(sb, '"');
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\n') sb_append(sb, "\\n");
        else if (c == '\t') sb_append(sb, "\\t");
        else if (c == '"') sb_append(sb, "\\\"");
        else if (c == '\\') sb_append(sb, "\\\\");
        else if (c < 0x20 || c == 0x7F) sb_appendf(sb, "\\x%02X", c);
        else sb_append_char(sb, (char)c);
    }
    sb_append_char(sb, '"');
}

static size_t min3(size_t a, size_t b, size_t c) {
    size_t m = a < b ? a : b;
    return m < c ? m : c;
}

size_t edit_distance(Arena *arena, const char *a, const char *b) {
    size_t len_a = strlen(a);
    size_t len_b = strlen(b);
    if (len_b >= SIZE_MAX / sizeof(size_t) - 1) overflow();

    // Three rows of the dynamic-programming table: rows i-2, i-1 and i.
    size_t row_bytes = (len_b + 1) * sizeof(size_t);
    size_t *before = arena_alloc(arena, row_bytes);
    size_t *prev = arena_alloc(arena, row_bytes);
    size_t *cur = arena_alloc(arena, row_bytes);
    for (size_t j = 0; j <= len_b; j++) {
        prev[j] = j;
    }

    for (size_t i = 1; i <= len_a; i++) {
        cur[0] = i;
        for (size_t j = 1; j <= len_b; j++) {
            size_t cost = a[i - 1] == b[j - 1] ? 0 : 1;
            cur[j] = min3(prev[j] + 1, cur[j - 1] + 1, prev[j - 1] + cost);
            bool swapped = i > 1 && j > 1 && a[i - 1] == b[j - 2] && a[i - 2] == b[j - 1];
            if (swapped && before[j - 2] + 1 < cur[j]) cur[j] = before[j - 2] + 1;
        }
        size_t *recycled = before;
        before = prev;
        prev = cur;
        cur = recycled;
    }
    return prev[len_b];
}
