#ifndef BACK_CODEGEN_INTERNAL_H
#define BACK_CODEGEN_INTERNAL_H

// Shared by the C code generator's files (codegen_c.c, codegen_expr.c). Not
// for use outside src/back/.

#include <stdbool.h>
#include "back/codegen_c.h"

// A loop being generated, for "it".
typedef struct {
    const Stmt *stmt;
    size_t id;  // es_cN / es_iN / es_nN
} LoopCode;

typedef struct {
    Arena *arena;
    StrBuf body;               // statements of main()
    Vec(const char *) globals; // variable names, in the order they're first made
    size_t temps;              // es_t1 ... es_tN, declared at the top of main()
    size_t depth;              // how many blocks the current statement is inside
    Vec(LoopCode) loops;       // loops the current statement is inside, innermost last
    size_t loop_ids;
    Vec(const Stmt *) functions; // every top-level STMT_FUNCTION
    Vec(const Stmt *) constants; // every top-level STMT_CONSTANT
    const Stmt *function;        // the function being generated, or NULL for main()
    Vec(const char *) locals;    // names made in that function, hoisted to its top
} Codegen;

// codegen_expr.c
void cg_emit_expr(Codegen *g, const Expr *expr, StrBuf *out);
// fn(line, OPERAND), or fn(OPERAND) when line is 0.
void cg_emit_call1(Codegen *g, const char *fn, size_t line, const Expr *operand, StrBuf *out);
// A C name: prefix plus the name with "_" doubled and "'" as "_q".
void cg_append_prefixed(StrBuf *out, const char *prefix, const char *name);
void cg_append_name(StrBuf *out, const char *name);  // a variable: "es_v_" prefix
void cg_append_c_string(StrBuf *out, const char *s, size_t len);
// A name the program makes: a global in main(), a hoisted local in a function.
void cg_declare_name(Codegen *g, const char *name);
size_t cg_new_temp(Codegen *g);

#endif
