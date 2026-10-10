#ifndef BACK_SOURCE_TABLE_H
#define BACK_SOURCE_TABLE_H

#include <stddef.h>
#include "common/arena.h"
#include "common/util.h"

// The program's source lines as a C table, for runtime errors to show the
// line that failed (like compile errors do):
//   static const char *const es_source[] = {"let x be 1", ...};
//   static const int es_source_line_count = 1;
// This is the only part of the back end that reads source text, and it only
// copies it; codegen gets the result as text to put into the program.
void source_table_c(Arena *arena, const char *source, size_t len, StrBuf *out);

#endif
