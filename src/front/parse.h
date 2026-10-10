#ifndef FRONT_PARSE_H
#define FRONT_PARSE_H

#include "common/ast.h"
#include "common/diag.h"
#include "front/lexer.h"

// Parses a whole program. Errors go to `diag`; parsing resumes at the next
// sentence, so one run reports every error. Check diag_count() afterwards.
//
// Until statements exist, a program is a list of expressions: one sentence
// per line, or several on a line separated by periods.
Block *parse_program(Arena *arena, Diag *diag, const char *source, const TokenList *tokens);

#endif
