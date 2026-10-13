#ifndef ES_NEW_H
#define ES_NEW_H

#include <stdio.h>
#include "common/arena.h"

// "easyscript new NAME [--template T]": a folder NAME holding main.es, made
// from a starter template (templates/*.es, built into the compiler).
// Gives back the exit status.
int new_program(Arena *arena, const char *name, const char *template_name);

// "easyscript new --list": the templates and what each one is for.
void new_list(FILE *out);

#endif
