// Sentences joined by "and" ("add 5 to total and say total") and the
// pieces of "say" ("say "Hello,", name, "!""), which both decide what a
// comma or an "and" means by what follows it.

#include <string.h>
#include "front/parse_internal.h"

#define COUNT(table) (sizeof(table) / sizeof((table)[0]))

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

