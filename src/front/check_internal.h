#ifndef FRONT_CHECK_INTERNAL_H
#define FRONT_CHECK_INTERNAL_H

// Shared by the checker's source files (check.c, check_names.c, check_expr.c, check_types.c,
// check_const.c, check_list.c, check_text.c, check_record.c, check_map.c).
// Not for use outside src/front/.

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
// "item 1 of xs" can have a kind before anything decides what xs holds. A
// map's is "a map from" its key's variable "to" its value's.
typedef struct {
    TypeVar parent;
    Type type;     // of the root: TYPE_UNKNOWN until decided, a kind, TYPE_LIST (a list of `item`),
                   // or TYPE_MAP (a map from `key` to `item`)
    size_t line;   // where it was decided
    TypeVar item;  // the kind of the items when type is TYPE_LIST, of the values when it's TYPE_MAP
    TypeVar key;   // the kind of the keys when type is TYPE_MAP
} TypeSlot;

typedef enum {
    DEFER_AND,        // logical (yes/no) or joining (text)
    DEFER_ORDER,      // numbers or text
    DEFER_TO_NUMBER,  // "as a number": a number or text
    DEFER_ONE_OF,     // `left` has one of the kinds in `allowed` (e.g. text or a list for "length of")
    DEFER_JOIN,       // "followed by": two lists of one kind join into a list; anything else into text
    DEFER_SEARCH,     // "contains", "the position of": `left` is text (and `right` text) or a list (of `right`)
    DEFER_FIELD,      // "R's NAME" when several kinds of record have NAME: `left` is R, `result` the field
    DEFER_PRESENT,    // a record or nothing (`left`) checked not to be nothing: `right` is the record
    DEFER_METHOD,     // a call of a method several kinds of record have: `left` is the record, `args`
                      // every input, `result` what it gives back, `allowed` 1 when used as a value
} DeferKind;

// Kinds for DEFER_ONE_OF.
enum { KIND_NUMBER = 1, KIND_TEXT = 2, KIND_YESNO = 4, KIND_LIST = 8, KIND_RECORD = 16, KIND_MAP = 32 };

typedef struct {
    DeferKind kind;
    const Expr *expr;
    TypeVar left, right, result;  // right/result unused for DEFER_TO_NUMBER and DEFER_ORDER
    bool done;
    unsigned allowed;     // DEFER_ONE_OF: KIND_ bits
    const char *message;  // DEFER_ONE_OF: "<message>, but this is <the kind of right>."
    const TypeVar *args;  // DEFER_METHOD
} Deferred;

typedef struct {
    Expr *expr;
    TypeVar var;
} TypedExpr;

// A kind in the program to fill in at the end (a variable's, for a sentence that sets it).
typedef struct {
    Type *slot;
    TypeVar var;
} TypedSlot;

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

// A value checked not to be nothing ("if the node's next is not nothing:"),
// by its path: a name, or a name and fields ("node's next"). Until the block
// ends (or something sets it), it's used as the record it holds.
typedef struct {
    const char *path;
    bool killed;  // set since it was checked
} Narrowing;

typedef struct {
    Arena *arena;
    Diag *diag;
    Records *records;                 // the program's kinds of record and of map (new kinds of map are added)
    Vec(Narrowing) present;           // values checked not to be nothing, innermost last
    const Expr *unnarrowed;           // checked as written even if it was checked ("x is nothing", "set x's f to")
    bool explained_default;           // a field's starting value used a name, and that was explained
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
    Vec(TypedSlot) slots;  // other kinds to fill in at the end
} Checker;

// check_types.c
TypeVar types_new(Checker *c, Type type, size_t line);  // a list type gets a variable for its items
TypeVar types_new_list(Checker *c, TypeVar item, size_t line);
TypeVar types_new_map(Checker *c, TypeVar key, TypeVar value, size_t line);
TypeVar types_item(Checker *c, TypeVar list);  // a list's item variable, or a map's value variable
TypeVar types_key(Checker *c, TypeVar map);    // a map's key variable (it must be a map)
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

// check_names.c (and check.c)
const char *checker_type_name(Checker *c, Type type);  // "a number", "a dog or nothing", ...
const char *checker_kind(Checker *c, TypeVar var);  // "a number", "a list of text", ...
const Symbol *checker_find_made(const Checker *c, const char *name);
// A name the main program makes anywhere (with the line it's first made on).
const Symbol *checker_find_later(const Checker *c, const char *name);
const Function *checker_find_function(const Checker *c, const char *name);
// Whether several methods (for different kinds of record) share the name.
bool checker_shared_method(const Checker *c, const char *name);
bool checker_settle_method(Checker *c, const Deferred *d);
void checker_report_unsettled_method(Checker *c, const Deferred *d);
const Constant *checker_find_constant(const Checker *c, const char *name);
// "I don't know anything called …" with the most helpful note.
void checker_report_unknown(Checker *c, const char *name, const char *display, SourcePos pos);
// A variable, input, or loop number named like a function or constant: reports and returns true.
bool checker_clashes(Checker *c, const Name *name);
// Makes a variable (unless the name is taken or clashes, which is reported).
void checker_make(Checker *c, const Name *name, TypeVar var);
// "to area with width and height"
const char *checker_function_header(Checker *c, const Function *f);

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
// What's looked for in a list ("contains", "the position of") has to be the kind it holds.
void checker_require_item(Checker *c, const Expr *list, TypeVar list_var, const Expr *value, TypeVar value_var);
void checker_check_list_stmt(Checker *c, const Stmt *stmt);
// A fixed list or map (a constant's name) can't change: reports and returns true.
bool checker_reject_fixed(Checker *c, const Expr *list);
// "add E to X" where X is a list: adds to its end when that's the only meaning.
void checker_add_to_list(Checker *c, Stmt *stmt, TypeVar list, TypeVar amount);

// check_text.c
// "character 2 of T", "the first 3 characters of T", ...: an EXPR_ITEM or EXPR_SLICE marked `characters`.
TypeVar checker_check_characters(Checker *c, const Expr *expr);
TypeVar checker_check_text_op(Checker *c, const Expr *expr);  // EXPR_TEXT_OP
// "L contains X" (text or a list): waits until the kind of L is known.
TypeVar checker_check_contains(Checker *c, const Expr *expr, TypeVar left, TypeVar right);
TypeVar checker_check_starts(Checker *c, const Expr *expr, TypeVar left, TypeVar right);  // starts/ends with
// Settles a DEFER_SEARCH once the kinds are known enough; true when settled.
bool checker_settle_search(Checker *c, const Deferred *d);

// check_record.c
void checker_collect_records(Checker *c, const Block *program);  // reports kinds defined twice
void checker_check_record(Checker *c, Stmt *stmt);               // STMT_RECORD: its fields and starting values
TypeVar checker_check_field(Checker *c, const Expr *expr);       // EXPR_FIELD
TypeVar checker_check_new(Checker *c, const Expr *expr);         // EXPR_NEW
// A value that may be "nothing" (stored, given to a function, given back):
// "nothing" gets the kind nothing (TYPE_NOTHING), which only fits a record
// or nothing; anything else is checked as usual.
TypeVar checker_check_value(Checker *c, const Expr *expr);
// Whether a value of kind `value` can go where `target` is kept: the same
// kind, or a record (or nothing) where a record or nothing is kept.
bool checker_fits(Checker *c, TypeVar target, TypeVar value, size_t line);
// "X is nothing" / "X is not nothing": checks it and sets *result; false if the comparison isn't one.
bool checker_check_nothing_test(Checker *c, const Expr *expr, TypeVar *result);
// A name or field read where it was checked not to be nothing: its record.
TypeVar checker_narrow(Checker *c, const Expr *expr, TypeVar var);
// What a condition being `when` (true or false) says isn't nothing, added to c->present.
void checker_narrow_condition(Checker *c, const Expr *condition, bool when);
// Forgets that a name or field (and everything under it) was checked: it's being set.
void checker_kill_path(Checker *c, const Expr *target, const char *name);
// Forgets every check of something a block sets (before a loop body that sets it).
void checker_kill_set_in(Checker *c, const Block *block);
// "rex", "the order's customer"; NULL for any other value.
const char *checker_path(Checker *c, const Expr *expr);
// "\"node's next\" could be nothing, so check it first." for a record-or-nothing used as a record.
void checker_report_maybe(Checker *c, const Expr *at, TypeVar var);
bool checker_settle_field(Checker *c, const Deferred *d);
bool checker_settle_present(Checker *c, const Deferred *d);
// A kind of record that has a field called `name`, for suggestions (NULL if none).
const RecordKind *checker_field_owner(const Checker *c, const char *name);

// check_map.c
// The kinds of a map's keys and values; if `var` isn't a map, reports
// "<message>, but this is <kind>." (and `list_note`, if it's a list).
bool checker_require_map(Checker *c, TypeVar var, const Expr *at, const char *message, const char *list_note,
                         TypeVar *key, TypeVar *value);
TypeVar checker_check_map_expr(Checker *c, const Expr *expr);  // EXPR_MAP, EXPR_EMPTY_MAP, EXPR_MAP_GET
TypeVar checker_check_has(Checker *c, const Expr *expr, TypeVar left, TypeVar right);  // M has K
// "the number of entries in M", "the keys of M", "the values of M" (M checked as var).
TypeVar checker_check_map_op(Checker *c, const Expr *expr, TypeVar var);
// What a "for each" loop goes through: a list's items, or a map's keys
// (with their values in *value, for "for each K and V in M").
TypeVar checker_check_each(Checker *c, const Stmt *stmt, TypeVar *value);
void checker_check_remove_key(Checker *c, const Stmt *stmt);  // remove K from M
// "set the value for K in M to V" (V already checked as value_var).
void checker_check_map_set(Checker *c, const Expr *target, const Expr *value_expr, TypeVar value_var);
// "add V to the value for K in M" and the other changes: the kind of the value that changes.
TypeVar checker_check_map_change(Checker *c, const Expr *target);

// check_const.c
// The kind of a worked-out constant: a number, text, yes/no, a list or a map.
Type checker_const_type(Checker *c, const ConstValue *v);
void checker_collect_constants(Checker *c, const Block *program);
void checker_check_constant(Checker *c, const Stmt *stmt);
const Constant *checker_closest_constant(const Checker *c, const char *name);

#endif
