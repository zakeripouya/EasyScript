// Sentences of the standard library. "wait", "shuffle", "save" and "put"
// start sentences (parse_stmt.c's table); the others only mean something
// with the words after them, so "rename", "copy", "make" and "delete" can
// still be names of functions:
//
//   wait 2 seconds (or a second, 500 milliseconds, 1 minute)
//   use the same random numbers every time.      shuffle cards.
//   clear the screen.
//   make a new file called "notes.txt".          make a folder called "Photos".
//   rename the file "a.txt" to "b.txt".          (or the folder)
//   copy the file "a.txt" to "b.txt".
//   move the file "a.txt" to the folder "Old".   (or the folder)
//   delete the file "a.txt".                     delete the folder "Old" [and everything in it].
//   read csv file "sales.csv" and call it rows.  save rows as csv file "out.csv".
//   put "hello" in file "notes.txt".             (like "write ... to file")

#include <string.h>
#include "front/parse_internal.h"

static Stmt *new_action(Parser *p, LibraryAction op, const Token *verb) {
    Stmt *stmt = parser_new_stmt(p, STMT_LIBRARY, verb);
    stmt->as.library.op = op;
    stmt->as.library.scale = 1;
    return stmt;
}

// A value that has to come next (a path, a list): reports what's missing before it.
static Expr *expect_value(Parser *p, const char *example) {
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "Write it like \"%s\".", example);
        }
        return NULL;
    }
    Expr *value = parse_unary(p);
    return p->sentence_failed ? NULL : value;
}

static bool expect(Parser *p, const char *words, const char *display, const char *example) {
    return parser_expect_phrase(p, words, display, example);
}

bool parser_at_library_sentence(const Parser *p) {
    static const char *const starts[] = {
        "make new file", "make new folder", "make folder", "rename file", "rename folder", "copy file",
        "move file", "move folder", "delete file", "delete folder", "clear screen", "read csv file",
        "use same random numbers",
    };
    for (size_t i = 0; i < sizeof starts / sizeof starts[0]; i++) {
        if (parser_match_words(p, starts[i])) return true;
    }
    return false;
}

static Stmt *parse_make(Parser *p, const Token *verb) {
    if (parser_at_word(p, 0, "new")) parser_advance(p);
    bool folder = parser_at_word(p, 0, "folder");
    parser_advance(p);
    const char *example = folder ? "make a folder called \"Photos\"" : "make a new file called \"notes.txt\"";
    if (!expect(p, "called", "called", example)) return NULL;
    Stmt *stmt = new_action(p, folder ? DO_MAKE_FOLDER : DO_MAKE_FILE, verb);
    stmt->as.library.a = expect_value(p, example);
    return stmt->as.library.a ? stmt : NULL;
}

// rename/copy/move the file (or folder) A to [the folder] B
static Stmt *parse_two_paths(Parser *p, const Token *verb, LibraryAction op) {
    bool folder = parser_at_word(p, 0, "folder");
    parser_advance(p);
    static const char *const examples[] = {"rename the file \"a.txt\" to \"b.txt\"", "copy the file \"a.txt\" to "
                                           "\"b.txt\"", "move the file \"a.txt\" to the folder \"Old\""};
    const char *example = examples[op == DO_RENAME ? 0 : op == DO_COPY ? 1 : 2];
    Stmt *stmt = new_action(p, op, verb);
    stmt->as.library.folder = folder;
    if (!(stmt->as.library.a = expect_value(p, example))) return NULL;
    if (!expect(p, op == DO_MOVE ? "to folder" : "to", op == DO_MOVE ? "to the folder" : "to", example)) return NULL;
    stmt->as.library.b = expect_value(p, example);
    return stmt->as.library.b ? stmt : NULL;
}

static Stmt *parse_delete(Parser *p, const Token *verb) {
    bool folder = parser_at_word(p, 0, "folder");
    parser_advance(p);
    const char *example = folder ? "delete the folder \"Old\"" : "delete the file \"old.txt\"";
    Stmt *stmt = new_action(p, folder ? DO_DELETE_FOLDER : DO_DELETE_FILE, verb);
    if (!(stmt->as.library.a = expect_value(p, example))) return NULL;
    size_t n = parser_match_words(p, "and everything in it");
    if (n && folder) {
        parser_consume(p, n);
        stmt->as.library.everything = true;
    } else if (n) {
        Span span = parser_consume(p, n);
        if (parser_error(p, span, "Only a folder has everything in it.")) {
            diag_note(p->diag, "To delete a folder and what's in it, write \"delete the folder \"Old\" and everything "
                               "in it\".");
        }
        return NULL;
    }
    return stmt;
}

static Stmt *parse_read_csv(Parser *p, const Token *verb) {
    static const char *const example = "read csv file \"sales.csv\" and call it rows";
    parser_consume(p, 2);  // csv file
    Stmt *stmt = new_action(p, DO_READ_CSV, verb);
    if (!(stmt->as.library.a = expect_value(p, example))) return NULL;
    size_t n = parser_match_words(p, "and call it");
    if (!n) n = parser_match_words(p, "and call answer");
    if (!n) {
        expect(p, "and call it", "and call it", example);
        return NULL;
    }
    parser_consume(p, n);
    return parser_parse_name(p, &stmt->as.library.name, NULL) ? stmt : NULL;
}

Stmt *parser_parse_library_sentence(Parser *p) {
    const Token *verb = parser_advance(p);
    p->ends_with_name = false;
    const char *word = verb->text;
    if (strcmp(word, "make") == 0) return parse_make(p, verb);
    if (strcmp(word, "rename") == 0) return parse_two_paths(p, verb, DO_RENAME);
    if (strcmp(word, "copy") == 0) return parse_two_paths(p, verb, DO_COPY);
    if (strcmp(word, "move") == 0) return parse_two_paths(p, verb, DO_MOVE);
    if (strcmp(word, "delete") == 0) return parse_delete(p, verb);
    if (strcmp(word, "read") == 0) return parse_read_csv(p, verb);
    if (strcmp(word, "clear") == 0) {
        parser_advance(p);
        return new_action(p, DO_CLEAR_SCREEN, verb);
    }
    // use the same random numbers every time
    parser_consume(p, 3);
    if (!expect(p, "every time", "every time", "use the same random numbers every time")) return NULL;
    return new_action(p, DO_SAME_RANDOM, verb);
}

Stmt *parser_parse_wait(Parser *p, const Token *verb) {
    static const struct {
        const char *word;
        double scale;
    } units[] = {{"seconds", 1}, {"second", 1}, {"milliseconds", 0.001}, {"millisecond", 0.001},
                 {"minutes", 60}, {"minute", 60}};
    static const char *const example = "wait 2 seconds";
    Stmt *stmt = new_action(p, DO_WAIT, verb);
    for (size_t i = 0; i < sizeof units / sizeof units[0]; i++) {  // "wait a second": one
        if (!parser_at_word(p, 0, units[i].word)) continue;
        const Token *unit = parser_advance(p);
        Expr *one = ast_new_expr(p->arena, EXPR_NUMBER, parser_token_pos(unit));
        one->as.number.text = "1";
        stmt->as.library.a = one;
        stmt->as.library.scale = units[i].scale;
        return stmt;
    }
    if (!(stmt->as.library.a = expect_value(p, example))) return NULL;
    for (size_t i = 0; i < sizeof units / sizeof units[0]; i++) {
        if (!parser_at_word(p, 0, units[i].word)) continue;
        parser_advance(p);
        stmt->as.library.scale = units[i].scale;
        return stmt;
    }
    const Token *token = parser_peek(p, 0);
    bool reported = parser_at_sentence_end(p)
                        ? parser_error(p, p->prev_span, "Say how long to wait: seconds, milliseconds or minutes.")
                        : parser_error(p, token->span, "I expected \"seconds\", \"milliseconds\" or \"minutes\" here, but "
                                                       "found %s.", parser_describe(p, token));
    if (reported) diag_note(p->diag, "Write it like \"%s\".", example);
    return NULL;
}

Stmt *parser_parse_shuffle(Parser *p, const Token *verb) {
    Stmt *stmt = new_action(p, DO_SHUFFLE, verb);
    stmt->as.library.a = expect_value(p, "shuffle cards");
    return stmt->as.library.a ? stmt : NULL;
}

Stmt *parser_parse_save(Parser *p, const Token *verb) {
    static const char *const example = "save rows as csv file \"out.csv\"";
    Stmt *stmt = new_action(p, DO_SAVE_CSV, verb);
    p->as_ends_value = true;  // "save rows as csv": "as" isn't a conversion here
    stmt->as.library.a = expect_value(p, example);
    p->as_ends_value = false;
    if (!stmt->as.library.a || !expect(p, "as csv file", "as csv file", example)) return NULL;
    stmt->as.library.b = expect_value(p, example);
    return stmt->as.library.b ? stmt : NULL;
}

Stmt *parser_parse_put(Parser *p, const Token *verb) {
    static const char *const example = "put \"hello\" in file \"notes.txt\"";
    Stmt *stmt = parser_new_stmt(p, STMT_WRITE_FILE, verb);
    if (parser_match_words(p, "in file")) {  // "put in file ...": the value is missing
        if (parser_error(p, verb->span, "Something is missing after \"put\".")) {
            diag_note(p->diag, "Write it like \"%s\".", example);
        }
        return NULL;
    }
    stmt->as.file_write.text = parse_expression(p);
    if (p->sentence_failed || !expect(p, "in file", "in file", example)) return NULL;
    stmt->as.file_write.path = parse_expression(p);
    return stmt;
}
