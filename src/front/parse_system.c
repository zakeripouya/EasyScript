// System tools: what the program is given when it starts, other programs,
// finishing, settings and the keyboard. Values and tests:
//
//   the words given to the program     the number of words given to the program
//   word N given to the program        the text sent to the program
//   the lines sent to the program      the setting called "HOME"
//   the key being pressed              R worked / R didn't work / R did not work
//
// Sentences:
//
//   run "git" with "status" and "--short" [and call the result r]   (or "and call it r")
//   complain "That file is missing."
//   stop the program, saying it failed.     stop the program with code 3.
//   wait for a key and call it key.         (parse_library_stmt.c's "wait" sends it here)
//
// "run" and "complain" start sentences only where the program hasn't
// defined a function with that name, like the file sentences.

#include <string.h>
#include "front/parse_internal.h"

static Expr *new_value(Parser *p, LibraryOp op, SourcePos start, Span op_span, Expr *a) {
    Expr *expr = ast_new_expr(p->arena, EXPR_LIBRARY, start);
    expr->as.library.op = op;
    expr->as.library.op_span = op_span;
    expr->as.library.a = a;
    expr->pos = ast_pos_join(start, a ? a->pos : (SourcePos){op_span, 0, 0, 0});
    return expr;
}

// Words alone: "the words given to the program", "the key being pressed".
static Expr *words_only(Parser *p, LibraryOp op, size_t n) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span span = parser_consume(p, n);
    return new_value(p, op, start, span, NULL);
}

// "word" + a position + "given to the program": the position is a number, a
// name, text (a kind mistake the checker explains) or something in parentheses.
static bool at_program_word(const Parser *p) {
    if (!parser_at_word(p, 0, "word")) return false;
    const Token *next = parser_peek(p, 1);
    if (next->kind == TOK_NUMBER || next->kind == TOK_WORD || next->kind == TOK_STRING) return parser_match_words_at(p, 2, "given to program") > 0;
    if (next->kind != TOK_LPAREN) return false;
    size_t depth = 0, k = 1;
    for (;; k++) {
        TokenKind kind = parser_peek(p, k)->kind;
        if (kind == TOK_EOF || kind == TOK_NEWLINE) return false;
        if (kind == TOK_LPAREN) depth++;
        if (kind == TOK_RPAREN && --depth == 0) break;
    }
    return parser_match_words_at(p, k + 1, "given to program") > 0;
}

static Expr *parse_program_word(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    parser_advance(p);  // word
    bool no_call = p->no_call;
    p->no_call = true;  // "word n given ...": n is a position, never a call
    Expr *position = parse_primary(p);
    p->no_call = no_call;
    if (p->sentence_failed) return parser_error_expr(p);
    Span end = parser_consume(p, 3);  // given to program
    Expr *expr = new_value(p, LIB_PROGRAM_WORD, start, end, position);
    expr->pos = ast_pos_join(start, (SourcePos){end, 0, 0, 0});
    return expr;
}

// "the setting called X": X is a single value, like "the length of".
static Expr *parse_setting(Parser *p) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    Span op_span = parser_consume(p, 2);
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, op_span, "Something is missing after \"%s\".", parser_text(p, op_span))) {
            diag_note(p->diag, "Write the setting's name after it, like \"the setting called \"HOME\"\".");
        }
        return parser_error_expr(p);
    }
    Expr *name = parse_primary(p);
    return new_value(p, LIB_SETTING, start, op_span, name);
}

Expr *parser_parse_system_word(Parser *p) {
    static const struct {
        const char *words;
        LibraryOp op;
    } plain[] = {
        {"words given to program", LIB_PROGRAM_WORDS}, {"number of words given to program", LIB_PROGRAM_COUNT},
        {"text sent to program", LIB_SENT_TEXT},       {"lines sent to program", LIB_SENT_LINES},
        {"key being pressed", LIB_KEY_PRESSED},
    };
    for (size_t i = 0; i < sizeof plain / sizeof plain[0]; i++) {
        size_t n = parser_match_words(p, plain[i].words);
        if (n) return words_only(p, plain[i].op, n);
    }
    if (parser_match_words(p, "setting called")) return parse_setting(p);
    if (at_program_word(p)) return parse_program_word(p);
    if (parser_match_words(p, "word given to program")) {  // which word?
        if (parser_error(p, parser_peek(p, 0)->span, "Something is missing after \"word\".")) {
            diag_note(p->diag, "Say which word, like \"word 1 given to the program\".");
        }
        return parser_error_expr(p);
    }
    return NULL;
}

// "R worked", "R didn't work", "R did not work".
Expr *parser_parse_system_test(Parser *p, Expr *left) {
    size_t n = parser_match_words(p, "worked");
    bool negated = false;
    if (!n && (n = parser_match_words(p, "didn't work"))) negated = true;
    if (!n && (n = parser_match_words(p, "did not work"))) negated = true;
    if (!n) return NULL;
    Span op_span = parser_consume(p, n);
    Expr *expr = new_value(p, LIB_WORKED, left->pos, op_span, left);
    expr->as.library.part = negated;
    expr->pos = ast_pos_join(left->pos, (SourcePos){op_span, 0, 0, 0});
    return expr;
}

// --- Sentences ------------------------------------------------------------------

bool parser_at_system_sentence(const Parser *p) {
    const Token *token = parser_peek(p, 0);
    if (!parser_at_word(p, 0, "run") && !parser_at_word(p, 0, "complain")) return false;
    return !parser_is_function(p, token->text);
}

static Stmt *new_action(Parser *p, LibraryAction op, const Token *verb) {
    Stmt *stmt = parser_new_stmt(p, STMT_LIBRARY, verb);
    stmt->as.library.op = op;
    stmt->as.library.scale = 1;
    return stmt;
}

static bool missing_after(Parser *p, const char *example) {
    if (!parser_at_sentence_end(p)) return false;
    if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Write it like \"%s\".", example);
    }
    return true;
}

// "and call it NAME" or "and call the result NAME" (also "the answer"), then the name.
static bool parse_call_it(Parser *p, Stmt *stmt, const char *example) {
    static const char *const phrases[] = {"and call it", "and call result", "and call answer"};
    for (size_t i = 0; i < sizeof phrases / sizeof phrases[0]; i++) {
        size_t n = parser_match_words(p, phrases[i]);
        if (!n) continue;
        parser_consume(p, n);
        return parser_parse_name(p, &stmt->as.library.name, NULL);
    }
    parser_expect_phrase(p, "and call it", "and call it", example);
    return false;
}

// run P [with A and B ...] [and call the result NAME]
static Stmt *parse_run(Parser *p, const Token *verb) {
    static const char *const example = "run \"git\" with \"status\" and call the result r";
    Stmt *stmt = new_action(p, DO_RUN, verb);
    if (missing_after(p, example)) return NULL;
    bool no_call = p->no_call;
    p->no_call = true;  // "run tool with "x"": tool is the program, not a call of tool
    stmt->as.library.a = parse_unary(p);
    p->no_call = no_call;
    if (p->sentence_failed) return NULL;
    if (parser_at_word(p, 0, "with")) {
        parser_advance(p);
        for (;;) {
            if (missing_after(p, example)) return NULL;
            Expr *part = parse_unary(p);
            if (p->sentence_failed) return NULL;
            vec_push(p->arena, &stmt->as.library.parts, part);
            if (parser_at(p, 0, TOK_COMMA)) {
                parser_advance(p);
            } else if (parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call")) {
                parser_advance(p);
            } else {
                break;
            }
        }
    }
    if (parser_at_word(p, 0, "and") && !parse_call_it(p, stmt, example)) return NULL;
    return stmt;
}

static Stmt *parse_complain(Parser *p, const Token *verb) {
    Stmt *stmt = new_action(p, DO_COMPLAIN, verb);
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, verb->span, "Something is missing after \"complain\".")) {
            diag_note(p->diag, "Write what's wrong after it, like \"complain \"I need a file name.\"\".");
        }
        return NULL;
    }
    stmt->as.library.a = parse_expression(p);
    return p->sentence_failed ? NULL : stmt;
}

Stmt *parser_parse_system_sentence(Parser *p) {
    const Token *verb = parser_advance(p);
    p->ends_with_name = false;
    return strcmp(verb->text, "run") == 0 ? parse_run(p, verb) : parse_complain(p, verb);
}

// wait for a key and call it NAME (after "wait for key").
Stmt *parser_parse_wait_key(Parser *p, const Token *verb) {
    parser_consume(p, 2);
    Stmt *stmt = new_action(p, DO_WAIT_KEY, verb);
    return parse_call_it(p, stmt, "wait for a key and call it key") ? stmt : NULL;
}

// After "stop the program": ", saying it failed" or "with code N".
Stmt *parser_finish_stop(Parser *p, Stmt *stmt) {
    if (parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "saying")) parser_advance(p);
    if (parser_at_word(p, 0, "saying")) {
        static const char *const example = "stop the program, saying it failed";
        SourcePos at = parser_token_pos(parser_peek(p, 0));
        if (!parser_expect_phrase(p, "saying it failed", "saying it failed", example)) return NULL;
        Expr *one = ast_new_expr(p->arena, EXPR_NUMBER, ast_pos_join(at, (SourcePos){p->prev_span, 0, 0, 0}));
        one->as.number.text = "1";
        stmt->as.stop_code = one;
        return stmt;
    }
    size_t n = parser_match_words(p, "with code");
    if (!n) return stmt;
    parser_consume(p, n);
    if (missing_after(p, "stop the program with code 3")) return NULL;
    stmt->as.stop_code = parse_unary(p);
    return p->sentence_failed ? NULL : stmt;
}

// --- The result of running a program ---------------------------------------------

static void add_field(Parser *p, Stmt *definition, const char *name, size_t words, Type type) {
    Field field;
    memset(&field, 0, sizeof field);
    field.name.text = field.name.display = name;
    field.type = type;
    vec_push(p->arena, &definition->as.record.fields, field);
    parser_add_field_name(p, name, words);
}

// A kind of record the language defines: "a program result" with its output,
// error output and code. Added when any word that uses it appears ("run",
// "worked", "work", "program result"), so other programs don't carry it.
void parser_find_program_result(Parser *p) {
    bool used = false;
    for (size_t i = 0; i < p->count && !used; i++) {
        const Token *token = &p->tokens[i];
        if (token->kind != TOK_WORD) continue;
        used = strcmp(token->text, "run") == 0 || strcmp(token->text, "worked") == 0 ||
               strcmp(token->text, "work") == 0 ||
               (strcmp(token->text, "program") == 0 && i + 1 < p->count && p->tokens[i + 1].kind == TOK_WORD &&
                strcmp(p->tokens[i + 1].text, "result") == 0);
    }
    if (!used || !parser_add_builtin_kind(p, "program result", 2)) return;
    size_t index = p->records->len - 1;
    Stmt *definition = ast_new_stmt(p->arena, STMT_RECORD, parser_token_pos(&p->tokens[0]));
    definition->as.record.name.text = definition->as.record.name.display = "program result";
    definition->as.record.index = index;
    add_field(p, definition, "output", 1, TYPE_TEXT);
    add_field(p, definition, "error output", 2, TYPE_TEXT);
    add_field(p, definition, "code", 1, TYPE_NUMBER);
    p->records->items[index].definition = definition;
    p->records->program_result = index + 1;
}
