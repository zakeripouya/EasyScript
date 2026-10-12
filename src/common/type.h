#ifndef COMMON_TYPE_H
#define COMMON_TYPE_H

// The kinds of values (Type) and the kinds a program defines (Records):
// records, kinds with variants, and kinds of map. Part of the syntax tree
// (common/ast.h), which includes this.

#include <stdbool.h>
#include <stddef.h>
#include "common/arena.h"
#include "common/util.h"

// The kind of a value, worked out by the checker before the program runs. A
// list holds one kind of item, so a list type is its innermost kind plus
// TYPE_LIST for each level of list: TYPE_LIST + TYPE_NUMBER is a list of
// numbers, 2 * TYPE_LIST + TYPE_TEXT a list of lists of text. A record kind
// the program defines is TYPE_RECORD + its index in the program's Records,
// and a kind with variants ("a shape is one of:") TYPE_SUM + its index in
// Records.sums. TYPE_MAYBE added to a kind that isn't a list or a map makes
// it "or nothing" ("a dog or nothing", "a number or nothing"). A kind of map
// is TYPE_MAP + its index in Records.maps, where every kind of map the
// program uses is kept once (a map's key and value kinds don't fit in one
// number), and a kind of function ("a function taking a number and giving
// back text") TYPE_FUNC + its index in Records.funcs likewise. An interface
// ("something that can speak is anything with:") is TYPE_IFACE + its index
// in Records.interfaces. Two types are the same exactly when they're equal numbers.
typedef unsigned Type;

enum {
    TYPE_UNKNOWN,  // not decided (yet); TYPE_LIST + TYPE_UNKNOWN is a list of something not decided
    TYPE_NUMBER,
    TYPE_TEXT,
    TYPE_YESNO,
    TYPE_NOTHING,  // what a function gives back when it gives back no value, and the value "nothing"
    TYPE_ERROR,    // stands in after a type error, so one mistake gives one message
    TYPE_DATE,     // a day, and maybe a time of day ("today", "the time now"); a number of seconds while running
    TYPE_DOLLARS,  // an exact amount of money ("$19.99"): whole cents (a long long while running)
    TYPE_EUROS,    // the same in euros ("€4.50")
    TYPE_POUNDS,   // and in pounds ("£3")
    TYPE_TEXTKIND = 16,     // TYPE_TEXTKIND + n: the program's kind of text n ("an email is text that must ..."),
                            // only while checking: codegen sees text
    TYPE_RECORD = 64,       // TYPE_RECORD + n: the program's record kind n
    TYPE_IFACE = 1u << 10,  // TYPE_IFACE + n: the program's interface n (Records.interfaces)
    TYPE_SUM = 1u << 11,    // TYPE_SUM + n: the program's kind with variants n (Records.sums)
    TYPE_FUNC = 1u << 12,   // TYPE_FUNC + n: kind of function n (Records.funcs); TYPE_FUNC alone: inputs not decided
    TYPE_MAP = 1u << 13,    // TYPE_MAP + n: kind of map n (Records.maps); TYPE_MAP alone: a map, kinds not decided
    TYPE_MAYBE = 1u << 14,  // added to a kind: "or nothing"; alone: something or nothing, kind not decided
    TYPE_LIST = 1u << 15,
};

static inline Type type_list_of(Type item) { return item + TYPE_LIST; }
static inline bool type_is_list(Type type) { return type >= TYPE_LIST; }
static inline Type type_item(Type list) { return list - TYPE_LIST; }
static inline Type type_base(Type type) { return type % TYPE_LIST; }  // the innermost kind
static inline Type type_present(Type type) { return type_is_list(type) ? type : type & ~(Type)TYPE_MAYBE; }
static inline bool type_is_map(Type type) { return !type_is_list(type) && type >= TYPE_MAP && type < TYPE_MAYBE; }
static inline bool type_is_record(Type type) {
    return !type_is_list(type) && type_present(type) >= TYPE_RECORD && type_present(type) < TYPE_IFACE;
}
static inline bool type_is_textkind(Type type) { return type >= TYPE_TEXTKIND && type < TYPE_RECORD; }
static inline Type type_textkind(size_t index) { return TYPE_TEXTKIND + (Type)index; }
static inline size_t type_textkind_index(Type type) { return type - TYPE_TEXTKIND; }
static inline bool type_is_iface(Type type) {
    return !type_is_list(type) && type_present(type) >= TYPE_IFACE && type_present(type) < TYPE_SUM;
}
static inline bool type_is_func(Type type) {
    return !type_is_list(type) && type >= TYPE_FUNC && type < TYPE_MAP;
}
static inline bool type_is_sum(Type type) {
    return !type_is_list(type) && type_present(type) >= TYPE_SUM && type_present(type) < TYPE_FUNC;
}
// "a dog or nothing", "a number or nothing" (not TYPE_MAYBE alone, whose kind isn't decided)
static inline bool type_is_maybe(Type type) {
    return !type_is_list(type) && (type & TYPE_MAYBE) && type != TYPE_MAYBE;
}
static inline Type type_record(size_t index) { return TYPE_RECORD + (Type)index; }
static inline size_t type_record_index(Type type) { return type_present(type) - TYPE_RECORD; }
static inline Type type_sum(size_t index) { return TYPE_SUM + (Type)index; }
static inline size_t type_sum_index(Type type) { return type_present(type) - TYPE_SUM; }
static inline Type type_iface(size_t index) { return TYPE_IFACE + (Type)index; }
static inline size_t type_iface_index(Type type) { return type_present(type) - TYPE_IFACE; }
// A kind that can be "or nothing": a number, text, yes/no, a record kind, a
// kind with variants or an interface (not a list or a map, which can be
// empty instead, or a function).
static inline bool type_can_be_nothing(Type type) {
    return !type_is_maybe(type) && (type == TYPE_NUMBER || type == TYPE_TEXT || type == TYPE_YESNO ||
                                    type_is_record(type) || type_is_sum(type) || type_is_iface(type));
}
// Kept in a box while the program runs when it's "or nothing": a number, text or yes/no.
static inline bool type_is_money(Type type) {
    return type == TYPE_DOLLARS || type == TYPE_EUROS || type == TYPE_POUNDS;
}
static inline bool type_boxed(Type present) {
    return present == TYPE_NUMBER || present == TYPE_TEXT || present == TYPE_YESNO;
}
// Kept as a record while the program runs (an EsRecord *, NULL for nothing):
// records, values of kinds with variants and interfaces, functions (a
// closure, es_closure.h), and anything that can be nothing.
static inline bool type_is_object(Type type) {
    return type_is_record(type) || type_is_sum(type) || type_is_iface(type) || type_is_func(type) ||
           type_is_maybe(type);
}
static inline bool type_counted(Type type) {  // reference counted
    return type == TYPE_TEXT || type_is_list(type) || type_is_object(type) || type_is_map(type);
}

typedef struct Stmt Stmt;

// A kind of record the program defines ("a dog is a kind of thing with:"),
// found by the parser's pre-scan; its index makes its Type.
typedef struct {
    const char *name;     // matched: lowercased, one space between words
    const char *display;  // as written in the definition
    const char *plural;   // matched: "dogs", "boxes", "berries"
    Stmt *definition;     // STMT_RECORD (the first one, if it's defined twice)
    bool table;           // a row of "read the table in F": its fields are the columns the program uses
} RecordKind;

// One variant of a kind with variants ("a circle with a radius (a number)"
// in "a shape is one of:"), found by the pre-scan. Variants are numbered
// across the whole program, so a variant's name says which kind it's in.
typedef struct {
    const char *name;     // matched: lowercased, one space between words
    const char *display;  // as written in the definition
    size_t sum;           // its kind: Records.sums[sum]
    size_t index;         // its place in that kind's definition
} VariantKind;

// A kind of map: what its keys are (text or numbers) and what its values are.
typedef struct {
    Type key;
    Type value;
} MapKind;

// A kind of function: the kinds of its inputs, and what it gives back
// (TYPE_NOTHING: no value).
typedef struct {
    const Type *params;
    size_t count;
    Type result;
} FuncKind;

// The kinds the program defines and uses: its record kinds (items, len and
// cap, as in a Vec, so vec_push works on it), its kinds with variants (each
// a RecordKind whose definition is a STMT_SUM) and all their variants, and
// every kind of map and of function, each kept once so that its Type is a
// single number. maps[0] is "a map" whose key and value kinds aren't
// decided (TYPE_MAP itself), and funcs[0] "a function" whose inputs aren't.
// Interfaces are RecordKinds whose definition is a STMT_INTERFACE.
typedef struct {
    RecordKind *items;
    size_t len;
    size_t cap;
    Vec(RecordKind) sums;
    Vec(VariantKind) variants;
    Vec(MapKind) maps;
    Vec(FuncKind) funcs;
    Vec(RecordKind) interfaces;
    Vec(RecordKind) texts;  // kinds of text ("an email is text that must contain "@"."); definition: the STMT_RULE
    size_t program_result;  // 1 + the index of "program result" (what "run ... and call the result r" makes), or 0
} Records;

// "a number", "text", "a yes/no value", "nothing", "a list of numbers",
// "a list of lists of text", "a list" (items not decided), "a dog", "a dog
// or nothing", "a list of dogs", "a map from text to numbers", "a map"
// (kinds not decided). `records` names the record kinds and maps (NULL if
// there are none).
const char *ast_type_name(Arena *arena, const Records *records, Type type);

// The Type of a map from `key` to `value`, adding the kind to records->maps
// the first time it's used.
Type ast_map_type(Arena *arena, Records *records, Type key, Type value);
Type ast_map_key(const Records *records, Type map);    // TYPE_UNKNOWN if not decided
Type ast_map_value(const Records *records, Type map);  // TYPE_UNKNOWN if not decided
// The Type of a function taking `count` inputs of kinds `params` and giving
// back `result`, adding the kind to records->funcs the first time it's used.
Type ast_func_type(Arena *arena, Records *records, const Type *params, size_t count, Type result);
const FuncKind *ast_func_kind(const Records *records, Type func);  // NULL for TYPE_FUNC alone
// Whether some part of the kind isn't decided: its innermost kind, or a
// map's keys or values (or a record or nothing whose kind of record isn't).
bool ast_type_undecided(const Records *records, Type type);

#endif
