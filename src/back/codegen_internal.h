#ifndef BACK_CODEGEN_INTERNAL_H
#define BACK_CODEGEN_INTERNAL_H

// Shared by the C code generator's files (codegen_c.c, codegen_expr.c). Not
// for use outside src/back/.

#include <stdbool.h>
#include "back/codegen_c.h"

// A loop being generated, for "it", "stop the loop" and "skip this one".
typedef struct {
    const Stmt *stmt;
    size_t id;     // es_cN / es_iN / es_nN
    size_t scope;  // index in Codegen.scopes of the loop's body
} LoopCode;

// A variable: its name and kind. The C name depends on both (es_vn_, es_vt_,
// es_vb_), so blocks side by side can make the same name with different kinds.
typedef struct {
    const char *name;
    Type type;
} VarCode;

// The text variables first made in one block, released when the block ends
// (or is left early by stop the loop, skip this one, give back). Numbers and
// yes/no values own nothing, so they're never released.
typedef struct {
    Vec(VarCode) names;
} NameScope;

// Text literals, as static immortal text objects (es_s1, es_s2, ...),
// shared by main() and every function.
typedef struct {
    StrBuf code;
    size_t count;
} TextPool;

typedef struct {
    Arena *arena;
    TextPool *texts;
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
const char *cg_ctype(Type type);  // "double", "bool", "EsText *", "void"
// A C name: prefix plus the name with "_" doubled and "'" as "_q".
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

#endif
