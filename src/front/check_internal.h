#ifndef FRONT_CHECK_INTERNAL_H
#define FRONT_CHECK_INTERNAL_H

// Shared by the checker's source files (check.c, check_const.c). Not for use
// outside src/front/.

#include <stdbool.h>
#include "common/ast.h"
#include "common/diag.h"

typedef struct {
    const char *name;
    size_t line;        // where it was made
    const char *where;  // in `ended`: "the \"if\"" or "the loop" that held it
    bool loop_number;   // a loop's own number ("count ... as n")
} Symbol;

typedef struct {
    bool has_it;  // count and times loops have a number that "it" refers to
} Loop;

typedef struct {
    const char *name;
    const Stmt *definition;  // STMT_FUNCTION: params and line
} Function;

typedef struct {
    const char *name;
    Stmt *definition;  // STMT_CONSTANT; the checker stores the worked-out value in it
    size_t index;      // order in the program: a constant may only use earlier ones
    bool ok;           // its value was worked out
} Constant;

typedef struct {
    Arena *arena;
    Diag *diag;
    Vec(Function) functions;          // every top-level function (pre-scanned)
    Vec(Constant) constants;          // every top-level constant (pre-scanned)
    const Constant *constant;         // the constant whose value is being worked out, if any
    const Stmt *function;             // the function being checked, if any
    size_t floor;                     // made[floor..] is visible (a function sees only its own names)
    size_t blocks;                    // how many if/loop blocks we're inside
    Vec(Symbol) made;    // names that exist here: made so far, in blocks still open
    Vec(Symbol) later;   // every name the program makes anywhere, for "you make it later"
    Vec(Symbol) ended;   // names whose block has ended; line = the if or loop that held them
    Vec(Loop) loops;     // loops the current statement is inside, innermost last
} Checker;

// check.c
const Symbol *checker_find_made(const Checker *c, const char *name);
const Function *checker_find_function(const Checker *c, const char *name);
const Constant *checker_find_constant(const Checker *c, const char *name);
// "I don't know anything called …" with the most helpful note.
void checker_report_unknown(Checker *c, const char *name, SourcePos pos);
// A variable, input, or loop number named like a function or constant: reports and returns true.
bool checker_clashes(Checker *c, const Name *name);
void checker_check_expr(Checker *c, const Expr *expr);

// check_const.c
void checker_collect_constants(Checker *c, const Block *program);
void checker_check_constant(Checker *c, const Stmt *stmt);
const Constant *checker_closest_constant(const Checker *c, const char *name);

#endif
