// Statements. A program is a list of statements. A period or the end of the
// line ends each one, and several can share a line when periods separate
// them. A leading "please" is skipped.
//
//   let X be E / let X equal E          set X to E / change X to E
//   add E to X / subtract E from X      increase/decrease/multiply/divide X by E
//   say/print/show/display/write E      ask E and call the answer X
//   write E to file F                   append E to file F
//   read file F and call it X           stop the program
//   if C: / if C, S / if C then S        (parse_if.c)
//   loops, stop the loop, skip this one   (parse_loop.c)

#include <string.h>
#include "front/parse.h"
#include "front/parse_internal.h"

typedef struct StmtForm StmtForm;
typedef Stmt *(*StmtParseFn)(Parser *p, const Token *verb, const StmtForm *form);

struct StmtForm {
    const char *word;      // the first word of the statement
    StmtParseFn parse;
    bool ends_with_name;   // e.g. "add 5 to total": extra words get a different message
    int op;                // ChangeOp, for the change statements
    const char *connector; // "to", "from", or "by", for the change statements
    const char *example;   // shown when the statement is written wrong
};

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

// --- Helpers -----------------------------------------------------------------

Stmt *parser_new_stmt(Parser *p, StmtKind kind, const Token *verb) {
    Stmt *stmt = ast_new_stmt(p->arena, kind, parser_token_pos(verb));
    stmt->verb = verb->span;
    return stmt;
}

static bool word_in(const Token *token, const char *const *words) {
    if (token->kind != TOK_WORD || !words) return false;
    for (size_t i = 0; words[i]; i++) {
        if (strcmp(token->text, words[i]) == 0) return true;
    }
    return false;
}

static bool is_name_word(const Token *token) {
    return token->kind == TOK_WORD && !parser_is_operator_word(token->text) && !parser_is_value_word(token->text);
}

static void report_filler_name(Parser *p, Span filler) {
    if (parser_error(p, filler, "\"%s\" can't be used as a name.", parser_text(p, filler))) {
        diag_note(p->diag, "EasyScript ignores the words \"a\", \"an\" and \"the\" wherever they appear, "
                           "so they can't be names.");
        diag_note(p->diag, "Try a name that says what it holds, like \"total\" or \"answer\".");
    }
}

static void report_reserved_name(Parser *p, const Token *token) {
    if (parser_error(p, token->span, "\"%s\" can't be used as a name.", token->text)) {
        diag_note(p->diag, "\"%s\" already means something in EasyScript.", token->text);
        diag_note(p->diag, "Try a different name, like \"my_%s\".", token->text);
    }
}

static void report_name_missing_after_prev(Parser *p) {
    if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Add a name there, like \"total\".");
    }
}

// Parses a variable name. `next` lists the words that follow the name in this
// statement ("be" after "let X"), or is NULL when the name ends it. A filler
// word dropped right before `next` ("let a be 5") was meant as the name.
bool parser_parse_name(Parser *p, Name *name, const char *const *next) {
    const Token *token = parser_peek(p, 0);
    bool is_next_word = word_in(token, next);
    if (is_name_word(token) && !is_next_word) {
        parser_advance(p);
        name->text = token->text;
        name->pos = parser_token_pos(token);
        return true;
    }
    if (token->filler.length > 0 && (!is_name_word(token) || is_next_word)) {
        report_filler_name(p, token->filler);
    } else if (token->kind == TOK_WORD && !is_next_word) {
        report_reserved_name(p, token);
    } else if (is_next_word || parser_at_sentence_end(p) || parser_at(p, 0, TOK_COLON)) {
        report_name_missing_after_prev(p);
    } else if (parser_error(p, token->span, "I expected a name here, but found %s.", parser_describe(p, token))) {
        diag_note(p->diag, "A name is a single word, like \"total\" or \"player_2\".");
    }
    return false;
}

static void report_expected(Parser *p, const char *display, const char *example) {
    const Token *token = parser_peek(p, 0);
    bool reported = parser_at_sentence_end(p)
                        ? parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))
                        : parser_error(p, token->span, "I expected \"%s\" here, but found %s.", display,
                                       parser_describe(p, token));
    if (reported) diag_note(p->diag, "Write it like \"%s\".", example);
}

// Consumes one of `words` (NULL-terminated) or reports that `words[0]` was expected.
bool parser_expect_word(Parser *p, const char *const *words, const char *example) {
    if (word_in(parser_peek(p, 0), words)) {
        parser_advance(p);
        return true;
    }
    report_expected(p, words[0], example);
    return false;
}

// Consumes the phrase `words` (as the lexer produces it, without filler) or
// reports that `display` was expected, quoting whatever was there instead.
bool parser_expect_phrase(Parser *p, const char *words, const char *display, const char *example) {
    size_t n = parser_match_words(p, words);
    if (n > 0) {
        parser_consume(p, n);
        return true;
    }
    size_t matched = parser_leading_words(p, words);
    if (matched == 0 || parser_at(p, matched, TOK_NEWLINE) || parser_at(p, matched, TOK_EOF)) {
        report_expected(p, display, example);
        return false;
    }
    Span first = parser_peek(p, 0)->span;
    Span last = parser_peek(p, matched)->span;
    Span span = {first.offset, last.offset + last.length - first.offset};
    if (parser_error(p, span, "I expected \"%s\" here, but found \"%s\".", display, parser_text(p, span))) {
        diag_note(p->diag, "Write it like \"%s\".", example);
    }
    return false;
}

// "add to total", "write to file ...": the value is missing before `words`.
// ("to" is a valid name, so without this it would be read as the value.)
static bool reject_missing_value(Parser *p, const char *words) {
    if (!parser_match_words(p, words)) return false;
    if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Add a value there, like a number, some text in quotes, or a name.");
    }
    return true;
}

// --- Statements --------------------------------------------------------------

static Stmt *parse_let(Parser *p, const Token *verb, const StmtForm *form) {
    static const char *const after[] = {"be", "equal", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_LET, verb);
    if (!parser_parse_name(p, &stmt->as.assign.name, after) || !parser_expect_word(p, after, form->example)) return NULL;
    stmt->as.assign.value = parse_expression(p);
    return stmt;
}

static Stmt *parse_set(Parser *p, const Token *verb, const StmtForm *form) {
    static const char *const after[] = {"to", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_SET, verb);
    if (!parser_parse_name(p, &stmt->as.assign.name, after) || !parser_expect_word(p, after, form->example)) return NULL;
    stmt->as.assign.value = parse_expression(p);
    return stmt;
}

// add E to X, subtract E from X
static Stmt *parse_amount_first(Parser *p, const Token *verb, const StmtForm *form) {
    const char *const connector[] = {form->connector, NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_CHANGE, verb);
    stmt->as.change.op = (ChangeOp)form->op;
    if (reject_missing_value(p, form->connector)) return NULL;
    stmt->as.change.amount = parse_expression(p);
    if (p->sentence_failed || !parser_expect_word(p, connector, form->example)) return NULL;
    if (!parser_parse_name(p, &stmt->as.change.target, NULL)) return NULL;
    return stmt;
}

// increase/decrease/multiply/divide X by E
static Stmt *parse_target_first(Parser *p, const Token *verb, const StmtForm *form) {
    const char *const connector[] = {form->connector, NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_CHANGE, verb);
    stmt->as.change.op = (ChangeOp)form->op;
    if (!parser_parse_name(p, &stmt->as.change.target, connector) || !parser_expect_word(p, connector, form->example)) {
        return NULL;
    }
    stmt->as.change.amount = parse_expression(p);
    return stmt;
}

static Stmt *parse_say(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    Stmt *stmt = parser_new_stmt(p, STMT_SAY, verb);
    stmt->as.value = parse_expression(p);
    return stmt;
}

// "write E" says E; "write E to file F" writes it to a file.
static Stmt *parse_write(Parser *p, const Token *verb, const StmtForm *form) {
    if (reject_missing_value(p, "to file")) return NULL;
    Expr *value = parse_expression(p);
    if (p->sentence_failed) return NULL;
    if (parser_match_words(p, "to file")) {
        parser_consume(p, 2);
        Stmt *stmt = parser_new_stmt(p, STMT_WRITE_FILE, verb);
        stmt->as.file_write.text = value;
        stmt->as.file_write.path = parse_expression(p);
        return stmt;
    }
    if (parser_at_word(p, 0, "to")) {
        Span to = parser_advance(p)->span;
        if (parser_error(p, to, "Something is missing after \"to\".")) {
            diag_note(p->diag, "Did you mean \"to file\"? Write it like \"%s\".", form->example);
        }
        return NULL;
    }
    Stmt *stmt = parser_new_stmt(p, STMT_SAY, verb);
    stmt->as.value = value;
    return stmt;
}

static Stmt *parse_ask(Parser *p, const Token *verb, const StmtForm *form) {
    Stmt *stmt = parser_new_stmt(p, STMT_ASK, verb);
    stmt->as.ask.prompt = parse_expression(p);
    if (p->sentence_failed || !parser_expect_phrase(p, "and call answer", "and call the answer", form->example)) return NULL;
    if (!parser_parse_name(p, &stmt->as.ask.answer, NULL)) return NULL;
    return stmt;
}

static Stmt *parse_append(Parser *p, const Token *verb, const StmtForm *form) {
    Stmt *stmt = parser_new_stmt(p, STMT_APPEND_FILE, verb);
    if (reject_missing_value(p, "to file")) return NULL;
    stmt->as.file_write.text = parse_expression(p);
    if (p->sentence_failed || !parser_expect_phrase(p, "to file", "to file", form->example)) return NULL;
    stmt->as.file_write.path = parse_expression(p);
    return stmt;
}

static Stmt *parse_read(Parser *p, const Token *verb, const StmtForm *form) {
    static const char *const file[] = {"file", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_READ_FILE, verb);
    if (!parser_expect_word(p, file, form->example)) return NULL;
    stmt->as.read_file.path = parse_expression(p);
    if (p->sentence_failed || !parser_expect_phrase(p, "and call it", "and call it", form->example)) return NULL;
    if (!parser_parse_name(p, &stmt->as.read_file.name, NULL)) return NULL;
    return stmt;
}

static Stmt *parse_if_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_if(p, verb);
}

static Stmt *parse_function_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_function(p, verb);
}

static Stmt *parse_return_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_return(p, verb);
}

static Stmt *parse_call_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_call_statement(p, verb);
}

static Stmt *parse_loop_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_loop(p, verb);
}

static Stmt *parse_control_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_loop_control(p, verb);
}

static const StmtForm forms[] = {
    {"let", parse_let, false, 0, NULL, "let total be 0"},
    {"set", parse_set, false, 0, NULL, "set total to 10"},
    {"change", parse_set, false, 0, NULL, "change total to 10"},
    {"add", parse_amount_first, true, CHANGE_ADD, "to", "add 5 to total"},
    {"subtract", parse_amount_first, true, CHANGE_SUBTRACT, "from", "subtract 5 from total"},
    {"increase", parse_target_first, false, CHANGE_ADD, "by", "increase total by 5"},
    {"decrease", parse_target_first, false, CHANGE_SUBTRACT, "by", "decrease total by 5"},
    {"multiply", parse_target_first, false, CHANGE_MULTIPLY, "by", "multiply total by 2"},
    {"divide", parse_target_first, false, CHANGE_DIVIDE, "by", "divide total by 2"},
    {"say", parse_say, false, 0, NULL, "say \"hello\""},
    {"print", parse_say, false, 0, NULL, "print \"hello\""},
    {"show", parse_say, false, 0, NULL, "show \"hello\""},
    {"display", parse_say, false, 0, NULL, "display \"hello\""},
    {"write", parse_write, false, 0, NULL, "write \"hello\" to file \"notes.txt\""},
    {"ask", parse_ask, true, 0, NULL, "ask \"What's your name?\" and call the answer name"},
    {"append", parse_append, false, 0, NULL, "append \"hello\" to file \"notes.txt\""},
    {"read", parse_read, true, 0, NULL, "read file \"notes.txt\" and call it notes"},
    {"stop", parse_control_form, true, 0, NULL, "stop the loop"},
    {"break", parse_control_form, true, 0, NULL, "stop the loop"},
    {"skip", parse_control_form, true, 0, NULL, "skip this one"},
    {"continue", parse_control_form, true, 0, NULL, "skip this one"},
    {"move", parse_control_form, true, 0, NULL, "move on"},
    {"if", parse_if_form, false, 0, NULL, "if total is 5, say \"five\""},
    {"count", parse_loop_form, false, 0, NULL, "count from 1 to 10:"},
    {"go", parse_loop_form, false, 0, NULL, "go from 0 to 100 in steps of 10:"},
    {"for", parse_loop_form, false, 0, NULL, "for each n from 1 to 10:"},
    {"do", parse_loop_form, false, 0, NULL, "do this 3 times:"},
    {"repeat", parse_loop_form, false, 0, NULL, "repeat 3 times:"},
    {"while", parse_loop_form, false, 0, NULL, "while x is less than 10:"},
    {"as", parse_loop_form, false, 0, NULL, "as long as x is less than 10:"},
    {"keep", parse_loop_form, false, 0, NULL, "keep doing this until done:"},
    {"forever", parse_loop_form, false, 0, NULL, "forever:"},
    {"to", parse_function_form, false, 0, NULL, "to greet someone:"},
    {"give", parse_return_form, false, 0, NULL, "give back total"},
    {"return", parse_return_form, false, 0, NULL, "return total"},
    {"call", parse_call_form, false, 0, NULL, "call greet with \"Paris\""},
};

// --- Sentences ---------------------------------------------------------------

static const StmtForm *find_form_word(const char *word) {
    for (size_t i = 0; i < COUNT(forms); i++) {
        if (strcmp(word, forms[i].word) == 0) return &forms[i];
    }
    return NULL;
}

static const StmtForm *find_form(const Token *token) {
    return token->kind == TOK_WORD ? find_form_word(token->text) : NULL;
}

bool parser_is_statement_word(const char *word) {
    return find_form_word(word) != NULL || strcmp(word, "please") == 0;
}

bool parser_looks_like_assignment(const Token *next) {
    return next->kind == TOK_EQUAL ||
           (next->kind == TOK_WORD &&
            (strcmp(next->text, "is") == 0 || strcmp(next->text, "equals") == 0 || strcmp(next->text, "be") == 0));
}

static void report_unknown_start(Parser *p) {
    static const char *const generic = "Sentences start with a word like \"let\", \"set\", \"say\" or \"ask\".";
    const Token *token = parser_peek(p, 0);
    if (token->kind != TOK_WORD) {
        if (parser_error(p, token->span, "A sentence can't start with %s.", parser_describe(p, token))) {
            diag_note(p->diag, "%s", generic);
        }
        return;
    }
    if (!parser_error(p, token->span, "I don't know a sentence that starts with \"%s\".", token->text)) return;
    // Statement words and the program's function names, as suggestions.
    size_t n = COUNT(forms) + 1 + p->functions.len;
    const char **words = arena_alloc(p->arena, n * sizeof(char *));
    for (size_t i = 0; i < COUNT(forms); i++) words[i] = forms[i].word;
    words[COUNT(forms)] = "please";
    for (size_t i = 0; i < p->functions.len; i++) words[COUNT(forms) + 1 + i] = p->functions.items[i];
    const char *closest = parser_closest_words(p, token->text, words, n);
    if (closest) {
        diag_note(p->diag, "Did you mean %s?", closest);
    } else if (parser_looks_like_assignment(parser_peek(p, 1))) {
        diag_note(p->diag, "To make a variable, write \"let %s be ...\". To change one, write \"set %s to ...\".",
                  token->text, token->text);
    } else {
        diag_note(p->diag, "%s", generic);
    }
}

Stmt *parse_statement(Parser *p) {
    p->ended_with_block = false;
    p->ends_with_name = false;
    if (parser_at_word(p, 0, "please")) {
        parser_advance(p);
        if (parser_at_sentence_end(p)) {
            if (parser_error(p, p->prev_span, "Something is missing after \"please\".")) {
                diag_note(p->diag, "Write a sentence after it, like \"please say \"hello\"\".");
            }
            return NULL;
        }
    }
    const Token *verb = parser_peek(p, 0);
    const StmtForm *form = find_form(verb);
    if (!form && verb->kind == TOK_WORD && parser_is_function(p, verb->text)) {
        parser_advance(p);
        Stmt *call = parse_call_statement(p, verb);
        if (call) call->pos = ast_pos_join(call->pos, (SourcePos){p->prev_span, 0, 0});
        return call;
    }
    if (!form) {
        report_unknown_start(p);
        return NULL;
    }
    parser_advance(p);
    p->ends_with_name = form->ends_with_name;  // a one-line if's inner statement overrides this
    Stmt *stmt = form->parse(p, verb, form);
    if (stmt) {
        SourcePos end = {p->prev_span, 0, 0};
        stmt->pos = ast_pos_join(stmt->pos, end);
    }
    return stmt;
}

static void skip_sentence(Parser *p) {
    while (!parser_at_sentence_end(p)) parser_advance(p);
}

void parser_skip_line_and_block(Parser *p) {
    while (!parser_at(p, 0, TOK_NEWLINE) && !parser_at(p, 0, TOK_EOF) && !parser_at(p, 0, TOK_DEDENT)) {
        parser_advance(p);
    }
    if (parser_at(p, 0, TOK_NEWLINE)) parser_advance(p);
    p->sentence_failed = false;
    if (parser_at(p, 0, TOK_INDENT)) {
        parser_advance(p);
        Block discarded = {0};
        parse_statements(p, &discarded, true);
    }
}

static void report_extra_words(Parser *p) {
    const Token *token = parser_peek(p, 0);
    if (parser_error(p, token->span, "I expected the sentence to end after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "End the sentence with a period, or start a new line.");
    }
}

// After a statement: a period, the end of the line, or the end of the file.
static void end_statement(Parser *p) {
    if (!p->sentence_failed && !parser_at_sentence_end(p)) {
        if (p->ends_with_name) {
            report_extra_words(p);
        } else {
            parser_report_leftover(p);
        }
    }
    skip_sentence(p);
    if (parser_at(p, 0, TOK_PERIOD)) parser_advance(p);
    if (parser_at(p, 0, TOK_NEWLINE)) parser_advance(p);
    p->sentence_failed = false;
}

// An indented block that nothing opened: report it, then parse and discard
// it so its DEDENT can't close an enclosing block early.
static void reject_indent(Parser *p) {
    const Token *indent = parser_advance(p);
    if (parser_error(p, indent->span, "This line is indented, but nothing above it starts a block.")) {
        diag_note(p->diag, "Remove the spaces at the start of the line.");
    }
    p->sentence_failed = false;
    Block discarded = {0};
    parse_statements(p, &discarded, true);
}

// A period where a sentence should start: "say 1.." or ".5".
static void reject_stray_period(Parser *p) {
    if (!parser_reject_bare_decimal(p)) {
        const Token *period = parser_advance(p);
        if (parser_error(p, period->span, "There's a period here with no sentence before it.")) {
            diag_note(p->diag, "Remove the extra period.");
        }
    }
    skip_sentence(p);
    p->sentence_failed = false;
}

void parse_statements(Parser *p, Block *block, bool in_block) {
    const Stmt *previous = NULL;  // the statement before, for a stray "otherwise"
    while (!parser_at(p, 0, TOK_EOF)) {
        TokenKind kind = parser_peek(p, 0)->kind;
        if (kind == TOK_DEDENT) {
            parser_advance(p);
            if (in_block) return;
        } else if (kind == TOK_INDENT) {
            reject_indent(p);
        } else if (kind == TOK_PERIOD) {
            reject_stray_period(p);
        } else if (kind == TOK_NEWLINE) {
            parser_advance(p);
        } else if (parser_at_otherwise(p)) {
            parse_orphan_otherwise(p, previous);
            previous = NULL;
        } else {
            Stmt *stmt = parse_statement(p);
            if (stmt && !p->sentence_failed) vec_push(p->arena, block, stmt);
            if (p->ended_with_block) {
                p->sentence_failed = false;
            } else {
                end_statement(p);
            }
            previous = stmt;
        }
    }
}

Block *parse_program(Arena *arena, Diag *diag, const char *source, const TokenList *tokens) {
    Parser p = {0};
    p.arena = arena;
    p.diag = diag;
    p.source = source;
    p.tokens = tokens->items;
    p.count = tokens->len;

    Block *program = arena_alloc(arena, sizeof(Block));
    parser_find_functions(&p);
    parse_statements(&p, program, false);
    return program;
}
