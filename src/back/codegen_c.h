#ifndef BACK_CODEGEN_C_H
#define BACK_CODEGEN_C_H

#include "common/ast.h"
#include "common/util.h"

// Writes a complete, self-contained C program for `program` to `out`: the
// embedded runtime (runtime/es_runtime.h), one global per top-level
// variable, and main(). `program` must have passed check_program() with no
// errors; codegen reports nothing itself.
void codegen_c(Arena *arena, const Block *program, StrBuf *out);

#endif
