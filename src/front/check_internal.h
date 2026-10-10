#ifndef FRONT_CHECK_INTERNAL_H
#define FRONT_CHECK_INTERNAL_H

// Shared by the checker's source files (check.c, check_expr.c, check_types.c,
// check_const.c, check_list.c). Not for use outside src/front/.

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

// A list's type is a variable too: "a list of" its item's variable, so
// "item 1 of xs" can have a kind before anything decides what xs holds.
typedef struct {
    TypeVar parent;
    Type type;     // of the root: TYPE_UNKNOWN until decided, a kind, or TYPE_LIST (a list of `item`)
    size_t line;   // where it was decided
    TypeVar item;  // the kind of the items when type is TYPE_LIST
} TypeSlot;

typedef enum {
    DEFER_AND,        // logical (yes/no) or joining (text)
    DEFER_ORDER,      // numbers or text
    DEFER_TO_NUMBER,  // "as a number": a number or text
    DEFER_ONE_OF,     // `left` has one of the kinds in `allowed` (e.g. text or a list for "length of")
    DEFER_JOIN,       // "followed by": two lists of one kind join into a list; anything else into text
} DeferKind;

// Kinds for DEFER_ONE_OF.
enum { KIND_NUMBER = 1, KIND_TEXT = 2, KIND_YESNO = 4, KIND_LIST = 8 };

typedef struct {
    DeferKind kind;
    const Expr *expr;
    TypeVar left, right, result;  // right/result unused for DEFER_TO_NUMBER and DEFER_ORDER
    bool done;
    unsigned allowed;     // DEFER_ONE_OF: KIND_ bits
    const char *message;  // DEFER_ONE_OF: "<message>, but this is <the kind of right>."
} Deferred;

typedef struct {
    Expr *expr;
    TypeVar var;
} TypedExpr;

typedef struct {
    const char *name;     // as matched (lowercased)
    const char *display;  // as first written, for messages
    size_t line;        // where it was made
    const char *where;  // in `ended`: "the \"if\"" or "the loop" that held it
    bool loop_number;   // a loop's own number ("count ... as n") or item ("for each x in")
    TypeVar var;        // its kind
    bool loop_item;     // a "for each" loop's item
} Symbol;

typedef struct {
    bool has_it;  // count, times and for-each loops (and "every"/"only") have something "it" refers to
    TypeVar it;   // its kind: a number, or the item's kind
} Loop;

typedef struct {
    const char *name;
    const char *display;   // as written in the definition
    Stmt *definition;      // STMT_FUNCTION: params and line; the checker stores the kinds in it
    Vec(TypeVar) params;   // the kind of each input
    TypeVar result;        // what it gives back (decided as nothing if it never gives back a value)
} Function;

typedef struct {
    const char *name;
    const char *display;  // as written where it's kept
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
TypeVar types_new(Checker *c, Type type, size_t line);  // a list type gets a variable for its items
TypeVar types_new_list(Checker *c, TypeVar item, size_t line);
TypeVar types_item(Checker *c, TypeVar list);  // a list's item variable (it must be a list)
Type types_of(Checker *c, TypeVar var);  // TYPE_UNKNOWN if not decided
// Joins two variables. False (and nothing changes) if they have different kinds.
bool types_unify(Checker *c, TypeVar a, TypeVar b);
// Gives var the kind if it has none. False if it already has a different one.
bool types_require(Checker *c, TypeVar var, Type type, size_t line);
size_t types_line(Checker *c, TypeVar var);  // where its kind was decided
void types_defer(Checker *c, DeferKind kind, const Expr *expr, TypeVar left, TypeVar right, TypeVar result);
// `var` must turn out to have one of the `allowed` kinds; otherwise reports
// "<message>, but this is <the kind of shown>." at expr.
void types_defer_one_of(Checker *c, const Expr *expr, TypeVar var, TypeVar shown, unsigned allowed,
                        const char *message);
// Settles the deferred operators, reports kinds nothing decided (if no error
// was reported since first_error), and writes every kind into the program.
void types_finish(Checker *c, size_t first_error);

// check.c
const char *checker_kind(Checker *c, TypeVar var);  // "a number", "a list of text", ...
const Symbol *checker_find_made(const Checker *c, const char *name);
// A name the main program makes anywhere (with the line it's first made on).
const Symbol *checker_find_later(const Checker *c, const char *name);
const Function *checker_find_function(const Checker *c, const char *name);
const Constant *checker_find_constant(const Checker *c, const char *name);
// "I don't know anything called …" with the most helpful note.
void checker_report_unknown(Checker *c, const char *name, const char *display, SourcePos pos);
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

// The name each item of a list gets ("for each score in", "every score in").
void checker_make_item_name(Checker *c, const Name *name, TypeVar item);

// check_list.c
// The kind of a list's items; if `var` isn't a list, reports "<message>, but this is <kind>."
TypeVar checker_require_list(Checker *c, TypeVar var, const Expr *at, const char *message);
TypeVar checker_check_list_expr(Checker *c, const Expr *expr);  // EXPR_LIST ... EXPR_ONLY
TypeVar checker_check_contains(Checker *c, const Expr *expr, TypeVar left, TypeVar right);
void checker_check_list_stmt(Checker *c, const Stmt *stmt);
// "add E to X" where X is a list: adds to its end when that's the only meaning.
void checker_add_to_list(Checker *c, Stmt *stmt, const Symbol *symbol, TypeVar amount);

// check_const.c
void checker_collect_constants(Checker *c, const Block *program);
void checker_check_constant(Checker *c, const Stmt *stmt);
const Constant *checker_closest_constant(const Checker *c, const char *name);

#endif
