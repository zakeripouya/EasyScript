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
    bool as_ends_value;     // in a count loop's header, "as" names the loop variable
    Vec(const char *) functions; // names after "to" at the start of a line (pre-scan)
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

// Shared statement helpers (parse_stmt.c), used by parse_if.c and parse_loop.c.
Stmt *parser_new_stmt(Parser *p, StmtKind kind, const Token *verb);
// A variable name. `next` (NULL-terminated, or NULL) lists the words that
// follow the name in this sentence, to explain "let a be 5".
bool parser_parse_name(Parser *p, Name *name, const char *const *next);
// One of `words` (NULL-terminated), or an error showing `example`.
bool parser_expect_word(Parser *p, const char *const *words, const char *example);
// The phrase `words` (without filler), or an error naming `display`.
bool parser_expect_phrase(Parser *p, const char *words, const char *display, const char *example);
// "is", "=", "equals", or "be": the sentence looks like it sets a variable.
bool parser_looks_like_assignment(const Token *next);

// ":" NEWLINE INDENT statements DEDENT. Extends header->span to the colon.
// `if_pos` (or NULL) is the if the block belongs to, for "otherwise" errors;
// `what` names the header in messages ("this \"if\"").
bool parser_parse_block(Parser *p, Block *body, SourcePos *header, const SourcePos *if_pos, const char *what);

// parse_if.c
bool parser_at_otherwise(const Parser *p);  // "otherwise" or "else"
Stmt *parse_if(Parser *p, const Token *verb);
// An "otherwise" that doesn't belong to an if just above it.
void parse_orphan_otherwise(Parser *p, const Stmt *previous);

// Call arguments: single (unary) values separated by "and" (not "and call"),
// and also by commas when `commas` ("NAME using A, B").
void parser_parse_call_args(Parser *p, Expr *call, bool commas);
// After a call's arguments: arithmetic straight after them is ambiguous.
Expr *parser_finish_call(Parser *p, Expr *call);

// parse_stmt.c
bool parser_is_statement_word(const char *word);

// parse_func.c
void parser_find_functions(Parser *p);  // fills p->functions
bool parser_is_function(const Parser *p, const char *name);
Stmt *parse_function(Parser *p, const Token *verb);   // to NAME ...:
Stmt *parse_return(Parser *p, const Token *verb);     // give back / return
// "call NAME ..." (verb "call") or "NAME ..." (verb is a function's name).
Stmt *parse_call_statement(Parser *p, const Token *verb);

// parse_loop.c: a loop starting with `verb` (count, go, for, do, repeat,
// while, as, keep, forever), and stop/break/skip/continue/move on.
Stmt *parse_loop(Parser *p, const Token *verb);
Stmt *parse_loop_control(Parser *p, const Token *verb);

#endif
