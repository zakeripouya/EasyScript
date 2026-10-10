#ifndef FRONT_PARSE_INTERNAL_H
#define FRONT_PARSE_INTERNAL_H

// Shared by the parser's source files (parse_util.c, parse_expr.c,
// parse_stmt.c, parse_if.c). Not for use outside src/front/.

#include <stdbool.h>
#include "common/ast.h"
#include "front/lexer.h"

typedef struct {
    Arena *arena;
    Diag *diag;
    const char *source;
    const Token *tokens;
    size_t count;         // tokens[count - 1] is TOK_EOF
    size_t pos;
    Span prev_span;       // the last token or phrase consumed
    bool sentence_failed; // an error was reported; later ones in this sentence are suppressed
    bool ended_with_block;  // the statement just parsed ended with an indented block
    bool ends_with_name;    // ... or ended with a name or keyword ("add 5 to total")
    Vec(SourcePos) open_ifs; // ifs whose blocks are being parsed, innermost last
} Parser;

// A word or symbol sequence with a meaning, e.g. "is greater than".
typedef struct {
    const char *words;    // space-separated, as the lexer produces them
    const char *display;  // how messages show it ("as a number" for "as number")
    int value;            // e.g. a BinaryOp
    bool needs_operand;   // only matches when a value can follow it
} Phrase;

// --- Tokens ----------------------------------------------------------------

const Token *parser_peek(const Parser *p, size_t ahead);
bool parser_at(const Parser *p, size_t ahead, TokenKind kind);
bool parser_at_word(const Parser *p, size_t ahead, const char *word);
const Token *parser_advance(Parser *p);
// Consumes n tokens and returns their combined span.
Span parser_consume(Parser *p, size_t n);
SourcePos parser_token_pos(const Token *token);
bool parser_at_sentence_end(const Parser *p);

// yes, no, true, false, nothing, it: values, so they can't be names.
bool parser_is_value_word(const char *word);

// True if `token` could begin a value.
bool parser_starts_operand(const Token *token);
// Words with an operator meaning; they can't be used as names.
bool parser_is_operator_word(const char *word);

// --- Phrases ---------------------------------------------------------------

// Number of tokens if the next tokens spell `words`, else 0.
size_t parser_match_words(const Parser *p, const char *words);

// How many leading words of `words` the next tokens spell.
size_t parser_leading_words(const Parser *p, const char *words);

// The longest phrase in the table that the next tokens spell, with its token
// count in *len. If the next tokens are a phrase missing its last word
// ("is greater 5"), reports a "Did you mean" error and returns NULL.
const Phrase *parser_match_phrase(Parser *p, const Phrase *table, size_t n, size_t *len);

// --- Errors ----------------------------------------------------------------

// Reports an error unless one was already reported in this sentence.
// Returns whether it was reported, so the caller knows to add notes.
bool parser_error(Parser *p, Span span, const char *fmt, ...) PRINTF_LIKE(3, 4);

// An EXPR_ERROR node at the next token.
Expr *parser_error_expr(Parser *p);

// The source text of a span (arena copy).
const char *parser_text(const Parser *p, Span span);

// A span quoted for a message: "plus", or the text "Name?" for text.
const char *parser_quoted(const Parser *p, Span span);

// How messages refer to a token: "plus", "the end of the line", ...
const char *parser_describe(const Parser *p, const Token *token);

// Reports whatever is left in a sentence after a complete expression.
void parser_report_leftover(Parser *p);

// The candidates closest to `word` by edit distance, as "a" or "a" or "b";
// NULL if none is close enough.
const char *parser_closest_words(const Parser *p, const char *word, const char *const *candidates, size_t n);

// ".5": reports "write 0.5", consumes both tokens, and returns true.
bool parser_reject_bare_decimal(Parser *p);

// --- Grammar ---------------------------------------------------------------

Expr *parse_expression(Parser *p);

// One statement, including a leading "please". NULL after an error. Sets
// ended_with_block and ends_with_name.
Stmt *parse_statement(Parser *p);

// Statements until the end of the file, or (in_block) until and including
// the DEDENT that closes the block.
void parse_statements(Parser *p, Block *block, bool in_block);

// Skips to the end of the line. If an indented block follows, parses and
// discards it, so a broken header doesn't cause errors about its block.
void parser_skip_line_and_block(Parser *p);

// parse_if.c
bool parser_at_otherwise(const Parser *p);  // "otherwise" or "else"
Stmt *parse_if(Parser *p, const Token *verb);
// An "otherwise" that doesn't belong to an if just above it.
void parse_orphan_otherwise(Parser *p, const Stmt *previous);

#endif
