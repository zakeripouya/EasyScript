#ifndef FRONT_LEXER_INTERNAL_H
#define FRONT_LEXER_INTERNAL_H

// Shared by the lexer's files (lexer.c, lexer_text.c). Not for use outside src/front/.

#include <stdbool.h>
#include "front/lexer.h"

#define TAB_WIDTH 4

typedef struct {
    Arena *arena;
    Diag *diag;
    const char *src;
    size_t len;
    size_t pos;

    size_t line;           // 1-based line containing pos
    size_t line_start;     // offset of the first byte of that line
    size_t content_start;  // offset just past the line's indentation
    size_t indent;         // indentation width, tabs counted as TAB_WIDTH
    bool indent_has_tab;
    bool line_has_tokens;
    bool continued;        // the last line ended with ",", "then" or "and", so this one carries on its sentence

    size_t col_offset;     // column() cache: the column of col_offset
    size_t col;

    Vec(size_t) indents;   // open block indentation widths, bottom is 0
    Span pending_filler;   // a filler word dropped since the last token (length 0 if none)
    Vec(Token) tokens;
    unsigned file;         // the file being lexed (lex_files)
} Lexer;

char lexer_peek(const Lexer *lx, size_t offset);            // the byte at offset, or '\0' past the end
bool lexer_at_line_end(const Lexer *lx, size_t offset);     // a newline, a CR before one, or the end
Span lexer_span(size_t start, size_t end);
size_t lexer_utf8_len(const char *s, size_t avail);         // the UTF-8 sequence's length, or 0 if invalid
void lexer_emit_content(Lexer *lx, TokenKind kind, size_t start, size_t end, const char *text, size_t text_len);
bool lexer_unquoted(Lexer *lx);  // "grades.csv", "team@shop.com", "https://..." written without quotes
void lexer_token(Lexer *lx);                                // one token at lx->pos
void lexer_skip_to_line_end(Lexer *lx);
void lexer_string(Lexer *lx);                               // lexer_text.c: text in quotes, at its quote
bool lexer_text_block(Lexer *lx);
bool lexer_money(Lexer *lx);       // lexer_money.c: "$1,200", "€4.50", "£3" at lx->pos  // lexer_text.c: at the end of a line ending "the text below:", its text

#endif
