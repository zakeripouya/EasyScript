#ifndef LEGACY_H
#define LEGACY_H

#include <stdio.h>

// The old-syntax pipeline (src/lexer.c, src/parser.c, src/codegen.c), kept
// until the new front end replaces it. Writes the statements of main()'s body
// for `source` to `out`. Errors print one line and exit(1).
void legacy_compile(char *source, FILE *out);

#endif
