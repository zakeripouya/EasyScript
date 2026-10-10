#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common/arena.h"

// Default size of a new block. Larger requests get a block of their own size,
// so this is a growth step, not a limit.
#define ARENA_BLOCK_SIZE (64 * 1024)

typedef struct ArenaBlock {
    struct ArenaBlock *next;
    size_t cap;
    size_t used;
    max_align_t data[];
} ArenaBlock;

struct Arena {
    ArenaBlock *head;
};

static void out_of_memory(void) {
    fprintf(stderr, "EasyScript ran out of memory.\n");
    exit(1);
}

static ArenaBlock *new_block(size_t cap) {
    if (cap > SIZE_MAX - sizeof(ArenaBlock)) out_of_memory();
    ArenaBlock *block = malloc(sizeof(ArenaBlock) + cap);
    if (!block) out_of_memory();
    block->next = NULL;
    block->cap = cap;
    block->used = 0;
    return block;
}

Arena *arena_new(void) {
    Arena *arena = malloc(sizeof(Arena));
    if (!arena) out_of_memory();
    arena->head = new_block(ARENA_BLOCK_SIZE);
    return arena;
}

// Returns zeroed memory aligned for any type.
void *arena_alloc(Arena *arena, size_t size) {
    const size_t align = _Alignof(max_align_t);
    if (size > SIZE_MAX - align) out_of_memory();
    size = (size + align - 1) & ~(align - 1);

    ArenaBlock *block = arena->head;
    if (block->cap - block->used < size) {
        block = new_block(size > ARENA_BLOCK_SIZE ? size : ARENA_BLOCK_SIZE);
        block->next = arena->head;
        arena->head = block;
    }
    void *p = (char *)block->data + block->used;
    block->used += size;
    memset(p, 0, size);
    return p;
}

char *arena_strdup(Arena *arena, const char *s) {
    size_t len = strlen(s);
    char *copy = arena_alloc(arena, len + 1);
    memcpy(copy, s, len + 1);
    return copy;
}

char *arena_vsprintf(Arena *arena, const char *fmt, va_list args) {
    va_list copy;
    va_copy(copy, args);
    int len = vsnprintf(NULL, 0, fmt, copy);
    va_end(copy);
    if (len < 0) out_of_memory();

    char *s = arena_alloc(arena, (size_t)len + 1);
    vsnprintf(s, (size_t)len + 1, fmt, args);
    return s;
}

char *arena_sprintf(Arena *arena, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char *s = arena_vsprintf(arena, fmt, args);
    va_end(args);
    return s;
}

void arena_free(Arena *arena) {
    ArenaBlock *block = arena->head;
    while (block) {
        ArenaBlock *next = block->next;
        free(block);
        block = next;
    }
    free(arena);
}
