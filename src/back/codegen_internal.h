#ifndef BACK_CODEGEN_INTERNAL_H
#define BACK_CODEGEN_INTERNAL_H

// Shared by the C code generator's files (codegen_c.c, codegen_expr.c,
// codegen_list.c, codegen_text.c, codegen_record.c, codegen_map.c). Not for
// use outside src/back/.

#include <stdbool.h>
#include "back/codegen_c.h"

// A loop being generated, for "it", "stop the loop" and "skip this one".
// "every" and "only" helpers have one too (stmt NULL), for "it".
typedef struct {
    const Stmt *stmt;
    size_t id;         // es_cN / es_iN / es_nN
    size_t scope;      // index in Codegen.scopes of the loop's body
    const char *item;  // for-each loops and helpers: the variable "it" reads
    Type item_type;
} LoopCode;

// A variable: its name and kind. The C name depends on both (es_vn_, es_vt_,
// es_vb_), so blocks side by side can make the same name with different kinds.
typedef struct {
    const char *name;
    Type type;
} VarCode;

// The text and list variables first made in one block, released when the
// block ends (or is left early by stop the loop, skip this one, give back).
// Numbers and yes/no values own nothing, so they're never released.
typedef struct {
    Vec(VarCode) names;
} NameScope;

// Text literals, as static immortal text objects (es_s1, es_s2, ...),
// shared by main() and every function.
typedef struct {
    StrBuf code;
    size_t count;
} TextPool;

// The places runtime calls that can fail come from (es_sites[N]: line,
// column, length), for runtime errors to show the line and carets.
typedef struct {
    StrBuf code;
    size_t count;
} SitePool;

// The C functions that "every" and "only" become (es_every1, es_only2, ...).
typedef struct {
    StrBuf code;
    size_t count;
} HelperPool;

typedef struct {
    Arena *arena;
    const Records *records;      // the program's kinds of record and of map
    TextPool *texts;
    StrBuf *inits;               // statements that start main(): making the fixed maps
    HelperPool *helpers;
    SitePool *sites;
    StrBuf body;                 // statements of main() or of the function
    Vec(VarCode) vars;           // variables made there, hoisted to its top
    Vec(Type) temps;             // es_t1 ... es_tN (index + 1), declared at its top
    size_t depth;                // how many blocks the current statement is inside
    Vec(LoopCode) loops;         // loops the current statement is inside, innermost last
    size_t loop_ids;
    Vec(const Stmt *) functions; // every top-level STMT_FUNCTION
    Vec(const Stmt *) constants; // every top-level STMT_CONSTANT
    const Stmt *function;        // the function being generated, or NULL for main()
    Vec(NameScope) scopes;       // blocks the current statement is inside; [0] is the top level
    bool stops;                  // main() has a "stop the program" (goto es_end)
    Vec(VarCode) captured;       // in a helper: the variables it's given (its parameters)
} Codegen;

// codegen_expr.c
// The C expression for a value, of the C type of its kind.
void cg_emit_expr(Codegen *g, const Expr *expr, StrBuf *out);
// The same value as owned text (EsText *), converting numbers and yes/no.
void cg_emit_text(Codegen *g, const Expr *expr, StrBuf *out);
// Whether working the value out can fail or have effects (a call, a file,
// dividing, "as a number" on text). Two such values keep their order through
// a temporary, because C doesn't fix the order of operands.
bool cg_has_effects(const Codegen *g, const Expr *expr);
const char *cg_ctype(Type type);  // "double", "bool", "EsText *", "EsList *", "EsMap *", "void"
// For text and lists (type_counted): the runtime functions that retain,
// release, store into and drop a variable, and what an empty variable holds.
const char *cg_retain(Type type);   // "es_retain", "es_list_retain"
const char *cg_release(Type type);  // "es_release", "es_list_release"
const char *cg_set(Type type);      // "es_set", "es_list_set"
const char *cg_drop(Type type);     // "es_drop", "es_list_drop"
const char *cg_empty(Type type);    // "&es_empty_text", "&es_no_list", "false", "0"
// The C code for one value, as text or as its own kind.
const char *cg_operand(Codegen *g, const Expr *expr, bool as_text);
// n values worked out left to right, their code in values[]: when two or
// more have effects, each goes into a temporary first, and "(es_t1 = A, "...
// is appended to out; then it returns true and the caller adds a ")".
bool cg_ordered(Codegen *g, Expr *const *exprs, size_t n, const char **values, StrBuf *out);
// A list or map that's only looked at (an item, contains, length, a slice,
// a loop, every/only, as text, a lookup): a fixed list or map is used
// directly instead of copied.
const char *cg_list_operand(Codegen *g, const Expr *expr);
bool cg_is_constant(const Codegen *g, const char *name);
// The site of a node, for a runtime call that can fail: its index in es_sites.
size_t cg_site(Codegen *g, SourcePos pos);
// A C name: prefix plus the name with "_" doubled, "'" as "_q", and spaces as "_".
void cg_append_prefixed(StrBuf *out, const char *prefix, const char *name);
void cg_append_var(StrBuf *out, const char *name, Type type);  // es_vn_total, es_vt_name, ...
void cg_append_c_string(StrBuf *out, const char *s, size_t len);
// A new static text object holding the text; returns its number N (es_sN).
size_t cg_static_text(Codegen *g, const char *s, size_t len);
// A variable the program makes: hoisted to the top of main() or the function;
// text ones are also recorded in the innermost scope, unless already visible.
void cg_declare_name(Codegen *g, const char *name, Type type);
size_t cg_new_temp(Codegen *g, Type type);
// A function's definition, by name.
const Stmt *cg_find_function(const Codegen *g, const char *name);
// Reading a variable: "es_vn_total", or "es_retain(es_vt_name)" for text and lists.
void cg_emit_var(StrBuf *out, const char *name, Type type);

// codegen_c.c
// Temporaries and hoisted variables, at the top of main(), a function, or a helper.
void cg_emit_declarations(const Codegen *g, StrBuf *out);
// codegen_c.c, for codegen_program.c: statements, the outermost scope, the
// drops before leaving a function or the program, and leaving a frame.
void cg_emit_stmt(Codegen *g, const Stmt *stmt);
void cg_push_scope(Codegen *g);
void cg_emit_drop_all(Codegen *g);
const char *cg_leave_code(const Codegen *g);
// A worked-out number as C: "0.20000000000000001", "NAN", "-HUGE_VAL".
void cg_append_number(StrBuf *out, double x);
// A fixed list (a constant, or a field's starting value) as a static immortal list es_k_NAME.
void cg_emit_fixed_list(Codegen *g, const char *name, const ConstValue *v, StrBuf *out);

// codegen_record.c
void cg_emit_records(Codegen *g, StrBuf *out);                // every kind's struct, description and maker
void cg_emit_new(Codegen *g, const Expr *expr, StrBuf *out);    // EXPR_NEW
void cg_emit_field(Codegen *g, const Expr *expr, StrBuf *out);  // EXPR_FIELD
// "set R's F to V" or "add V to R's F" (and the other changes) as one C statement.
const char *cg_field_stmt(Codegen *g, const Stmt *stmt);

// codegen_list.c
const char *cg_item_suffix(Type list);  // the runtime's suffix for its items: "n", "b", "t", "l"
const char *cg_value_suffix(Type type); // the same for a value of that kind: "n", "b", "t", "l", "r", "m"
const char *cg_items(Type type);        // what the runtime calls a kind of item: "ES_ITEMS_NUMBER", ...
void cg_emit_list_expr(Codegen *g, const Expr *expr, StrBuf *out);  // EXPR_LIST ... EXPR_ONLY
void cg_emit_contains(Codegen *g, const Expr *expr, StrBuf *out);   // L contains X
// The C loop header of "for each N in L" (after its variables are declared).
void cg_emit_each_head(Codegen *g, const Stmt *stmt, size_t id, StrBuf *body, size_t depth);
const char *cg_list_stmt(Codegen *g, const Stmt *stmt);  // a list sentence as one C statement (no ";")

// codegen_map.c
void cg_emit_map_expr(Codegen *g, const Expr *expr, StrBuf *out);  // EXPR_MAP, EXPR_EMPTY_MAP, EXPR_MAP_GET
void cg_emit_has(Codegen *g, const Expr *expr, StrBuf *out);       // M has K
// The number of entries in M, the keys and values of M, M is empty, a copy of M.
void cg_emit_map_op(Codegen *g, const Expr *expr, StrBuf *out);
// A fixed map (a constant, or a field's starting value): a static es_k_NAME made when main() starts (g->inits).
void cg_emit_fixed_map(Codegen *g, const char *name, const ConstValue *v, StrBuf *out);
// The C loop header of "for each K and V in M" (after its variables are declared).
void cg_emit_each_map_head(Codegen *g, const Stmt *stmt, size_t id, StrBuf *body, size_t depth);
const char *cg_remove_key(Codegen *g, const Stmt *stmt);  // remove K from M, as one C expression (no ";")
// "set the value for K in M to V" or "add V to the value for K in M" (and the other changes) as one C statement.
const char *cg_map_stmt(Codegen *g, const Stmt *stmt);

// codegen_text.c
void cg_emit_text_op(Codegen *g, const Expr *expr, StrBuf *out);       // EXPR_TEXT_OP
void cg_emit_characters(Codegen *g, const Expr *expr, StrBuf *out);    // EXPR_ITEM/EXPR_SLICE of text
void cg_emit_text_search(Codegen *g, const Expr *expr, StrBuf *out);   // contains (text), starts with, ends with
bool cg_text_op_effects(const Codegen *g, const Expr *expr);           // cg_has_effects for EXPR_TEXT_OP
// "set T to T followed by ...": adds to T in place (es_append); false if the sentence isn't like that.
bool cg_emit_append(Codegen *g, const Stmt *stmt);

#endif
