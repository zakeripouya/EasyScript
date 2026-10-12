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
//   check which kind S is:               (parse_match.c)
//   loops, stop the loop, skip this one   (parse_loop.c)
//   add E to the end of L, remove, insert, set item N of L to E, sort  (parse_list.c)

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
    if (!parser_parse_name(p, &stmt->as.assign.name, after)) return NULL;
    if (parser_at(p, 0, TOK_POSSESSIVE)) {  // "let rex's age be 4"
        if (parser_error(p, parser_peek(p, 0)->span, "\"let\" makes a new name, so it can't change a field.")) {
            diag_note(p->diag, "To change a field, write \"set %s's ... to ...\".", stmt->as.assign.name.display);
        }
        return NULL;
    }
    if (!parser_expect_word(p, after, form->example)) return NULL;
    stmt->as.assign.value = parse_expression(p);
    return stmt;
}

static Stmt *parse_set(Parser *p, const Token *verb, const StmtForm *form) {
    static const char *const after[] = {"to", NULL};
    static const char *const of_in[] = {"of", "in", NULL};
    if (parser_at_item(p, of_in)) return parser_parse_set_item(p, verb);
    Stmt *stmt = parser_new_stmt(p, STMT_SET, verb);
    if (!parser_parse_target(p, &stmt->as.assign.name, &stmt->as.assign.field, after) ||
        !parser_expect_word(p, after, form->example)) {
        return NULL;
    }
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
    if (form->op == CHANGE_ADD && parser_match_words(p, "end of")) {
        return parser_finish_add_end(p, stmt, stmt->as.change.amount);
    }
    if (!parser_parse_target(p, &stmt->as.change.target, &stmt->as.change.field, NULL)) return NULL;
    return stmt;
}

// increase/decrease/multiply/divide X by E
static Stmt *parse_target_first(Parser *p, const Token *verb, const StmtForm *form) {
    const char *const connector[] = {form->connector, NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_CHANGE, verb);
    stmt->as.change.op = (ChangeOp)form->op;
    if (!parser_parse_target(p, &stmt->as.change.target, &stmt->as.change.field, connector) ||
        !parser_expect_word(p, connector, form->example)) {
        return NULL;
    }
    stmt->as.change.amount = parse_expression(p);
    return stmt;
}

// After a piece, a comma followed by one of these keeps its own meaning
// ("the value for K in M, or 0"), so it doesn't start another piece.
static bool at_postfix_after_comma(const Parser *p) {
    static const char *const words[] = {"or", "sorted", "as", "rounded", "starting"};
    for (size_t i = 0; i < COUNT(words); i++) {
        if (parser_at_word(p, 1, words[i])) return true;
    }
    return false;
}

// "say "Hello,", name, "!"": values separated by commas are pieces of one
// text, with a space between them where it's needed (BINARY_PIECES).
Expr *parser_parse_pieces(Parser *p) {
    Expr *expr = parse_expression(p);
    while (!p->sentence_failed && parser_at(p, 0, TOK_COMMA) && !at_postfix_after_comma(p)) {
        Span comma = parser_advance(p)->span;
        if (parser_at_sentence_end(p)) {
            if (parser_error(p, comma, "Something is missing after \",\".")) {
                diag_note(p->diag, "Put the next piece after the comma, like \"say \"Hello,\", name\".");
                diag_info(p->diag, DIAG_MISSING, "the next piece", NULL);
            }
            return parser_error_expr(p);
        }
        Expr *piece = parse_expression(p);
        Expr *joined = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(expr->pos, piece->pos));
        joined->as.binary.op = BINARY_PIECES;
        joined->as.binary.op_span = comma;
        joined->as.binary.left = expr;
        joined->as.binary.right = piece;
        expr = joined;
    }
    return expr;
}

static Stmt *parse_say(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    Stmt *stmt = parser_new_stmt(p, STMT_SAY, verb);
    stmt->as.value = parser_parse_pieces(p);
    return stmt;
}

// "write E" says E; "write E to file F" writes it to a file.
static Stmt *parse_write(Parser *p, const Token *verb, const StmtForm *form) {
    if (reject_missing_value(p, "to file")) return NULL;
    Expr *value = parse_expression(p);
    if (p->sentence_failed) return NULL;
    if (parser_match_words(p, "to file") || parser_match_words(p, "in file")) {  // "in file": like "put X in file F"
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

// "and call the answer" or "and call it" (the lexer drops "the"), in both
// "ask" and "read file"; an error names the one the sentence's example uses.
static bool expect_call_phrase(Parser *p, const char *phrase, const char *written, const char *example) {
    if (parser_match_words(p, "and call answer") || parser_match_words(p, "and call it")) {
        parser_consume(p, 3);
        return true;
    }
    return parser_expect_phrase(p, phrase, written, example);
}

static Stmt *parse_ask(Parser *p, const Token *verb, const StmtForm *form) {
    Stmt *stmt = parser_new_stmt(p, STMT_ASK, verb);
    stmt->as.ask.prompt = parse_expression(p);
    if (p->sentence_failed || !expect_call_phrase(p, "and call answer", "and call the answer", form->example)) return NULL;
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
    if (p->sentence_failed || !expect_call_phrase(p, "and call it", "and call it", form->example)) return NULL;
    if (!parser_parse_name(p, &stmt->as.read_file.name, NULL)) return NULL;
    return stmt;
}

static Stmt *parse_if_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_if(p, verb);
}

static Stmt *parse_check_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_match(p, verb);
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
    return parse_call_statement(p, verb, NULL);
}

// "keep doing this until" is a loop; any other "keep" makes a constant.
static Stmt *parse_keep_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    if (parser_at_keep_only(p)) return parser_parse_keep_only(p, verb);
    return parser_at_word(p, 0, "doing") ? parse_loop(p, verb) : parse_constant(p, verb);
}

static Stmt *parse_loop_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_loop(p, verb);
}

static Stmt *parse_control_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parse_loop_control(p, verb);
}

static Stmt *parse_remove_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_remove(p, verb);
}

static Stmt *parse_insert_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_insert(p, verb);
}

static Stmt *parse_sort_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_sort(p, verb);
}

static Stmt *parse_apply_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_apply(p, verb);
}

static Stmt *parse_wait_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_wait(p, verb);
}

static Stmt *parse_shuffle_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_shuffle(p, verb);
}

static Stmt *parse_save_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_save(p, verb);
}

static Stmt *parse_put_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_put(p, verb);
}

static Stmt *parse_use_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_use(p, verb);
}

static Stmt *parse_privately_form(Parser *p, const Token *verb, const StmtForm *form) {
    (void)form;
    return parser_parse_privately(p, verb);
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
    {"unless", parse_if_form, false, 0, NULL, "unless total is 5, say \"not five\""},
    {"check", parse_check_form, false, 0, NULL, "check which kind s is:"},
    {"count", parse_loop_form, false, 0, NULL, "count from 1 to 10:"},
    {"go", parse_loop_form, false, 0, NULL, "go from 0 to 100 in steps of 10:"},
    {"for", parse_loop_form, false, 0, NULL, "for each n from 1 to 10:"},
    {"do", parse_loop_form, false, 0, NULL, "do this 3 times:"},
    {"repeat", parse_loop_form, false, 0, NULL, "repeat 3 times:"},
    {"while", parse_loop_form, false, 0, NULL, "while x is less than 10:"},
    {"as", parse_loop_form, false, 0, NULL, "as long as x is less than 10:"},
    {"keep", parse_keep_form, false, 0, NULL, "keep tax_rate as 0.2"},
    {"forever", parse_loop_form, false, 0, NULL, "forever:"},
    {"to", parse_function_form, false, 0, NULL, "to greet someone:"},
    {"give", parse_return_form, false, 0, NULL, "give back total"},
    {"return", parse_return_form, false, 0, NULL, "return total"},
    {"call", parse_call_form, false, 0, NULL, "call greet with \"Paris\""},
    {"remove", parse_remove_form, true, 0, NULL, "remove item 2 from scores"},
    {"insert", parse_insert_form, true, 0, NULL, "insert 7 at position 1 of scores"},
    {"sort", parse_sort_form, false, 0, NULL, "sort scores"},
    {"apply", parse_apply_form, false, 0, NULL, "apply double to every number in scores"},
    {"use", parse_use_form, true, 0, NULL, "use helpers"},
    {"privately", parse_privately_form, false, 0, NULL, "privately to tidy with text:"},
    {"wait", parse_wait_form, false, 0, NULL, "wait 2 seconds"},
    {"shuffle", parse_shuffle_form, false, 0, NULL, "shuffle cards"},
    {"save", parse_save_form, false, 0, NULL, "save rows as csv file \"sales.csv\""},
    {"put", parse_put_form, false, 0, NULL, "put \"hello\" in file \"notes.txt\""},
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

// Sentence words that can't follow "and": they open blocks or define things.
static bool starts_block_or_definition(const char *word) {
    static const char *const words[] = {"if", "unless", "check", "count", "go", "for", "do", "repeat", "while",
                                        "as", "forever", "to", "keep", "use", "privately"};
    for (size_t i = 0; i < COUNT(words); i++) {
        if (strcmp(word, words[i]) == 0) return true;
    }
    return false;
}

// A token that can start a value (and so can't come right after one).
static bool starts_value(const Token *token) {
    switch (token->kind) {
    case TOK_NUMBER:
    case TOK_STRING:
    case TOK_LPAREN: return true;
    case TOK_WORD: return !parser_is_operator_word(token->text);
    default: return false;
    }
}

// "and" between two actions means "and then": "say "hi" and stop the loop".
// It's an action when "and" is followed by "then", or by a word that starts
// simple sentences and then by something that starts a value (which can't
// follow a value); "stop", "skip", "break" and "continue" need only not be
// followed by an operator, and "move" needs "on". A function's name after
// "and" is a value ("if done and next word in r is ..."); "and then" calls
// it as a sentence.
bool parser_at_next_action(const Parser *p) {
    if (!parser_at_word(p, 0, "and")) return false;
    if (parser_at_word(p, 1, "then")) return true;
    const Token *word = parser_peek(p, 1);
    if (word->kind != TOK_WORD || strcmp(word->text, "call") == 0) return false;
    if (parser_match_name(p, 1, false)) return false;  // a name the program knows, or a function
    Parser ahead = *p;
    ahead.pos++;
    bool sentence = (parser_is_statement_word(word->text) && !starts_block_or_definition(word->text)) ||
                    parser_at_library_sentence(&ahead);
    if (!sentence) return false;
    if (strcmp(word->text, "move") == 0) return parser_at_word(p, 2, "on");  // "and move on"; "move" alone is a name
    static const char *const controls[] = {"stop", "skip", "break", "continue"};
    for (size_t i = 0; i < COUNT(controls); i++) {
        if (strcmp(word->text, controls[i]) == 0) {
            const Token *next = parser_peek(p, 2);
            return next->kind != TOK_WORD || !parser_is_operator_word(next->text);
        }
    }
    return starts_value(parser_peek(p, 2));
}

bool parser_at_and(const Parser *p) {
    return parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call") && !parser_at_next_action(p);
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
    // A name of several words ("late fee is 5") is shown whole.
    const KnownName *known = parser_match_name(p, 0, false);
    size_t words = known ? known->words : 1;
    Span span = {token->span.offset, parser_peek(p, words - 1)->span.offset + parser_peek(p, words - 1)->span.length -
                                         token->span.offset};
    const char *written = parser_text(p, span);
    if (!parser_error(p, span, "I don't know a sentence that starts with \"%s\".", written)) return;
    // Statement words and the program's function names, as suggestions.
    size_t n = COUNT(forms) + 1 + p->functions.len;
    const char **candidates = arena_alloc(p->arena, n * sizeof(char *));
    for (size_t i = 0; i < COUNT(forms); i++) candidates[i] = forms[i].word;
    candidates[COUNT(forms)] = "please";
    for (size_t i = 0; i < p->functions.len; i++) candidates[COUNT(forms) + 1 + i] = p->functions.items[i];
    const char *closest = words == 1 ? parser_closest_words(p, token->text, candidates, n) : NULL;
    const char *longer = NULL;  // a function whose name starts with this word: "to greet someone:"
    for (size_t i = 0; words == 1 && !longer && i < p->functions.len; i++) {
        const char *f = p->functions.items[i];
        if (strncmp(f, token->text, token->text_len) == 0 && f[token->text_len] == ' ') longer = f;
    }
    if (longer) {
        diag_note(p->diag, "The definition is called \"%s\": its name is every word before \"with\", "
                           "\"of\", \"using\" or \"to\". To give \"%s\" an input, write \"to %s with %s:\".",
                  longer, written, written, longer + token->text_len + 1);
    } else if (parser_at_word(p, words, "can")) {  // "a cat can purr:" before any cat is defined
        diag_note(p->diag, "\"%s\" isn't a kind of record I know. Define it first, like \"a %s has a name.\", "
                           "and then what it can do.", written, written);
    } else if (closest) {
        diag_note(p->diag, "Did you mean %s?", closest);
    } else if (parser_looks_like_assignment(parser_peek(p, words))) {
        diag_note(p->diag, "To keep a value in a name, write \"let %s be ...\". To change it, write \"set %s to ...\".",
                  written, written);
    } else {
        diag_note(p->diag, "%s", generic);
    }
}

Stmt *parse_statement(Parser *p) {
    p->ended_with_block = false;
    p->block_end = 0;
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
    if (parser_at_record_definition(p)) return parse_record(p);
    if (parser_at_has_definition(p)) return parser_parse_has(p);
    if (parser_at_can_method(p)) return parser_parse_can(p);
    if (parser_at_sum_definition(p)) return parse_sum(p);
    if (parser_at_made_of(p)) return parser_parse_made_of(p);
    if (parser_interface_words(p, p->pos) > 0) return parse_interface(p);
    if (parser_at_library_sentence(p)) {  // "delete the file F", "clear the screen", "run "git"", ...
        Stmt *stmt = parser_parse_library_sentence(p);
        if (stmt) stmt->pos = ast_pos_join(stmt->pos, (SourcePos){p->prev_span, 0, 0, 0});
        return stmt;
    }
    if (parser_at_module(p, 0)) {  // helpers's greet with "Ann"
        Stmt *call = parser_parse_module_call(p);
        if (call) call->pos = ast_pos_join(call->pos, (SourcePos){p->prev_span, 0, 0, 0});
        return call;
    }
    const StmtForm *form = find_form(verb);
    // A function's name starts a call: one of several words even if its first
    // word starts sentences ("add tax to 100"), a one-word one otherwise.
    const KnownName *function = parser_match_name(p, 0, true);
    if (function && (function->words > 1 || !form)) {
        Stmt *call = parse_call_statement(p, verb, function);
        if (call) call->pos = ast_pos_join(call->pos, (SourcePos){p->prev_span, 0, 0, 0});
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
        SourcePos end = {p->prev_span, 0, 0, 0};
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
    // A broken sentence ending in a colon ("let f be a function taking x:"):
    // its block belongs to it, so it's skipped too.
    bool block = p->sentence_failed && p->pos > 0 && p->tokens[p->pos - 1].kind == TOK_COLON &&
                 parser_at(p, 0, TOK_NEWLINE) && parser_at(p, 1, TOK_INDENT);
    if (parser_at(p, 0, TOK_PERIOD)) parser_advance(p);
    if (parser_at(p, 0, TOK_NEWLINE)) parser_advance(p);
    p->sentence_failed = false;
    if (block) {
        parser_advance(p);
        Block discarded = {0};
        parse_statements(p, &discarded, true);
    }
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

// A sentence, and the ones joined to it with "and" (or "and then"), each
// added to the block. Returns the last.
Stmt *parser_parse_actions(Parser *p, Block *block) {
    Stmt *stmt = parse_statement(p);
    if (stmt && !p->sentence_failed) vec_push(p->arena, block, stmt);
    while (stmt && !p->sentence_failed && !p->ended_with_block && parser_at_next_action(p)) {
        parser_consume(p, parser_at_word(p, 1, "then") ? 2 : 1);
        stmt = parse_statement(p);
        if (stmt && !p->sentence_failed) vec_push(p->arena, block, stmt);
    }
    return stmt;
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
            Stmt *stmt = parser_parse_actions(p, block);
            if (p->ended_with_block) {
                p->sentence_failed = false;
            } else {
                end_statement(p);
            }
            previous = stmt;
        }
    }
}

Program *parse_program(Arena *arena, Diag *diag, const char *source, const TokenList *tokens) {
    Parser p = {0};
    p.arena = arena;
    p.diag = diag;
    p.source = source;
    p.tokens = tokens->items;
    p.count = tokens->len;

    Program *program = arena_alloc(arena, sizeof(Program));
    p.records = &program->records;
    parser_find_modules(&p);
    parser_find_names(&p);
    parse_statements(&p, &program->body, false);
    return program;
}
