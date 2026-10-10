// Functions: defining, giving back, and calling as a statement.
//
//   to greet someone:                    give back E / return E / return
//   to area with width and height:       ("of" and "using" work like "with")
//
//   greet "Paris".   greet with "Paris".   call greet with "Paris".
//
// Calls inside expressions ("area of 3 and 4") are in parse_expr.c. A
// pre-scan collects every "to NAME" at the start of a line first, so a
// sentence can call a function that's defined further down.

#include <string.h>
#include "front/parse_internal.h"

static bool at_line_start(const Parser *p, size_t index) {
    if (index == 0) return true;
    TokenKind before = p->tokens[index - 1].kind;
    return before == TOK_NEWLINE || before == TOK_INDENT || before == TOK_DEDENT;
}

void parser_find_functions(Parser *p) {
    for (size_t i = 0; i + 1 < p->count; i++) {
        const Token *token = &p->tokens[i];
        if (token->kind == TOK_WORD && strcmp(token->text, "to") == 0 && at_line_start(p, i) &&
            p->tokens[i + 1].kind == TOK_WORD) {
            vec_push(p->arena, &p->functions, p->tokens[i + 1].text);
        }
    }
}

bool parser_is_function(const Parser *p, const char *name) {
    for (size_t i = 0; i < p->functions.len; i++) {
        if (strcmp(p->functions.items[i], name) == 0) return true;
    }
    return false;
}

// --- Defining ------------------------------------------------------------------

static bool reject_statement_word_name(Parser *p, const Name *name) {
    if (!parser_is_statement_word(name->text)) return false;
    if (parser_error(p, name->pos.span, "\"%s\" starts sentences in EasyScript, so it can't be the name of a function.",
                     name->text)) {
        diag_note(p->diag, "Pick another name, like \"%s_it\".", name->text);
    }
    return true;
}

static bool report_missing_input(Parser *p) {
    if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Name the inputs, like \"to area with width and height:\", or leave out \"%s\".",
                  parser_text(p, p->prev_span));
    }
    return false;
}

// [with | using] P [and | ,] Q ... up to the colon.
static bool parse_params(Parser *p, Stmt *stmt) {
    static const char *const after_param[] = {"and", NULL};
    bool introduced = parser_at_word(p, 0, "with") || parser_at_word(p, 0, "using") || parser_at_word(p, 0, "of");
    if (introduced) parser_advance(p);
    const Token *next = parser_peek(p, 0);
    if (next->kind == TOK_COLON && next->filler.length > 0) {
        Name ignored;
        return parser_parse_name(p, &ignored, NULL);  // "to greet a:": explains that "a" is filler
    }
    if (introduced && (next->kind == TOK_COLON || parser_at_sentence_end(p))) return report_missing_input(p);
    while (!parser_at(p, 0, TOK_COLON) && !parser_at_sentence_end(p)) {
        Name param = {0};
        if (!parser_parse_name(p, &param, after_param)) return false;
        vec_push(p->arena, &stmt->as.function.params, param);
        if (!parser_at_word(p, 0, "and") && !parser_at(p, 0, TOK_COMMA)) break;
        parser_advance(p);
    }
    return true;
}

Stmt *parse_function(Parser *p, const Token *verb) {
    static const char *const after_name[] = {"with", "using", "of", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_FUNCTION, verb);
    bool ok = parser_parse_name(p, &stmt->as.function.name, after_name) &&
              !reject_statement_word_name(p, &stmt->as.function.name) && parse_params(p, stmt);
    if (!ok) {
        parser_skip_line_and_block(p);
    } else {
        SourcePos header = stmt->pos;
        ok = parser_parse_block(p, &stmt->as.function.body, &header, NULL, "this function");
    }
    p->ended_with_block = true;  // set last: statements inside the block reset it
    return ok ? stmt : NULL;
}

// --- Giving back -----------------------------------------------------------------

Stmt *parse_return(Parser *p, const Token *verb) {
    static const char *const back[] = {"back", NULL};
    if (strcmp(verb->text, "give") == 0 && !parser_expect_word(p, back, "give back total")) return NULL;
    Stmt *stmt = parser_new_stmt(p, STMT_RETURN, verb);
    if (!parser_at_sentence_end(p)) stmt->as.returned = parse_expression(p);
    return stmt;
}

// --- Calling as a statement --------------------------------------------------------

Stmt *parse_call_statement(Parser *p, const Token *verb) {
    static const char *const after_name[] = {"with", "using", "of", NULL};
    Name name = {parser_peek(p, 0)->text, parser_token_pos(verb)};
    if (strcmp(verb->text, "call") == 0) {
        if (!parser_parse_name(p, &name, after_name)) return NULL;
    } else {
        name.text = verb->text;  // the function's name was the first word
    }
    Stmt *stmt = parser_new_stmt(p, STMT_CALL, verb);
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, name.pos);
    call->as.call.name = name.text;
    bool commas = false;
    bool introduced = parser_at_word(p, 0, "with") || parser_at_word(p, 0, "using") || parser_at_word(p, 0, "of");
    if (introduced) {
        commas = parser_at_word(p, 0, "using");
        parser_advance(p);
    }
    if (introduced && parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "Add the values to give it, like \"greet with \"Paris\"\", or leave out \"%s\".",
                      parser_text(p, p->prev_span));
        }
        return NULL;
    }
    if (!parser_at_sentence_end(p)) {
        parser_parse_call_args(p, call, commas);
        call = parser_finish_call(p, call);
    }
    stmt->as.call = call;
    return stmt;
}
