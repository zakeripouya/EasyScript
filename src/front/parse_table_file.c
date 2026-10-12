// "read the table in "sales.csv" [and call it sales]": the rows of a csv
// file, as records of a kind made for this sentence ("a row of sales.csv").
// The kind's fields are the columns the program uses ("its amount", "the total
// amount of sales", "per region"), with kinds worked out from that use; reading
// checks each of them is a column of the file. In a program that reads a
// table, a word where a field goes can be a column even if no kind names it.

#include <string.h>
#include "front/parse_internal.h"

// "read table in F" (the "the" is dropped): only ever this sentence.
static bool at_read_table(const Parser *p, size_t i) {
    return i + 3 < p->count && p->tokens[i].kind == TOK_WORD && strcmp(p->tokens[i].text, "read") == 0 &&
           p->tokens[i + 1].kind == TOK_WORD && strcmp(p->tokens[i + 1].text, "table") == 0 &&
           p->tokens[i + 2].kind == TOK_WORD && strcmp(p->tokens[i + 2].text, "in") == 0;
}

void parser_find_table_files(Parser *p) {
    for (size_t i = 0; i < p->count; i++) {
        if (!at_read_table(p, i)) continue;
        const Token *file = &p->tokens[i + 3];
        const char *shown = file->kind == TOK_STRING ? file->text : "the table";
        size_t index = p->records->len;
        RecordKind kind = {arena_sprintf(p->arena, "0row%zu", index), arena_sprintf(p->arena, "row of %s", shown), NULL,
                           NULL, true};
        kind.plural = arena_sprintf(p->arena, "rows of %s", shown);
        Stmt *definition = ast_new_stmt(p->arena, STMT_RECORD, parser_token_pos(&p->tokens[i]));
        definition->as.record.name.text = kind.name;
        definition->as.record.name.display = kind.display;
        definition->as.record.index = index;
        static const char *const hidden[] = {"0header", "0cells"};  // the file's header and each row's cells
        for (size_t f = 0; f < 2; f++) {
            Field field = {{hidden[f], hidden[f], definition->pos}, type_list_of(TYPE_TEXT), NULL, definition->pos, {0}, {0}};
            vec_push(p->arena, &definition->as.record.fields, field);
        }
        kind.definition = definition;
        vec_push(p->arena, p->records, kind);
        vec_push(p->arena, &p->tables, index);
    }
}

size_t parser_field_words(const Parser *p, size_t ahead) {
    size_t known = parser_known_field_words(p, ahead);
    if (known || p->tables.len == 0) return known;
    static const char *const not_columns[] = {"of", "per", "in", "by", "to", "from", "with", "at", "is", "are", "that",
                                              "whose", "where", "then", "and", "or"};
    const Token *token = parser_peek(p, ahead);
    if (!parser_is_name_word(token)) return 0;
    for (size_t i = 0; i < sizeof not_columns / sizeof not_columns[0]; i++) {
        if (strcmp(token->text, not_columns[i]) == 0) return 0;
    }
    return 1;
}

// After "read", at "table": "table in F [and call it NAME | , called NAME]".
Stmt *parser_parse_read_table(Parser *p, const Token *verb) {
    static const char *const example = "read the table in \"sales.csv\" and call it sales";
    parser_consume(p, 2);  // table in
    Stmt *stmt = parser_new_stmt(p, STMT_LIBRARY, verb);
    stmt->as.library.op = DO_READ_TABLE;
    size_t kind = p->tables.items[p->tables_read++];
    stmt->as.library.type = type_list_of(type_record(kind));
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after \"in\".")) {
            diag_note(p->diag, "Say which file, like \"%s\".", example);
            diag_info(p->diag, DIAG_MISSING, "the file's name", "text");
        }
        return NULL;
    }
    stmt->as.library.a = parse_primary(p);
    if (p->sentence_failed) return NULL;
    size_t n = parser_match_words(p, "and call it");
    if (!n) n = parser_match_words(p, "called") ? 1 : 0;
    if (!n && parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "called")) n = 2;
    if (n) {
        parser_consume(p, n);
        if (!parser_parse_name(p, &stmt->as.library.name, NULL)) return NULL;
    } else {  // for the next step's "it"
        stmt->as.library.name.text = arena_sprintf(p->arena, "0step%zu", ++p->hidden_lists);
        stmt->as.library.name.display = "the table";
        stmt->as.library.name.pos = stmt->as.library.a->pos;
    }
    return stmt;
}

// --- try to ... --------------------------------------------------------------------------

// try to S. if that fails, S2. / ... if that fails: (indented sentences)
Stmt *parser_parse_try(Parser *p) {
    static const char *const example = "try to read file \"notes.txt\" and call it notes. if that fails, say \"No notes yet.\"";
    const Token *verb = parser_advance(p);
    parser_advance(p);  // to
    Stmt *stmt = parser_new_stmt(p, STMT_TRY, verb);
    stmt->as.attempt.attempt = parse_statement(p);
    if (!stmt->as.attempt.attempt || p->sentence_failed) return NULL;
    size_t gap = 0;  // the sentence's end, then "if that fails" (on the same line or the next)
    if (parser_at(p, gap, TOK_PERIOD) || parser_at(p, gap, TOK_COMMA)) gap++;
    if (parser_at(p, gap, TOK_NEWLINE)) gap++;
    if (!parser_match_words_at(p, gap, "if that fails")) {
        if (parser_error(p, verb->span, "Say what to do if it fails.")) {
            diag_note(p->diag, "Write it like \"%s\".", example);
            diag_info(p->diag, DIAG_MISSING, "\"if that fails\"", NULL);
        }
        return NULL;
    }
    parser_consume(p, gap + 3);
    bool outer = p->in_failure;
    p->in_failure = true;  // "the reason" means why it failed
    if (parser_at(p, 0, TOK_COMMA)) {
        parser_advance(p);
        parser_parse_actions(p, &stmt->as.attempt.fallback);
    } else {
        SourcePos header = stmt->pos;
        parser_parse_block(p, &stmt->as.attempt.fallback, &header, NULL, "\"if that fails\"");
        p->ended_with_block = true;
    }
    p->in_failure = outer;
    return p->sentence_failed ? NULL : stmt;
}
