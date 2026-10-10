#ifndef ARENA_H
#define ARENA_H

#include <stddef.h>

typedef struct Arena Arena;

Arena *arena_new(void);
void *arena_alloc(Arena *arena, size_t size);
char *arena_strdup(Arena *arena, const char *s);
char *arena_sprintf(Arena *arena, const char *fmt, ...);
void arena_free(Arena *arena);

#endif
