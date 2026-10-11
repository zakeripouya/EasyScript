#ifndef FRONT_CONSTEVAL_H
#define FRONT_CONSTEVAL_H

#include <stdbool.h>
#include "common/ast.h"
#include "common/diag.h"

// Looks up a name inside a constant's value. Returns true and sets *out if
// it's a constant that can be used there; otherwise reports why not and
// returns false.
typedef bool (*ConstLookup)(void *context, const Expr *name, ConstValue *out);

// Works out a constant's value before the program runs: number, text and
// yes/no values, arithmetic, joining, comparisons, and/or/not, conversions,
// fixed lists written out (of numbers, text or yes/no) and their length,
// fixed maps written out and their number of entries, and other constants
// (through `lookup`); `records` names the kinds of record and map. Anything
// that's only known while the program runs (variables, function calls, "it",
// file contents, length of) is an error. Errors use the same wording as the runtime's (e.g. "You
// divided by zero."). Reports at most one error; returns false after it.
bool const_eval(Arena *arena, Diag *diag, const Records *records, const Expr *expr, ConstLookup lookup, void *context,
                ConstValue *out);

// How a number prints: the same rules as the runtime's es_number_text.
const char *const_number_text(Arena *arena, double x, size_t *len);
// A number key as the runtime keeps it (es_key_number): whole numbers below
// 1e15 exactly, others to 12 significant digits. Equal texts, same key.
const char *const_key_text(Arena *arena, double x);

#endif
