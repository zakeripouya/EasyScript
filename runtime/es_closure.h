/* EasyScript runtime, part 10: functions as values. Parts 1 to 9 come first.
 *
 * A function kept as a value ("the function double", "a function taking x
 * that gives back x times 2") is a closure: a record (EsClosure) holding a
 * pointer to its C code, and, as its fields, the cells of the names it uses
 * from around it. The compiler writes one C function per function value,
 * taking the call's site, the closure itself and the inputs, and calls it
 * through the code pointer cast to that exact type (the checker knows every
 * kind, so the cast is always right).
 *
 * A name that a function made inside uses is kept in a cell: a record with
 * one field (es_cell_type_n, ...). The code around it and every closure that
 * uses it share the cell, so a change either one makes is seen by both (a
 * counter made by "to make counter" counts on). Each "let" makes a new cell,
 * so closures made in different rounds of a loop each keep their own.
 *
 * A function defined with "to" is a static immortal closure (es_fv_NAME)
 * with no cells, whose code (es_w_NAME) calls the function. */
#ifndef ES_CLOSURE_H
#define ES_CLOSURE_H

#ifndef ES_KIND_H
#include "es_kind.h"
#endif

typedef void (*EsCode)(void);

typedef struct {
    EsRecord base;
    EsCode code;
} EsClosure;

/* The C function a closure runs, as the function pointer type T. Only looks. */
#define ES_CODE(f, T) ((T)((const EsClosure *)(f))->code)

/* A new closure; the compiler puts its cells (retained) in its fields. */
static EsRecord *es_closure_new(const EsRecordType *type, EsCode code) {
    EsRecord *r = es_record_new(type);
    ((EsClosure *)r)->code = code;
    return r;
}

/* --- Cells ----------------------------------------------------------------------- */

/* What a cell holds, as a C lvalue of type T. */
#define ES_CELL(c, T) ES_FIELD(c, T, sizeof(EsRecord))

static const char *const es_cell_names[] = {"value"};
static const size_t es_cell_at[] = {sizeof(EsRecord)};

/* es_cell_n(v): a new cell holding v (taken). */
#define ES_CELL_KIND(S, T, KIND)                                                                             \
    static const EsRecordType es_cell_type_##S = {"a name", 1, es_cell_names, KIND, es_cell_at,               \
                                                  sizeof(EsRecord) + sizeof(T), ES_CELL};                   \
    static EsRecord *es_cell_##S(T v) {                                                                      \
        EsRecord *c = es_record_new(&es_cell_type_##S);                                                      \
        ES_CELL(c, T) = v;                                                                                   \
        return c;                                                                                            \
    }

ES_CELL_KIND(n, double, "n")
ES_CELL_KIND(b, bool, "b")
ES_CELL_KIND(t, EsText *, "t")
ES_CELL_KIND(l, EsList *, "l")
ES_CELL_KIND(r, EsRecord *, "r")
ES_CELL_KIND(m, EsMap *, "m")

/* --- The memory check ------------------------------------------------------------- */

static bool es_report_function_cycle(size_t circled, EsRecord *oldest) {
    fprintf(stderr, "Memory check: %zu value%s still alive at the end of the program, because a function keeps "
                    "itself alive through a name it uses:\n", circled, circled == 1 ? " was" : "s were");
    size_t shown = 0;
    for (EsRecord *r = oldest; r && shown < 3; r = r->prev) {
        if (r->mark == ES_REACHED || r->type->style == ES_CELL) continue;
        EsText *t = es_record_short_text(r);
        fputs("    ", stderr);
        es_print_cut(t, 70, false);
        fputc('\n', stderr);
        es_release(t);
        shown++;
    }
    if (circled > shown) fprintf(stderr, "    ...and %zu more\n", circled - shown);
    fputs("A function that uses the name it's kept in (or a record, list or map that holds it) is never freed. "
          "Define it with \"to\" instead, or set that name to something else before the program ends.\n", stderr);
    return true;
}

#endif
