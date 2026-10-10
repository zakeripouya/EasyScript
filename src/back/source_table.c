#include "back/source_table.h"
#include "back/codegen_internal.h"

void source_table_c(Arena *arena, const char *source, size_t len, StrBuf *out) {
    (void)arena;
    sb_append(out, "static const char *const es_source[] = {\n");
    size_t lines = 0;
    size_t start = 0;
    while (start < len || (start == len && lines == 0)) {
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
    sb_appendf(out, "};\nstatic const int es_source_line_count = %zu;\n\n", lines);
}
