#ifndef FRONT_CHECK_INTERNAL_H
#define FRONT_CHECK_INTERNAL_H

// Shared by the checker's source files (check.c, check_names.c, check_expr.c, check_types.c, check_narrow.c, check_kind.c,
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
                   // TYPE_MAP (a map from `key` to `item`), or TYPE_FUNC (a function taking
                   // `params` and giving back `item`)
    size_t line;   // where it was decided
    TypeVar item;  // the kind of the items when type is TYPE_LIST, of the values when it's TYPE_MAP,
                   // of the result when it's TYPE_FUNC
    TypeVar key;   // the kind of the keys when type is TYPE_MAP
    const TypeVar *params;  // TYPE_FUNC: the kinds of its inputs
    size_t count;           // TYPE_FUNC: how many inputs it takes
} TypeSlot;

typedef enum {
    DEFER_ORDER,      // numbers or text
    DEFER_TO_NUMBER,  // "as a number": a number or text
    DEFER_ONE_OF,     // `left` has one of the kinds in `allowed` (e.g. text or a list for "length of")
    DEFER_JOIN,       // "followed by": two lists of one kind join into a list; anything else into text
    DEFER_SEARCH,     // "contains", "the position of": `left` is text (and `right` text) or a list (of `right`)
    DEFER_FIELD,      // "R's NAME" when several kinds of record have NAME: `left` is R, `result` the field
    DEFER_PRESENT,    // a record or nothing (`left`) checked not to be nothing: `right` is the record
    DEFER_METHOD,     // a call of a method several kinds of record have: `left` is the record, `args`
                      // every input, `result` what it gives back, `allowed` 1 when used as a value
    DEFER_ARITH,      // arithmetic with a side not decided yet (it could be money): `allowed` is the BinaryOp,
                      // `result` what it gives, reported at `span`
} DeferKind;

// Kinds for DEFER_ONE_OF.
// KIND_OTHER: a function, an interface's value, or anything that can be nothing (but not nothing itself).
enum { KIND_NUMBER = 1, KIND_TEXT = 2, KIND_YESNO = 4, KIND_LIST = 8, KIND_RECORD = 16, KIND_MAP = 32, KIND_OTHER = 64, KIND_DATE = 128, KIND_MONEY = 256,
       KIND_NUMBER_FIRST = 512 };  // with KIND_NUMBER: a number, unless something decides otherwise before the end

typedef struct {
    DeferKind kind;
    const Expr *expr;
    TypeVar left, right, result;  // right/result unused for DEFER_TO_NUMBER and DEFER_ORDER
    bool done;
    unsigned allowed;     // DEFER_ONE_OF: KIND_ bits
    const char *message;  // DEFER_ONE_OF: "<message>, but this is <the kind of right>."
    const TypeVar *args;  // DEFER_METHOD
    Span span;            // DEFER_ARITH: where to report
    size_t line;          // DEFER_ARITH
} Deferred;

typedef struct {
    Expr *expr;
    TypeVar var;
} TypedExpr;

// A plain "ask" and the kind of its answer.
typedef struct {
    Stmt *ask;
    TypeVar var;
} AskAnswer;

// A kind in the program to fill in at the end (a variable's, for a sentence that sets it).
typedef struct {
    Type *slot;
    TypeVar var;
} TypedSlot;

// A field written without a kind ("a dog has a name"): one kind everywhere,
// worked out from how it's used.
typedef struct {
    Field *field;
    TypeVar var;
} FieldKind;

typedef struct {
    const char *name;     // as matched (lowercased)
    const char *display;  // as first written, for messages
    size_t line;        // where it was made
    const char *where;  // in `ended`: "the \"if\"" or "the loop" that held it
    bool loop_number;   // a loop's own number ("count ... as n") or item ("for each x in")
    TypeVar var;        // its kind
    bool loop_item;     // a "for each" loop's item
    const Stmt *owner;  // the function (or function made inside a value) that made it; NULL: the main program
    size_t set_inside;  // a function made inside another sets it: the line that function is made on (else 0)
} Symbol;

// "Not a variant": a narrowing that only says a value isn't nothing.
#define NO_VARIANT ((size_t)-1)

typedef struct {
    bool has_it;  // count, times and for-each loops (and "every"/"only") have something "it" refers to
    TypeVar it;   // its kind: a number, or the item's kind
    bool subject;    // not a loop: a "when" of "check which kind S is", where "it" is S
    size_t variant;  // the "when"'s one variant, or NO_VARIANT
    bool helper;     // "every" or "only": worked out in a C function of its own
} Loop;

typedef struct {
    const char *name;
    const char *display;   // as written in the definition
    unsigned file;         // the file it's defined in
    bool private;          // "privately to ...": only that file can use it
    Stmt *definition;      // STMT_FUNCTION: params and line; the checker stores the kinds in it
    Vec(TypeVar) params;   // the kind of each input
    TypeVar result;        // what it gives back (decided as nothing if it never gives back a value)
} Function;

// A function made inside a value ("a function taking x ...") being checked:
// names made before `scope` (and at or after the floor) are from around it,
// so using one captures it.
typedef struct LambdaScope LambdaScope;
struct LambdaScope {
    Stmt *definition;
    size_t scope;
    LambdaScope *outer;
    bool in_loop;  // made inside a loop that has an "it" (which it can't use: it runs later)
};

typedef struct {
    const char *name;
    const char *display;  // as written where it's kept
    unsigned file;        // the file it's kept in
    bool private;         // "privately keep ...": only that file can use it
    Stmt *definition;  // STMT_CONSTANT; the checker stores the worked-out value in it
    size_t index;      // order in the program: a constant may only use earlier ones
    bool ok;           // its value was worked out
} Constant;

// A value checked not to be nothing ("if the node's next is not nothing:"),
// by its path: a name, or a name and fields ("node's next"). Until the block
// ends (or something sets it), it's used as the record it holds.
typedef struct {
    const char *path;
    bool killed;     // set since it was checked
    size_t variant;  // checked to be this variant ("if s is a circle"), or NO_VARIANT
} Narrowing;

// A chart's values ("save a bar chart of X as F"): which currency, if any,
// is decided once every kind is known.
typedef struct {
    Expr *currency;  // the text handed to lib/charts.es: "", "dollars", "euros" or "pounds"
    TypeVar item;    // the kind of X's items (a list's, or a map's values)
} ChartValues;

typedef struct TableFile TableFile;  // check_table_file.c: a table's file, looked at before the program runs

typedef struct {
    Arena *arena;
    Diag *diag;
    Records *records;                 // the program's kinds of record and of map (new kinds of map are added)
    Vec(Narrowing) present;           // values checked not to be nothing, innermost last
    const Expr *unnarrowed;           // checked as written even if it was checked ("x is nothing", "set x's f to")
    bool explained_default;           // a field's starting value used a name, and that was explained
    bool setting_field;               // checking the field a sentence sets ("set s's radius to")
    Vec(Function) functions;          // every top-level function (pre-scanned)
    Vec(Constant) constants;          // every top-level constant (pre-scanned)
    const Constant *constant;         // the constant whose value is being worked out, if any
    const Function *function;         // the function being checked, if any
    size_t floor;                     // made[floor..] is visible (a function sees only its own names)
    size_t blocks;                    // how many if/loop blocks we're inside
    size_t rules;                     // "must" rules so far (each gets a C function, es_rule1, ...)
    bool in_rule;                     // checking a rule's condition (it sees only constants and "it")
    bool storing_field;               // storing a field's value (new, set): text can become its kind of text
                                      // there, as the field's rules check it
    Vec(Symbol) made;    // names that exist here: made so far, in blocks still open
    Vec(Symbol) later;   // every name the program makes anywhere, for "you make it later"
    Vec(Symbol) ended;   // names whose block has ended; line = the if or loop that held them
    Vec(Loop) loops;     // loops the current statement is inside, innermost last
    Vec(TypeSlot) types;
    Vec(Deferred) deferred;
    Vec(TypedExpr) typed;  // every expression checked, to receive its kind at the end
    Vec(TypedSlot) slots;  // other kinds to fill in at the end
    Vec(FieldKind) field_kinds;  // fields whose kinds are worked out from use
    LambdaScope *lambda;   // the innermost function made inside a value being checked, if any
    Vec(Function *) lambdas;          // every function made inside a value
    Program *program;                 // its cells (names functions made inside the main program use)
    Vec(Type) assumed;                // kinds and interfaces being matched (pairs), assumed to fit meanwhile
    unsigned file;                    // the file of the statement being checked (Program.files)
    unsigned main_file;               // the file that's run: the last one
    unsigned only_file;               // "helpers's double": names are looked up only in this file (1 + its index), or 0
    Vec(struct ModuleUse) module_uses;  // names used from other files, renamed once the program is checked
    Vec(ChartValues) charts;          // every chart sentence, for its currency
    Vec(TableFile *) table_files;     // the files tables are read from, named in quotes
    Vec(AskAnswer) asks;              // plain "ask" sentences: the answer's kind comes from its use (text if nothing decides)
} Checker;

// A name in a value that means another file's function or constant.
struct ModuleUse {
    Expr *expr;
    const Stmt *definition;
};

// What checker_enter_names changed, put back by checker_leave_names.
typedef struct {
    unsigned only_file;
    size_t floor;
} NameContext;

// "ask ... and call the answer X", "read csv file ... and call it X": makes X, or (if it exists) sets it.
void checker_make_or_store(Checker *c, const Name *name, TypeVar value);

// check_library.c: the standard library
TypeVar checker_check_library(Checker *c, const Expr *expr);  // EXPR_LIBRARY
void checker_check_library_stmt(Checker *c, const Stmt *stmt);  // STMT_LIBRARY
void checker_finish_charts(Checker *c);  // check_library.c: each chart's currency, once kinds are known
TypeVar checker_check_system(Checker *c, const Expr *expr);      // check_system.c: LIB_PROGRAM_WORDS and later
void checker_check_system_stmt(Checker *c, const Stmt *stmt);    // DO_COMPLAIN, DO_RUN, DO_WAIT_KEY
void checker_check_stop(Checker *c, const Stmt *stmt);           // STMT_STOP
// pi, today, tomorrow, yesterday where the program hasn't made the name: turns expr into its EXPR_LIBRARY.
bool checker_library_name(Checker *c, const Expr *expr, TypeVar *result);
// D's year (a field of a date): turns expr into LIB_DATE_PART. `base` is D, a date.
TypeVar checker_date_field(Checker *c, const Expr *expr, TypeVar base);
// The part of a date a field's name means (DATE_YEAR, ...), or -1.
int checker_date_part(const char *name);
// Once kinds are decided: a list, map, record or variant that would hold a date is reported (not yet supported).

// check_module.c: files that use other files
// Whether the current file can use a definition in `file` (private: only its own file can).
bool checker_visible(const Checker *c, unsigned file, bool private);
// Before looking up a name in a value (expr: EXPR_NAME or EXPR_CALL): "helpers's double" looks only in
// helpers.es, and a name two used files both define is reported. False if that was reported.
bool checker_enter_names(Checker *c, const Expr *expr, const char *name, NameContext *saved);
void checker_leave_names(Checker *c, const NameContext *saved);
// expr turned out to name a definition: one in another file is renamed when the program is checked.
void checker_resolved(Checker *c, const Expr *expr, const Stmt *definition);
// An unknown name some other file defines (privately, or a file this one doesn't use): reports and returns true.
bool checker_report_hidden(Checker *c, const char *name, const char *display, SourcePos pos);
// "line 4", or "line 4 of helpers.es" for a place in another file.
const char *checker_where(Checker *c, SourcePos pos);
// A top-level statement of a file another file uses: only definitions are allowed. Reports and returns false.
bool checker_check_module_stmt(Checker *c, const Stmt *stmt);
void checker_check_use(Checker *c, const Stmt *stmt);  // STMT_USE
// Gives the definitions in files other than the one that's run names of their own, so two files can use one name.
void checker_rename_modules(Checker *c);

// check_types.c
TypeVar types_new(Checker *c, Type type, size_t line);  // a list type gets a variable for its items
TypeVar types_new_list(Checker *c, TypeVar item, size_t line);
TypeVar types_new_map(Checker *c, TypeVar key, TypeVar value, size_t line);
// A function taking `count` inputs (their kinds copied from params) and giving back `result`.
TypeVar types_new_func(Checker *c, const TypeVar *params, size_t count, TypeVar result, size_t line);
// A function's inputs and result (it must be a function): returns how many inputs it takes.
size_t types_func(Checker *c, TypeVar func, const TypeVar **params, TypeVar *result);
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
bool types_number_first(Checker *c, TypeVar var);  // check_types.c: it becomes a number if nothing decides it
void types_settle(Checker *c);  // check_types.c: settles what can be settled now
void checker_judge_columns(Checker *c);  // check_table_file.c: tables' columns from their files, before the defaults
void checker_check_columns(Checker *c);  // check_table_file.c: columns used as numbers or amounts, against their files
void types_default_numbers(Checker *c);  // check_types.c: KIND_NUMBER_FIRST kinds nothing decided become numbers
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
TypeVar checker_arithmetic(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Span span, size_t line);
bool checker_settle_arith(Checker *c, const Deferred *d);  // check_expr.c: DEFER_ARITH
void types_defer_arith(Checker *c, BinaryOp op, TypeVar left, TypeVar right, TypeVar result, Span span, size_t line);
TypeVar checker_money_arithmetic(Checker *c, BinaryOp op, TypeVar left, TypeVar right, Span span, size_t line);

// The name each item of a list gets ("for each score in", "every score in").
void checker_make_item_name(Checker *c, const Name *name, TypeVar item);
// A block of statements, a scope of its own: names made in it end with it (`where` on `line`).
void checker_check_block(Checker *c, const Block *block, size_t line, const char *where);

// check.c, for check_func_value.c
void checker_check_statements(Checker *c, const Block *block);
bool checker_gives_value(const Block *block);  // a "give back E" anywhere in it
// A function's display for messages: "\"double\"" (quoted), or "the function made on line 3".
const char *checker_function_label(Checker *c, const Function *f);
const char *checker_capital(Checker *c, const char *text);  // with its first letter a capital

// check_func_value.c: functions as values
TypeVar checker_check_function_ref(Checker *c, const Expr *expr);  // the function double
TypeVar checker_check_lambda(Checker *c, const Expr *expr);        // a function taking x ...
// A call of a function kept as a value ("call f with 3", "f with 3"); `name` is the variable called, if it is one.
TypeVar checker_check_value_call(Checker *c, const Expr *expr, bool as_value);
// A name made around a function made inside a value, used (or set) inside it: captured.
void checker_use(Checker *c, const Symbol *symbol, bool setting);
// "x" is checked not to be nothing (or to be a variant) but a function made inside sets it: reports and returns true.
bool checker_reject_set_inside(Checker *c, const Expr *expr, const char *path);
void checker_report_undecided_lambdas(Checker *c);
void checker_write_lambdas(Checker *c);  // the kinds of their inputs, results and captures, into the program

// check_apply.c: built-ins that take a function
TypeVar checker_check_apply(Checker *c, const Expr *expr);  // EXPR_APPLY
TypeVar checker_check_aggregate(Checker *c, const Expr *expr);  // check_aggregate.c: EXPR_AGGREGATE
bool checker_resolve_rows(Checker *c, Expr *expr);  // check_aggregate.c: the list "0rows FIELD" means
TypeVar checker_check_default(Checker *c, const Expr *expr);    // check_narrow.c: X, or D
TypeVar checker_check_table(Checker *c, const Expr *expr);      // check_narrow.c: a "depends on" table
void checker_check_replace(Checker *c, const Stmt *stmt);   // apply F to every X in L, keep only ..., sort L by F

// check_iface.c: interfaces
void checker_collect_interfaces(Checker *c, const Block *program);  // reports interfaces named twice
void checker_check_interface(Checker *c, const Stmt *stmt);         // STMT_INTERFACE
// Whether the kind (a record kind or kind with variants) has every way of the interface; if not, the way it's
// missing (and its method with that name that doesn't match, if there is one).
bool checker_satisfies(Checker *c, Type kind, Type iface, const Way **missing, const Function **wrong);
// A record (or value of a kind with variants) where an interface is kept: true if it fits, or if it doesn't
// and that was reported at expr ("A fish can't be used as something that can speak: ...").
bool checker_iface_fits(Checker *c, TypeVar target, TypeVar value, const Expr *expr);
// An interface two kinds of record both fit, for suggestions (TYPE_UNKNOWN if none).
size_t checker_common_interfaces(Checker *c, const Type *kinds, size_t n, Type *first);  // check_iface.c
// A call of a way on a value of an interface ("speak pet"): true (with *result) if it is one.
bool checker_dispatch(Checker *c, const Expr *expr, const TypeVar *args, bool as_value, TypeVar *result);

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
void checker_end_scope(Checker *c, size_t outer, size_t line, const char *where);
void checker_check_loop(Checker *c, const Stmt *stmt);
void checker_check_event(Checker *c, const Stmt *stmt);  // check_loop.c: STMT_EVENT
void checker_check_try(Checker *c, const Stmt *stmt);    // check_library.c: STMT_TRY
TypeVar checker_field_kind(Checker *c, Field *field, size_t line);
void checker_report_undecided_fields(Checker *c);
TypeVar checker_check_field(Checker *c, const Expr *expr);       // EXPR_FIELD
void checker_check_rule(Checker *c, Stmt *stmt);
Field *checker_add_column(Checker *c, Stmt *def, Field column);  // a table's row: a column the program uses                 // STMT_RULE: "an age must be between 0 and 150"
TypeVar checker_check_new(Checker *c, const Expr *expr);         // EXPR_NEW
const char *checker_capitalized(Checker *c, Type type);           // "A dog", for the start of a sentence

// check_narrow.c
// A value that may be "nothing" (stored, given to a function, given back):
// "nothing" gets the kind nothing (TYPE_NOTHING), which only fits a record
// or nothing; anything else is checked as usual.
TypeVar checker_check_value(Checker *c, const Expr *expr);
// Whether a value of kind `value` can go where `target` is kept: the same
// kind, or a record (or nothing) where a record or nothing is kept.
// `expr` is the value (marked `lifted` when it goes where "or nothing" is
// kept); NULL when there's no expression to mark, and then it must be the same kind.
bool checker_fits(Checker *c, TypeVar target, TypeVar value, const Expr *expr, size_t line);
// After a kind mistake: "It could be nothing. Check it first, ..." when `var` can be nothing.
void checker_note_maybe(Checker *c, TypeVar var, const Expr *at);
// "X is nothing" / "X is not nothing": checks it and sets *result; false if the comparison isn't one.
bool checker_check_nothing_test(Checker *c, const Expr *expr, TypeVar *result);
// A name or field read where it was checked not to be nothing: its record.
TypeVar checker_narrow(Checker *c, const Expr *expr, TypeVar var);
// What a condition being `when` (true or false) says isn't nothing (or is a
// variant), added to c->present.
void checker_narrow_condition(Checker *c, const Expr *condition, bool when);
// The path (a name, or name's field...) is checked: not nothing, and the variant unless it's NO_VARIANT.
void checker_add_narrowing(Checker *c, const char *path, size_t variant);
// The variant a value is checked to be here ("if s is a circle:", "when it's a circle:"), or NO_VARIANT.
size_t checker_narrowed_variant(Checker *c, const Expr *expr);
// Forgets that a name or field (and everything under it) was checked: it's being set.
void checker_kill_path(Checker *c, const Expr *target, const char *name);
// Forgets every check of something a block sets (before a loop body that sets it).
void checker_kill_set_in(Checker *c, const Block *block);
// "rex", "the order's customer"; NULL for any other value.
const char *checker_path(Checker *c, const Expr *expr);
// "\"node's next\" could be nothing, so check it first." for a record-or-nothing used as a record.
void checker_report_maybe(Checker *c, const Expr *at, TypeVar var);
bool checker_settle_present(Checker *c, const Deferred *d);

// check_record.c (fields)
bool checker_settle_field(Checker *c, const Deferred *d);
// A kind of record (or with variants) that has a field called `name`, for suggestions (NULL if none).
const RecordKind *checker_field_owner(const Checker *c, const char *name);

// check_kind.c: kinds with variants
void checker_collect_sums(Checker *c, const Block *program);   // reports kinds and variants named twice
void checker_check_sum(Checker *c, const Stmt *stmt);           // STMT_SUM: its variants' fields
TypeVar checker_check_new_variant(Checker *c, const Expr *expr);  // EXPR_NEW of a variant
// R's NAME where R is of a kind with variants (`base` its kind): R has to be checked to be a variant that has it.
TypeVar checker_sum_field(Checker *c, const Expr *expr, TypeVar base, Type type);
TypeVar checker_check_kind_test(Checker *c, const Expr *expr);  // EXPR_KIND_TEST
void checker_check_match(Checker *c, const Stmt *stmt);         // STMT_MATCH
// The kind with variants that has a field called `name` (TYPE_UNKNOWN if none, TYPE_ERROR if several do).
Type checker_sum_with_field(const Checker *c, const char *name);

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
