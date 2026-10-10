#ifndef FRONT_CHECK_H
#define FRONT_CHECK_H

#include "common/ast.h"
#include "common/diag.h"

// Checks names in a parsed program and reports problems to `diag`:
// - a name used before it's made, or never made ("Did you mean ...?")
// - a name made twice with "let"
// - "it" outside a counting or "repeat ... times" loop
// - "stop the loop" / "skip this one" outside a loop
// - a loop's number using a name that already exists
// - calling a function (not available yet)
//
// "let", "ask ... and call the answer X", and "read file ... and call it X"
// make a name; "set", "change", and the arithmetic statements need one that
// exists. A name made inside a block (an if branch or a loop body), and a
// loop's number, exist only until that block ends. Only run it on a program that parsed without errors.
void check_program(Arena *arena, Diag *diag, const Block *program);

#endif
