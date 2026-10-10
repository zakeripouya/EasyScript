#ifndef FRONT_PARSE_H
#define FRONT_PARSE_H

#include "common/ast.h"
#include "common/diag.h"
#include "front/lexer.h"

// Parses a whole program: a list of statements, each ended by a period or the
// end of its line. Errors go to `diag`; parsing resumes at the next sentence,
// so one run reports every error. Statements with errors are left out of the
// returned block. Check diag_count() afterwards.
Block *parse_program(Arena *arena, Diag *diag, const char *source, const TokenList *tokens);

#endif
