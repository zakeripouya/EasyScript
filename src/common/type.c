// Kinds of values: their names in messages ("a number", "a list of dogs",
// "a shape or nothing") and the kinds of map a program uses.

#include <string.h>
#include "common/ast.h"

// "a dog", "an apple": the article that goes before a word.
static const char *article(const char *word) {
    return strchr("aeiouAEIOU", word[0]) ? "an" : "a";
}

// The record kind or kind with variants a type is (or is "or nothing" of).
static const RecordKind *record_kind(const Records *records, Type type) {
    if (!records) return NULL;
    if (type_is_sum(type)) {
        size_t index = type_sum_index(type);
        return index < records->sums.len ? &records->sums.items[index] : NULL;
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

bool ast_type_undecided(const Records *records, Type type) {
    Type base = type_base(type);
    if (type_is_map(base)) {
        return ast_type_undecided(records, ast_map_key(records, base)) ||
               ast_type_undecided(records, ast_map_value(records, base));
    }
    return base == TYPE_UNKNOWN || base == TYPE_MAYBE;
}

static const char *map_name(Arena *arena, const Records *records, Type type);

// What a list or map holds, in the plural: "numbers", "lists of text",
// "dogs", "maps from text to numbers"; NULL when the innermost kind isn't decided.
static const char *plural_name(Arena *arena, const Records *records, Type type) {
    static const char *const plural[] = {NULL, "numbers", "text", "yes/no values"};
    StrBuf sb;
    sb_init(&sb, arena);
    for (Type t = type; type_is_list(t); t = type_item(t)) sb_append(&sb, "lists of ");
    Type base = type_base(type);
    if (base == TYPE_NUMBER || base == TYPE_TEXT || base == TYPE_YESNO) {
        sb_append(&sb, plural[base]);
        return sb.data;
    }
    const RecordKind *kind = record_kind(records, base);
    if (kind) {
        sb_appendf(&sb, "%s%s", kind->plural, type_is_maybe(base) ? " or nothing" : "");
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

const char *ast_type_name(Arena *arena, const Records *records, Type type) {
    if (type_is_list(type)) return list_name(arena, records, type);
    if (type_is_map(type)) return map_name(arena, records, type);
    if (type == TYPE_MAYBE) return "a record or nothing";
    const RecordKind *kind = record_kind(records, type);
    if (kind) {
        return arena_sprintf(arena, "%s %s%s", article(kind->display), kind->display,
                             type_is_maybe(type) ? " or nothing" : "");
    }
    switch (type) {
    case TYPE_NUMBER: return "a number";
    case TYPE_TEXT: return "text";
    case TYPE_YESNO: return "a yes/no value";
    case TYPE_NOTHING: return "nothing";
    default: return "an unknown kind of value";
    }
}

