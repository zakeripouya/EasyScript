#ifndef BACK_CODEGEN_C_H
#define BACK_CODEGEN_C_H

#include "common/ast.h"
#include "common/util.h"

// Writes a complete, self-contained C program for `program` to `out`: the
// embedded runtime (runtime/es_runtime.h), one global per top-level
// variable, and main(). `program` must have passed check_program() with no
// errors; codegen reports nothing itself.
// `source_table` (source_table.h) goes into the program as it is, so runtime
// errors can show the line that failed; codegen doesn't read it.
// When `plan` isn't NULL, a line for each high-level form (a question about a
// list, a filter, ...) saying what it became is written to it ("easyscript plan").
void codegen_c(Arena *arena, const Program *program, const char *source_table, StrBuf *out, StrBuf *plan);

#endif
