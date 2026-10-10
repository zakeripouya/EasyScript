#ifndef FRONT_CHECK_H
#define FRONT_CHECK_H

#include "common/ast.h"
#include "common/diag.h"

// Checks names in a parsed program and reports problems to `diag`:
// - a name used before it's made, or never made ("Did you mean ...?")
// - a name made twice with "let"
// - features that don't exist yet: calling a function, and "it"
//
// "let", "ask ... and call the answer X", and "read file ... and call it X"
// make a name; "set", "change", and the arithmetic statements need one that
// exists. Only run it on a program that parsed without errors.
void check_program(Arena *arena, Diag *diag, const Block *program);

#endif
