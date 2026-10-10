#ifndef FRONT_CHECK_INTERNAL_H
#define FRONT_CHECK_INTERNAL_H

// Shared by the checker's source files (check.c, check_expr.c, check_types.c,
// check_const.c). Not for use outside src/front/.

#include <stdbool.h>
#include "common/ast.h"
#include "common/diag.h"

// --- Types (check_types.c) ------------------------------------------------------
//
// Every value gets a type variable. Variables that must have the same kind are
// joined (union-find), and a variable gets its kind as soon as anything
// decides it. A function's inputs and result are variables too, so their
// kinds come from how they're used and what they're given. Operators whose
// meaning depends on the kinds ("and", ordering comparisons, "as a number")
// wait in `deferred` until their sides are known.

typedef size_t TypeVar;

typedef struct {
    TypeVar parent;
    Type type;    // of the root; TYPE_UNKNOWN until decided
    size_t line;  // where it was decided
} TypeSlot;

typedef enum {
    DEFER_AND,        // logical (yes/no) or joining (text)
    DEFER_ORDER,      // numbers or text
    DEFER_TO_NUMBER,  // "as a number": a number or text
} DeferKind;

typedef struct {
    DeferKind kind;
    const Expr *expr;
    TypeVar left, right, result;  // right/result unused for DEFER_TO_NUMBER and DEFER_ORDER
    bool done;
} Deferred;

typedef struct {
    Expr *expr;
    TypeVar var;
} TypedExpr;

typedef struct {
    const char *name;
    size_t line;        // where it was made
    const char *where;  // in `ended`: "the \"if\"" or "the loop" that held it
    bool loop_number;   // a loop's own number ("count ... as n")
    TypeVar var;        // its kind
} Symbol;

typedef struct {
    bool has_it;  // count and times loops have a number that "it" refers to
} Loop;

typedef struct {
    const char *name;
    Stmt *definition;      // STMT_FUNCTION: params and line; the checker stores the kinds in it
    Vec(TypeVar) params;   // the kind of each input
    TypeVar result;        // what it gives back (decided as nothing if it never gives back a value)
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
    const Function *function;         // the function being checked, if any
    size_t floor;                     // made[floor..] is visible (a function sees only its own names)
    size_t blocks;                    // how many if/loop blocks we're inside
    Vec(Symbol) made;    // names that exist here: made so far, in blocks still open
    Vec(Symbol) later;   // every name the program makes anywhere, for "you make it later"
    Vec(Symbol) ended;   // names whose block has ended; line = the if or loop that held them
    Vec(Loop) loops;     // loops the current statement is inside, innermost last
    Vec(TypeSlot) types;
    Vec(Deferred) deferred;
    Vec(TypedExpr) typed;  // every expression checked, to receive its kind at the end
} Checker;

// check_types.c
TypeVar types_new(Checker *c, Type type, size_t line);
Type types_of(Checker *c, TypeVar var);  // TYPE_UNKNOWN if not decided
// Joins two variables. False (and nothing changes) if they have different kinds.
bool types_unify(Checker *c, TypeVar a, TypeVar b);
// Gives var the kind if it has none. False if it already has a different one.
bool types_require(Checker *c, TypeVar var, Type type, size_t line);
size_t types_line(Checker *c, TypeVar var);  // where its kind was decided
void types_defer(Checker *c, DeferKind kind, const Expr *expr, TypeVar left, TypeVar right, TypeVar result);
// Settles the deferred operators, reports kinds nothing decided (if no error
// was reported since first_error), and writes every kind into the program.
void types_finish(Checker *c, size_t first_error);

// check.c
const Symbol *checker_find_made(const Checker *c, const char *name);
// A name the main program makes anywhere (with the line it's first made on).
const Symbol *checker_find_later(const Checker *c, const char *name);
const Function *checker_find_function(const Checker *c, const char *name);
const Constant *checker_find_constant(const Checker *c, const char *name);
// "I don't know anything called …" with the most helpful note.
void checker_report_unknown(Checker *c, const char *name, SourcePos pos);
// A variable, input, or loop number named like a function or constant: reports and returns true.
bool checker_clashes(Checker *c, const Name *name);
// "to area with width and height"
const char *checker_function_header(const Checker *c, const Function *f);

// check_expr.c
// Checks an expression and returns its kind. as_value: a function call must
// give back a value (false only for a call written as a sentence).
TypeVar checker_check_expr(Checker *c, const Expr *expr);
TypeVar checker_check_call(Checker *c, const Expr *expr, bool as_value);
// Requires a kind; otherwise reports "<message>, but this is <kind>." with the hint.
void checker_require(Checker *c, TypeVar var, Type type, const Expr *at, const char *message, const char *hint);
// The type-checked arithmetic rule: both sides numbers, with the runtime's wording.
void checker_arithmetic(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Span span, size_t line);

// check_const.c
void checker_collect_constants(Checker *c, const Block *program);
void checker_check_constant(Checker *c, const Stmt *stmt);
const Constant *checker_closest_constant(const Checker *c, const char *name);

#endif
