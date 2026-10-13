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

// The one-word name after "and call it" / "called" (token index), or 0.
static size_t table_name_at(const Parser *p, size_t i) {
    static const char *const calls[] = {"and call it", ", called", "called"};
    for (size_t c = 0; c < 3; c++) {
        size_t at = i;
        bool match = true;
        for (const char *w = calls[c]; *w && match;) {
            const char *space = strchr(w, ' ');
            size_t len = space ? (size_t)(space - w) : strlen(w);
            const Token *token = at < p->count ? &p->tokens[at] : NULL;
            match = token && (len == 1 && *w == ',' ? token->kind == TOK_COMMA
                                                    : token->kind == TOK_WORD && strlen(token->text) == len &&
                                                          strncmp(token->text, w, len) == 0);
            at++;
            w += space ? len + 1 : len;
        }
        if (match && at < p->count && p->tokens[at].kind == TOK_WORD &&
            (at + 1 >= p->count || p->tokens[at + 1].kind != TOK_WORD)) {
            return at;
        }
    }
    return 0;
}

// "purchases" -> "purchase": the kind of row of a table called with a plural
// name (whose plural is that name again), unless that's already a kind. NULL
// otherwise.
static bool ends_with(const char *word, size_t len, const char *end) {
    size_t n = strlen(end);
    return len > n && strcmp(word + len - n, end) == 0;
}

const char *parser_singular_of(Arena *arena, const char *plural) {
    size_t len = strlen(plural);
    const char *one = NULL;
    if (ends_with(plural, len, "ies")) {
        one = arena_sprintf(arena, "%.*sy", (int)(len - 3), plural);
    } else if (ends_with(plural, len, "xes") || ends_with(plural, len, "ches") || ends_with(plural, len, "shes") ||
               ends_with(plural, len, "sses") || ends_with(plural, len, "zzes")) {
        one = arena_sprintf(arena, "%.*s", (int)(len - 2), plural);
    } else if (ends_with(plural, len, "s")) {
        one = arena_sprintf(arena, "%.*s", (int)(len - 1), plural);
    }
    return one && strcmp(parser_plural_of(arena, one), plural) == 0 ? one : NULL;
}

static const char *row_kind_name(Parser *p, const char *table) {
    const char *row = parser_singular_of(p->arena, table);
    if (!row) return NULL;
    for (size_t k = 0; k < p->records->len; k++) {
        if (strcmp(p->records->items[k].name, row) == 0) return NULL;
    }
    return row;
}

// The first text the program writes into the file `name` ("put the text
// below in file "bank.csv":", "write "a,b" to file "bank.csv"."), or NULL.
static const char *written_to(const Parser *p, const char *name) {
    for (size_t i = 0; i + 4 < p->count; i++) {
        const Token *t = &p->tokens[i];
        if (t->kind != TOK_WORD || (strcmp(t->text, "put") != 0 && strcmp(t->text, "write") != 0)) continue;
        if (t[1].kind != TOK_STRING || t[2].kind != TOK_WORD || t[3].kind != TOK_WORD || t[4].kind != TOK_STRING) continue;
        if (strcmp(t[2].text, "in") != 0 && strcmp(t[2].text, "to") != 0) continue;
        if (strcmp(t[3].text, "file") == 0 && strcmp(t[4].text, name) == 0 &&
            (i + 5 >= p->count || t[5].kind != TOK_LBRACKET)) {
            return t[1].text;
        }
    }
    return NULL;
}

void parser_find_table_files(Parser *p) {
    for (size_t i = 0; i < p->count; i++) {
        if (!at_read_table(p, i)) continue;
        const Token *file = &p->tokens[i + 3];
        const char *shown = file->kind == TOK_STRING ? file->text : "the table";
        size_t index = p->records->len;
        RecordKind kind = {arena_sprintf(p->arena, "0row%zu", index), arena_sprintf(p->arena, "row of %s", shown), NULL,
                           NULL, true, NULL, NULL, 0, 0};
        kind.plural = arena_sprintf(p->arena, "rows of %s", shown);
        if (file->kind == TOK_STRING && (i + 4 >= p->count || p->tokens[i + 4].kind != TOK_LBRACKET)) {
            kind.file = file->text;  // the checker looks at it (or at what the program writes into it)
            kind.written = written_to(p, file->text);
            kind.file_offset = file->span.offset;
            kind.file_length = file->span.length;
        }
        size_t called = 0;  // "... and call it purchases": a kind called "purchase" (maybe after ", with ...")
        for (size_t j = i + 4; !called && j < p->count && p->tokens[j].kind != TOK_PERIOD && p->tokens[j].kind != TOK_NEWLINE; j++) {
            called = table_name_at(p, j);
        }
        const char *row = called ? row_kind_name(p, p->tokens[called].text) : NULL;
        if (row) {
            kind.name = kind.display = row;
            kind.plural = p->tokens[called].text;
        }
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

// ", with amount in dollars and quantity as a number": each column's kind
// written down (in dollars, euros or pounds, or as a number). False after an
// error.
static bool parse_column_kinds(Parser *p, Stmt *stmt) {
    static const char *const example = "read the table in \"bank.csv\", with amount in dollars and quantity as a number";
    static const char *const next[] = {"in", "as", NULL};
    static const struct {
        const char *words;
        Type type;
    } kinds[] = {{"in dollars", TYPE_DOLLARS}, {"in euros", TYPE_EUROS}, {"in pounds", TYPE_POUNDS}, {"as number", TYPE_NUMBER}};
    if (!(parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "with"))) return true;
    parser_consume(p, 2);
    for (;;) {
        ColumnKind column = {{0}, TYPE_UNKNOWN};
        if (!parser_parse_name(p, &column.name, next)) return false;
        size_t n = 0;
        for (size_t k = 0; k < sizeof kinds / sizeof kinds[0] && !n; k++) {
            if ((n = parser_match_words(p, kinds[k].words))) column.type = kinds[k].type;
        }
        if (!n) {
            const Token *token = parser_peek(p, 0);
            if (parser_error(p, token->span, "I expected \"in dollars\", \"in euros\", \"in pounds\" or \"as a number\" "
                             "here, but found %s.", parser_describe(p, token))) {
                diag_note(p->diag, "Write it like \"%s\".", example);
                diag_info(p->diag, DIAG_MISSING, "the column's kind", NULL);
            }
            return false;
        }
        parser_consume(p, n);
        if (column.type != TYPE_NUMBER) p->records->money = true;
        vec_push(p->arena, &stmt->as.library.columns, column);
        if (parser_at_step_link(p)) return true;  // ", then total the amount per month"
        if (parser_at(p, 0, TOK_COMMA) && parser_match_words_at(p, 1, "and call it")) {  // ", and call it purchases"
            parser_advance(p);
            return true;
        }
        bool more = parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call");
        more = more || (parser_at(p, 0, TOK_COMMA) && !parser_at_word(p, 1, "called"));
        if (!more) return true;
        parser_advance(p);
    }
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
    if (p->sentence_failed || !parse_column_kinds(p, stmt)) return NULL;
    size_t n = parser_match_words(p, "and call it");
    if (!n) n = parser_match_words(p, "called") ? 1 : 0;
    if (!n && parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "called")) n = 2;
    if (n) {
        parser_consume(p, n);
        if (!parser_parse_name(p, &stmt->as.library.name, NULL)) return NULL;
        if (!stmt->as.library.columns.len && !parse_column_kinds(p, stmt)) return NULL;
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
