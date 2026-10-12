// Sentences joined by "and" ("add 5 to total and say total") and the
// pieces of "say" ("say "Hello,", name, "!""), which both decide what a
// comma or an "and" means by what follows it.

#include <string.h>
#include "front/parse_internal.h"

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

// After a piece, a comma followed by one of these keeps its own meaning
// ("the value for K in M, or 0"), so it doesn't start another piece.
static bool at_postfix_after_comma(const Parser *p) {
    static const char *const words[] = {"or", "sorted", "as", "rounded", "starting", "indented"};
    for (size_t i = 0; i < COUNT(words); i++) {
        if (parser_at_word(p, 1, words[i])) return true;
    }
    return false;
}

// "say "Hello,", name, "!"": values separated by commas are pieces of one
// text, with a space between them where it's needed (BINARY_PIECES).
Expr *parser_parse_pieces(Parser *p) {
    Expr *expr = parse_expression(p);
    while (!p->sentence_failed && parser_at(p, 0, TOK_COMMA) && !at_postfix_after_comma(p) && !parser_at_step_link(p)) {
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
    case TOK_MONEY:
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


// --- Sentences that aren't understood ------------------------------------------------

// More sentences to suggest, besides each statement word's example.
static const char *const more_sentences[] = {
    "make a new file called \"notes.txt\"", "read the table in \"sales.csv\" and call it sales",
    "total the amount per month", "what is the total?", "show me the big orders", "for each score in scores:",
};

static bool ends_at(const Parser *p, size_t i) {
    TokenKind kind = parser_peek(p, i)->kind;
    return kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT || kind == TOK_INDENT;
}

// How many of the sentence's words (up to its end) are words of the example.
static size_t shared_words(const Parser *p, const char *example) {
    size_t shared = 0;
    for (size_t i = 0; !ends_at(p, i) && i < 12; i++) {
        const Token *token = parser_peek(p, i);
        if (token->kind != TOK_WORD) continue;
        bool again = false;  // a word written twice counts once
        for (size_t j = 0; j < i && !again; j++) {
            again = parser_peek(p, j)->kind == TOK_WORD && strcmp(parser_peek(p, j)->text, token->text) == 0;
        }
        for (const char *at = example; !again && (at = strstr(at, token->text)) != NULL; at++) {
            bool start = at == example || at[-1] == ' ' || at[-1] == '"';
            char end = at[token->text_len];
            if (start && (end == '\0' || end == ' ' || end == '"' || end == '?' || end == ':')) {
                shared++;
                break;
            }
        }
    }
    return shared;
}

// "Sentences like these work: ...": the (up to) 3 example sentences sharing
// the most words with this one (the extra ones, then the statement table's,
// when they tie). False if none
// shares a word.
static bool note_closest_sentences(Parser *p) {
    size_t forms = parser_form_count(), n = forms + COUNT(more_sentences);
    const char *best[3] = {NULL, NULL, NULL};
    size_t score[3] = {0, 0, 0};
    for (size_t i = 0; i < n; i++) {
        size_t more = COUNT(more_sentences);  // these first: they win ties
        const char *example = i < more ? more_sentences[i] : parser_form_example(i - more);
        bool seen = false;
        for (size_t k = 0; k < 3; k++) seen |= best[k] && strcmp(best[k], example) == 0;
        size_t s = seen ? 0 : shared_words(p, example);
        for (size_t k = 0; s > 0 && k < 3; k++) {
            if (s <= score[k]) continue;
            for (size_t m = 2; m > k; m--) best[m] = best[m - 1], score[m] = score[m - 1];
            best[k] = example, score[k] = s;
            break;
        }
    }
    if (!best[0]) return false;
    StrBuf list;
    sb_init(&list, p->arena);
    for (size_t k = 0; k < 3 && best[k]; k++) {
        bool last = k == 2 || !best[k + 1];
        sb_appendf(&list, "%s\"%s\"", k == 0 ? "" : last ? " or " : ", ", best[k]);
        diag_choice(p->diag, "%s", best[k]);
    }
    diag_note(p->diag, "%s: %s.", best[1] ? "Sentences like these work" : "A sentence like this works", list.data);
    return true;
}

void parser_report_unknown_start(Parser *p) {
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
    size_t forms = parser_form_count(), n = forms + 1 + p->functions.len;
    const char **candidates = arena_alloc(p->arena, n * sizeof(char *));
    for (size_t i = 0; i < forms; i++) candidates[i] = parser_form_word(i);
    candidates[forms] = "please";
    for (size_t i = 0; i < p->functions.len; i++) candidates[forms + 1 + i] = p->functions.items[i];
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
    } else if (!note_closest_sentences(p)) {
        diag_note(p->diag, "%s", generic);
    }
}

