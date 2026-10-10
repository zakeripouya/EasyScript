#ifndef FRONT_LEXER_H
#define FRONT_LEXER_H

#include <stddef.h>
#include "common/arena.h"
#include "common/diag.h"
#include "common/util.h"

typedef enum {
    TOK_EOF,
    TOK_NEWLINE,
    TOK_INDENT,
    TOK_DEDENT,
    TOK_WORD,
    TOK_NUMBER,
    TOK_STRING,
    TOK_PERIOD,         // .
    TOK_COMMA,          // ,
    TOK_COLON,          // :
    TOK_LPAREN,         // (
    TOK_RPAREN,         // )
    TOK_PLUS,           // +
    TOK_MINUS,          // -
    TOK_STAR,           // *
    TOK_SLASH,          // /
    TOK_PERCENT,        // %
    TOK_EQUAL,          // =
    TOK_LESS,           // <
    TOK_GREATER,        // >
    TOK_LESS_EQUAL,     // <=
    TOK_GREATER_EQUAL,  // >=
    TOK_NOT_EQUAL,      // !=
    TOK_EQUAL_EQUAL,    // ==
} TokenKind;

typedef struct {
    TokenKind kind;
    // WORD: lowercased. STRING: the decoded value (escapes applied, no
    // quotes). NUMBER and symbols: as written. Layout tokens and EOF: "".
    const char *text;
    size_t text_len;
    Span span;      // the source bytes this token came from
    size_t line;    // 1-based
    size_t column;  // 1-based, counted in characters (a tab is one)
} Token;

typedef struct {
    Token *items;
    size_t len;  // always ends with TOK_EOF
} TokenList;

// Splits source into tokens. Problems are reported to `diag`, and lexing
// carries on so one run reports every error; check diag_count() afterwards.
//
// Layout: NEWLINE ends every line that produced a token. Blank lines,
// comment lines ("# ..." or "note: ..."), and lines holding only filler
// words produce nothing. INDENT/DEDENT come before the first token of a line
// whose indentation changed (a tab counts as 4 spaces). At EOF the last line
// gets its NEWLINE and every open block gets a DEDENT.
//
// Words are lowercased and "the", "a", "an" are dropped. No word is treated
// as a keyword here; the parser decides what words mean.
TokenList lex(Arena *arena, Diag *diag, const char *source, size_t source_len);

const char *token_kind_name(TokenKind kind);

// One token per line: kind, line:column, then the text (strings quoted and
// escaped). Used by `easyscript tokens`.
void tokens_dump(const TokenList *tokens, StrBuf *out);

#endif
