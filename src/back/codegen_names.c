// C names: functions' and variables' C names, the C type of each kind, the
// runtime's retain/release/set/drop functions per counted kind, and C string
// literals.

#include <assert.h>
#include "back/codegen_internal.h"

// A C name: the prefix plus the name with "_" doubled, "'" as "_q", and the
// space between the words of a name as one "_", so different EasyScript
// names can never collide ("late fee" is late_fee, "late_fee" late__fee).
// A function's C name with a prefix: "es_f_speak", and "es_f_speak_0r1" for
// a method sharing its name with another kind's ("_0" can't come from a
// name, whose words start with a letter).
void cg_append_function(StrBuf *out, const char *prefix, const Stmt *fn) {
    cg_append_prefixed(out, prefix, fn->as.function.name.text);
    if (fn->as.function.overload) sb_appendf(out, "_0r%zu", fn->as.function.overload - 1);
}

void cg_append_prefixed(StrBuf *out, const char *prefix, const char *name) {
    sb_append(out, prefix);
    for (const char *c = name; *c; c++) {
        if (*c == ' ') {
            sb_append_char(out, '_');
        } else if (*c == '_') {
            sb_append(out, "__");
        } else if (*c == '\'') {
            sb_append(out, "_q");
        } else {
            sb_append_char(out, *c);
        }
    }
}

const char *cg_ctype(Type type) {
    if (type_is_list(type)) return "EsList *";
    if (type_is_map(type)) return "EsMap *";
    if (type_is_object(type)) return "EsRecord *";
    switch (type) {
    case TYPE_NUMBER:
    case TYPE_DATE: return "double";
    case TYPE_YESNO: return "bool";
    case TYPE_TEXT: return "EsText *";
    case TYPE_NOTHING: return "void";
    default: assert(!"a value whose kind wasn't decided reached codegen"); return "void";
    }
}

// A variable: "es_vn_total" (number), "es_vt_name" (text), "es_vb_done"
// (yes/no), "es_vl1n_scores" (a list of numbers), "es_vl2t_rows" (a list of
// lists of text), "es_vr0_rex" (record kind 0), "es_vm0_next" (kind 0 or
// nothing), "es_vl1r0_dogs" (a list of kind 0), "es_vh2_ages" (kind of map 2),
// "es_vk1_s" (kind with variants 1), "es_vq1_s" (kind 1 or nothing),
// "es_von_p" (a number or nothing; "ot", "ob" likewise), "es_vf3_double"
// (kind of function 3), "es_vi0_pet" (interface 0; "es_vj0_" or nothing).
void cg_append_raw_var(StrBuf *out, const char *name, Type type) {
    Type base = type_base(type), present = type_present(base);
    bool maybe = type_is_maybe(base);
    char kind = present == TYPE_TEXT ? 't' : present == TYPE_YESNO ? 'b' : present == TYPE_DATE ? 'd' : 'n';
    if (type_is_list(type)) sb_appendf(out, "es_vl%u", type / TYPE_LIST);
    sb_append(out, type_is_list(type) ? "" : "es_v");
    if (type_is_record(base)) {
        sb_appendf(out, "%c%zu", maybe ? 'm' : 'r', type_record_index(base));
    } else if (type_is_sum(base)) {
        sb_appendf(out, "%c%zu", maybe ? 'q' : 'k', type_sum_index(base));
    } else if (type_is_map(base)) {
        sb_appendf(out, "h%u", base - TYPE_MAP);
    } else if (type_is_func(base)) {
        sb_appendf(out, "f%u", base - TYPE_FUNC);
    } else if (type_is_iface(base)) {
        sb_appendf(out, "%c%zu", maybe ? 'j' : 'i', type_iface_index(base));
    } else {
        sb_appendf(out, "%s%c", maybe ? "o" : "", kind);
    }
    cg_append_prefixed(out, "_", name);
}

const char *cg_retain(Type type) {
    if (type_is_map(type)) return "es_map_retain";
    return type_is_list(type) ? "es_list_retain" : type_is_object(type) ? "es_record_retain" : "es_retain";
}

const char *cg_release(Type type) {
    if (type_is_map(type)) return "es_map_release";
    return type_is_list(type) ? "es_list_release" : type_is_object(type) ? "es_record_release" : "es_release";
}

const char *cg_set(Type type) {
    if (type_is_map(type)) return "es_map_set";
    return type_is_list(type) ? "es_list_set" : type_is_object(type) ? "es_record_set" : "es_set";
}

const char *cg_drop(Type type) {
    if (type_is_map(type)) return "es_map_drop";
    return type_is_list(type) ? "es_list_drop" : type_is_object(type) ? "es_record_drop" : "es_drop";
}

const char *cg_empty(Type type) {
    if (type_is_list(type)) return "&es_no_list";
    if (type_is_map(type)) return "&es_no_map";
    if (type_is_object(type)) return "NULL";
    return type == TYPE_TEXT ? "&es_empty_text" : type == TYPE_YESNO ? "false" : "0";
}

// A C string literal. Non-ASCII and control bytes use three-digit octal
// escapes; "?" is escaped so "??" can never form a trigraph.
void cg_append_c_string(StrBuf *out, const char *s, size_t len) {
    sb_append_char(out, '"');
    for (size_t i = 0; i < len; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '"' || c == '\\' || c == '?') {
            sb_append_char(out, '\\');
            sb_append_char(out, (char)c);
        } else if (c == '\n') {
            sb_append(out, "\\n");
        } else if (c == '\t') {
            sb_append(out, "\\t");
        } else if (c >= 0x20 && c < 0x7F) {
            sb_append_char(out, (char)c);
        } else {
            sb_appendf(out, "\\%03o", c);
        }
    }
    sb_append_char(out, '"');
}
