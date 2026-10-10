#ifndef ARENA_H
#define ARENA_H

#include <stdarg.h>
#include <stddef.h>

typedef struct Arena Arena;

Arena *arena_new(void);
void *arena_alloc(Arena *arena, size_t size);
char *arena_strdup(Arena *arena, const char *s);
char *arena_sprintf(Arena *arena, const char *fmt, ...);
char *arena_vsprintf(Arena *arena, const char *fmt, va_list args);
void arena_free(Arena *arena);

#endif
