// Kinds of values: their names in messages ("a number", "a list of dogs",
// "a shape or nothing") and the kinds of map a program uses.

#include <string.h>
#include "common/ast.h"

// "a dog", "an apple": the article that goes before a word. An interface
// called "something that can speak" (or "anything ...") takes none.
static const char *article(const char *word) {
    if (strncmp(word, "something ", 10) == 0 || strncmp(word, "anything ", 9) == 0) return "";
    return strchr("aeiouAEIOU", word[0]) ? "an " : "a ";
}

// The record kind, kind with variants or interface a type is (or is "or nothing" of).
static const RecordKind *record_kind(const Records *records, Type type) {
    if (!records) return NULL;
    if (type_is_sum(type)) {
        size_t index = type_sum_index(type);
        return index < records->sums.len ? &records->sums.items[index] : NULL;
    }
    if (type_is_iface(type)) {
        size_t index = type_iface_index(type);
        return index < records->interfaces.len ? &records->interfaces.items[index] : NULL;
    }
    size_t index = type_record_index(type);
    return type_is_record(type) && index < records->len ? &records->items[index] : NULL;
}

Type ast_map_type(Arena *arena, Records *records, Type key, Type value) {
    if (records->maps.len == 0) {  // maps[0]: a map whose kinds aren't decided
        MapKind undecided = {TYPE_UNKNOWN, TYPE_UNKNOWN};
        vec_push(arena, &records->maps, undecided);
    }
    for (size_t i = 0; i < records->maps.len; i++) {
        if (records->maps.items[i].key == key && records->maps.items[i].value == value) return TYPE_MAP + (Type)i;
    }
    MapKind kind = {key, value};
    vec_push(arena, &records->maps, kind);
    return TYPE_MAP + (Type)(records->maps.len - 1);
}

static const MapKind *map_kind(const Records *records, Type map) {
    size_t index = map - TYPE_MAP;
    return records && index < records->maps.len ? &records->maps.items[index] : NULL;
}

Type ast_map_key(const Records *records, Type map) {
    const MapKind *kind = map_kind(records, map);
    return kind ? kind->key : TYPE_UNKNOWN;
}

Type ast_map_value(const Records *records, Type map) {
    const MapKind *kind = map_kind(records, map);
    return kind ? kind->value : TYPE_UNKNOWN;
}

Type ast_func_type(Arena *arena, Records *records, const Type *params, size_t count, Type result) {
    if (records->funcs.len == 0) {  // funcs[0]: a function whose inputs aren't decided
        FuncKind undecided = {NULL, 0, TYPE_UNKNOWN};
        vec_push(arena, &records->funcs, undecided);
    }
    for (size_t i = 1; i < records->funcs.len; i++) {
        const FuncKind *kind = &records->funcs.items[i];
        if (kind->count == count && kind->result == result &&
            (count == 0 || memcmp(kind->params, params, count * sizeof(Type)) == 0)) {
            return TYPE_FUNC + (Type)i;
        }
    }
    Type *copy = arena_alloc(arena, (count + 1) * sizeof(Type));
    if (count > 0) memcpy(copy, params, count * sizeof(Type));
    FuncKind kind = {copy, count, result};
    vec_push(arena, &records->funcs, kind);
    return TYPE_FUNC + (Type)(records->funcs.len - 1);
}

const FuncKind *ast_func_kind(const Records *records, Type func) {
    size_t index = func - TYPE_FUNC;
    return records && index > 0 && index < records->funcs.len ? &records->funcs.items[index] : NULL;
}

bool ast_type_undecided(const Records *records, Type type) {
    Type base = type_base(type);
    if (type_is_map(base)) {
        return ast_type_undecided(records, ast_map_key(records, base)) ||
               ast_type_undecided(records, ast_map_value(records, base));
    }
    if (type_is_func(base)) {
        const FuncKind *kind = ast_func_kind(records, base);
        if (!kind) return true;
        for (size_t i = 0; i < kind->count; i++) {
            if (ast_type_undecided(records, kind->params[i])) return true;
        }
        return ast_type_undecided(records, kind->result);
    }
    return base == TYPE_UNKNOWN || base == TYPE_MAYBE;
}

static const char *map_name(Arena *arena, const Records *records, Type type);
static const char *func_name(Arena *arena, const Records *records, Type type);

// What a list or map holds, in the plural: "numbers", "lists of text",
// "dogs", "maps from text to numbers"; NULL when the innermost kind isn't decided.
static const char *plural_name(Arena *arena, const Records *records, Type type) {
    static const char *const plural[] = {NULL, "numbers", "text", "yes/no values"};
    StrBuf sb;
    sb_init(&sb, arena);
    for (Type t = type; type_is_list(t); t = type_item(t)) sb_append(&sb, "lists of ");
    Type base = type_base(type);
    Type present = type_present(base);
    if (present == TYPE_DATE) {
        sb_append(&sb, "dates");
        return sb.data;
    }
    if (type_is_money(present)) {
        sb_append(&sb, present == TYPE_DOLLARS ? "amounts in dollars" : present == TYPE_EUROS ? "amounts in euros"
                                                                                               : "amounts in pounds");
        return sb.data;
    }
    if (present == TYPE_NUMBER || present == TYPE_TEXT || present == TYPE_YESNO) {
        sb_appendf(&sb, "%s%s", plural[present], type_is_maybe(base) ? " or nothing" : "");
        return sb.data;
    }
    const RecordKind *kind = record_kind(records, base);
    if (kind) {
        sb_appendf(&sb, "%s%s", kind->plural, type_is_maybe(base) ? " or nothing" : "");
        return sb.data;
    }
    if (type_is_func(base)) {
        sb_appendf(&sb, "functions%s", func_name(arena, records, base) + 10);  // without "a function"
        return sb.data;
    }
    if (!type_is_map(base)) return NULL;
    sb_appendf(&sb, "maps%s", map_name(arena, records, base) + 5);  // "a map from ..." without "a map"
    return sb.data;
}

// "a list of numbers", "a list of lists of text", "a list of dogs"; "a list"
// (or "a list of lists") when the innermost kind isn't decided.
static const char *list_name(Arena *arena, const Records *records, Type type) {
    const char *items = plural_name(arena, records, type_item(type));
    if (items) return arena_sprintf(arena, "a list of %s", items);
    StrBuf sb;
    sb_init(&sb, arena);
    sb_append(&sb, "a list");
    for (Type t = type_item(type); type_is_list(t); t = type_item(t)) sb_append(&sb, " of lists");
    return sb.data;
}

// "a map from text to numbers", "a map from numbers to lists of text"; "a
// map" when its keys or values aren't decided.
static const char *map_name(Arena *arena, const Records *records, Type type) {
    Type key = ast_map_key(records, type);
    const char *values = plural_name(arena, records, ast_map_value(records, type));
    if ((key != TYPE_TEXT && key != TYPE_NUMBER) || !values) return "a map";
    return arena_sprintf(arena, "a map from %s to %s", key == TYPE_TEXT ? "text" : "numbers", values);
}

// Whether a kind's name ends in a function's kind ("a function taking a
// number", "a list of functions giving back text"), so after it the "and"
// of another input would be ambiguous.
static bool ends_in_function(const Records *records, Type type) {
    Type base = type_base(type);
    if (type_is_map(base)) return ends_in_function(records, ast_map_value(records, base));
    return type_is_func(base);
}

// "a function taking a number and text and giving back a yes/no value", "a
// function giving back a number", "a function taking text"; an input that
// is a function itself goes in parentheses. "a function" when its inputs
// aren't decided (or when it takes none and gives back nothing).
static const char *func_name(Arena *arena, const Records *records, Type type) {
    const FuncKind *kind = ast_func_kind(records, type);
    if (!kind) return "a function";
    StrBuf sb;
    sb_init(&sb, arena);
    sb_append(&sb, "a function");
    for (size_t i = 0; i < kind->count; i++) {
        const char *name = ast_type_name(arena, records, kind->params[i]);
        bool parenthesized = ends_in_function(records, kind->params[i]);
        sb_appendf(&sb, "%s%s%s%s", i == 0 ? " taking " : " and ", parenthesized ? "(" : "", name,
                   parenthesized ? ")" : "");
    }
    if (kind->result != TYPE_NOTHING && kind->result != TYPE_UNKNOWN) {  // (undecided: said nothing about)
        sb_appendf(&sb, "%sgiving back %s", kind->count > 0 ? " and " : " ",
                   ast_type_name(arena, records, kind->result));
    }
    return sb.data;
}

const char *ast_type_name(Arena *arena, const Records *records, Type type) {
    if (type_is_list(type)) return list_name(arena, records, type);
    if (type_is_func(type)) return func_name(arena, records, type);
    if (type_is_map(type)) return map_name(arena, records, type);
    if (type == TYPE_MAYBE) return "something or nothing";
    if (type_is_maybe(type) && type_boxed(type_present(type))) {
        return arena_sprintf(arena, "%s or nothing", ast_type_name(arena, records, type_present(type)));
    }
    const RecordKind *kind = record_kind(records, type);
    if (kind) {
        return arena_sprintf(arena, "%s%s%s", article(kind->display), kind->display,
                             type_is_maybe(type) ? " or nothing" : "");
    }
    switch (type) {
    case TYPE_NUMBER: return "a number";
    case TYPE_TEXT: return "text";
    case TYPE_YESNO: return "a yes/no value";
    case TYPE_DATE: return "a date";
    case TYPE_DOLLARS: return "an amount in dollars";
    case TYPE_EUROS: return "an amount in euros";
    case TYPE_POUNDS: return "an amount in pounds";
    case TYPE_NOTHING: return "nothing";
    default: return "an unknown kind of value";
    }
}

