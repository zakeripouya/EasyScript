#include <string.h>
#include "back/source_table.h"
#include "back/codegen_internal.h"

void source_table_c(Arena *arena, const char *source, size_t len, const char *const *names, const size_t *starts,
                    size_t files, StrBuf *out) {
    (void)arena;
    size_t *first = arena_alloc(arena, (files ? files : 1) * sizeof(size_t));  // each file's first line
    sb_append(out, "static const char *const es_source[] = {\n");
    size_t lines = 0;
    size_t start = 0;
    while (start < len || (start == len && lines == 0)) {
        for (size_t f = 0; f < files; f++) {
            if (starts[f] == start) first[f] = lines;
        }
        size_t end = start;
        while (end < len && source[end] != '\n') end++;
        size_t visible = end;
        if (visible > start && source[visible - 1] == '\r') visible--;  // the line without its "\r\n"
        sb_append(out, "    ");
        cg_append_c_string(out, source + start, visible - start);
        sb_append(out, ",\n");
        lines++;
        start = end + 1;
    }
    sb_appendf(out, "};\nstatic const int es_source_line_count = %zu;\n", lines);
    sb_append(out, "static const EsSourceFile es_source_files[] = {");
    for (size_t f = 0; f < files; f++) {
        sb_append(out, f ? ", {" : "{");
        if (files > 1 && names[f]) {  // NULL: a name not shown
            cg_append_c_string(out, names[f], strlen(names[f]));
        } else {
            sb_append(out, "NULL");
        }
        sb_appendf(out, ", %zu}", files > 1 ? first[f] : 0);
    }
    sb_append(out, "};\n\n");
}
