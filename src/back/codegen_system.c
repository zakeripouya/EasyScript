// The system tools: calls into runtime/es_keys.h and es_system.h.

#include "back/codegen_internal.h"

static const char *code_of(Codegen *g, const Expr *expr, bool text) {
    StrBuf sb;
    sb_init(&sb, g->arena);
    if (text) {
        cg_emit_text(g, expr, &sb);
    } else {
        cg_emit_expr(g, expr, &sb);
    }
    return sb.data;
}

// "R worked": R's code (its third field) is 0.
static void emit_worked(Codegen *g, const Expr *expr, StrBuf *out) {
    Expr *code = ast_new_expr(g->arena, EXPR_FIELD, expr->pos);
    code->as.field.record = expr->as.library.a;
    code->as.field.index = 2;
    code->type = TYPE_NUMBER;
    sb_append(out, "es_worked(");
    cg_emit_field(g, code, out);
    sb_append(out, expr->as.library.part ? ", false)" : ", true)");
}

void cg_emit_system(Codegen *g, const Expr *expr, StrBuf *out) {
    const Expr *a = expr->as.library.a;
    switch (expr->as.library.op) {
    case LIB_PROGRAM_WORDS: sb_append(out, "es_program_words()"); break;
    case LIB_PROGRAM_COUNT: sb_append(out, "((double)es_word_count())"); break;
    case LIB_PROGRAM_WORD:
        sb_appendf(out, "es_program_word(%zu, %s)", cg_site(g, expr->pos), code_of(g, a, false));
        break;
    case LIB_SENT_TEXT: sb_append(out, "es_sent_text()"); break;
    case LIB_SENT_LINES: sb_append(out, "es_text_lines(es_sent_text())"); break;
    case LIB_SETTING: sb_appendf(out, "es_setting(%s)", code_of(g, a, false)); break;
    case LIB_KEY_PRESSED: sb_append(out, "es_key_pressed()"); break;
    case LIB_WORKED: emit_worked(g, expr, out); break;
    default: break;
    }
}

// run P with WORDS [and call the result NAME]: the words are worked out in
// order into a list, then the program runs.
static void emit_run(Codegen *g, const Stmt *stmt) {
    size_t site = cg_site(g, stmt->pos);
    cg_indent(g);
    sb_append(&g->body, "{\n");
    g->depth++;
    cg_indent(g);
    sb_appendf(&g->body, "EsText *es_program = %s;\n", code_of(g, stmt->as.library.a, false));
    cg_indent(g);
    sb_appendf(&g->body, "EsList *es_words = es_list_new(ES_ITEMS_TEXT, %zu);\n", stmt->as.library.parts.len);
    for (size_t i = 0; i < stmt->as.library.parts.len; i++) {
        const Expr *part = stmt->as.library.parts.items[i];
        cg_indent(g);
        if (type_is_list(part->type)) {
            sb_appendf(&g->body, "es_command_add_list(es_words, %s);\n", code_of(g, part, false));
        } else {
            sb_appendf(&g->body, "es_push_t(es_words, %s);\n", code_of(g, part, true));
        }
    }
    cg_indent(g);
    const Name *name = &stmt->as.library.name;
    if (!name->text) {
        sb_appendf(&g->body, "es_run(%zu, es_command(es_program, es_words));\n", site);
    } else {
        size_t kind = g->records->program_result - 1;
        Type type = type_record(kind);
        sb_append(&g->body, "EsText *es_out, *es_err;\n");
        cg_indent(g);
        sb_appendf(&g->body, "double es_code = es_spawn(%zu, es_command(es_program, es_words), &es_out, &es_err);\n",
                   site);
        cg_declare_name(g, name->text, type);
        cg_emit_new_cell(g, name->text, type);
        cg_emit_store(g, name->text, type, arena_sprintf(g->arena, "es_new%zu(es_out, es_err, es_code)", kind));
    }
    g->depth--;
    cg_indent(g);
    sb_append(&g->body, "}\n");
}

void cg_emit_system_stmt(Codegen *g, const Stmt *stmt) {
    const Name *name = &stmt->as.library.name;
    switch (stmt->as.library.op) {
    case DO_COMPLAIN:
        cg_indent(g);
        sb_appendf(&g->body, "es_complain(%s);\n", code_of(g, stmt->as.library.a, true));
        break;
    case DO_RUN: emit_run(g, stmt); break;
    case DO_WAIT_KEY:
        cg_declare_name(g, name->text, TYPE_TEXT);
        cg_emit_new_cell(g, name->text, TYPE_TEXT);
        cg_emit_store(g, name->text, TYPE_TEXT, arena_sprintf(g->arena, "es_wait_key(%zu)", cg_site(g, stmt->pos)));
        break;
    default: break;
    }
}

// "stop the program with code N": the code, then the usual stop.
void cg_emit_stop_code(Codegen *g, const Stmt *stmt) {
    if (!stmt->as.stop_code) return;
    cg_indent(g);
    sb_appendf(&g->body, "es_set_status(%zu, %s);\n", cg_site(g, stmt->as.stop_code->pos),
               code_of(g, stmt->as.stop_code, false));
}
