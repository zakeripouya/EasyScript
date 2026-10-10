#include <stdbool.h>
#include <stdio.h>
#include <string.h>
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

    size_t col_offset;     // column() cache: the column of col_offset
    size_t col;

    Vec(size_t) indents;   // open block indentation widths, bottom is 0
    Vec(Token) tokens;
} Lexer;

// --- Character classes (ASCII only, independent of locale) -----------------

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_word_char(char c) {
    return is_alpha(c) || is_digit(c) || c == '_';
}

static char to_lower(char c) {
    return (c >= 'A' && c <= 'Z') ? (char)(c - 'A' + 'a') : c;
}

static bool is_continuation_byte(char c) {
    return ((unsigned char)c & 0xC0) == 0x80;
}

// Length of the UTF-8 sequence starting at s, or 0 if it isn't valid.
static size_t utf8_seq_len(const char *s, size_t avail) {
    unsigned char c = (unsigned char)s[0];
    size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
    if (n == 0 || n > avail) return 0;
    for (size_t i = 1; i < n; i++) {
        if (!is_continuation_byte(s[i])) return 0;
    }
    return n;
}

// --- Positions and emitting tokens -----------------------------------------

static char peek_at(const Lexer *lx, size_t offset) {
    return offset < lx->len ? lx->src[offset] : '\0';
}

// True at a newline, a CR before a newline, or the end of input.
static bool at_line_end(const Lexer *lx, size_t offset) {
    if (offset >= lx->len) return true;
    char c = lx->src[offset];
    return c == '\n' || (c == '\r' && (offset + 1 >= lx->len || lx->src[offset + 1] == '\n'));
}

static size_t column(Lexer *lx, size_t offset) {
    if (lx->col_offset < lx->line_start || lx->col_offset > offset) {
        lx->col_offset = lx->line_start;
        lx->col = 1;
    }
    for (; lx->col_offset < offset; lx->col_offset++) {
        if (!is_continuation_byte(lx->src[lx->col_offset])) lx->col++;
    }
    return lx->col;
}

static void emit(Lexer *lx, TokenKind kind, size_t start, size_t end, const char *text, size_t text_len) {
    Token token = {kind, text, text_len, {start, end - start}, lx->line, column(lx, start)};
    vec_push(lx->arena, &lx->tokens, token);
}

static void emit_layout(Lexer *lx, TokenKind kind, size_t start, size_t end) {
    emit(lx, kind, start, end, "", 0);
}

static Span span_between(size_t start, size_t end) {
    Span span = {start, end - start};
    return span;
}

// --- Indentation ------------------------------------------------------------

static const char *plural_spaces(size_t n) {
    return n == 1 ? "space" : "spaces";
}

// "0, 4 or 8"
static const char *describe_levels(Lexer *lx) {
    StrBuf sb;
    sb_init(&sb, lx->arena);
    size_t n = lx->indents.len;
    for (size_t i = 0; i < n; i++) {
        if (i > 0) sb_append(&sb, i == n - 1 ? " or " : ", ");
        sb_appendf(&sb, "%zu", lx->indents.items[i]);
    }
    return sb.data;
}

static void report_bad_dedent(Lexer *lx, const char *levels) {
    diag_error(lx->diag, span_between(lx->line_start, lx->content_start),
               "This line is indented %zu %s, which doesn't line up with any block above it.",
               lx->indent, plural_spaces(lx->indent));
    diag_note(lx->diag, "Use %s spaces to match one of the blocks it belongs to.%s", levels,
              lx->indent_has_tab ? " (A tab counts as 4 spaces.)" : "");
}

// Called before the first token of a line. On a bad dedent the line stays
// at the nearest enclosing level so lexing can continue.
static void apply_indentation(Lexer *lx) {
    size_t top = vec_last(&lx->indents);
    if (lx->indent > top) {
        vec_push(lx->arena, &lx->indents, lx->indent);
        emit_layout(lx, TOK_INDENT, lx->line_start, lx->content_start);
        return;
    }
    if (lx->indent == top) return;

    const char *levels = describe_levels(lx);
    while (vec_last(&lx->indents) > lx->indent) {
        (void)vec_pop(&lx->indents);
        emit_layout(lx, TOK_DEDENT, lx->content_start, lx->content_start);
    }
    if (vec_last(&lx->indents) != lx->indent) report_bad_dedent(lx, levels);
}

static void emit_content(Lexer *lx, TokenKind kind, size_t start, size_t end, const char *text, size_t text_len) {
    if (!lx->line_has_tokens) {
        lx->line_has_tokens = true;
        apply_indentation(lx);
    }
    emit(lx, kind, start, end, text, text_len);
}

static void emit_source_text(Lexer *lx, TokenKind kind, size_t start, size_t end) {
    char *text = arena_alloc(lx->arena, end - start + 1);
    memcpy(text, lx->src + start, end - start);
    emit_content(lx, kind, start, end, text, end - start);
}

// --- Words and numbers ------------------------------------------------------

static bool is_filler(const char *word) {
    return strcmp(word, "the") == 0 || strcmp(word, "a") == 0 || strcmp(word, "an") == 0;
}

// An apostrophe belongs to a word only between letters: isn't, guest's.
static void lex_word(Lexer *lx) {
    size_t start = lx->pos;
    while (is_word_char(peek_at(lx, lx->pos)) ||
           (peek_at(lx, lx->pos) == '\'' && is_alpha(peek_at(lx, lx->pos + 1)))) {
        lx->pos++;
    }
    size_t n = lx->pos - start;
    char *text = arena_alloc(lx->arena, n + 1);
    for (size_t i = 0; i < n; i++) {
        text[i] = to_lower(lx->src[start + i]);
    }
    if (is_filler(text)) return;
    emit_content(lx, TOK_WORD, start, lx->pos, text, n);
}

static void report_letters_after_number(Lexer *lx, size_t start) {
    size_t number_end = lx->pos;
    size_t end = number_end;
    while (is_word_char(peek_at(lx, end))) end++;
    int number_len = (int)(number_end - start);
    int letters_len = (int)(end - number_end);
    diag_error(lx->diag, span_between(start, end), "A number can't have letters stuck to it: \"%.*s\".",
               (int)(end - start), lx->src + start);
    diag_note(lx->diag, "Put a space between them, like \"%.*s %.*s\".", number_len, lx->src + start,
              letters_len, lx->src + number_end);
}

// Integers and decimals. A period counts as a decimal point only when a digit
// follows, so "5." is the number 5 and then the end of a sentence.
static void lex_number(Lexer *lx) {
    size_t start = lx->pos;
    while (is_digit(peek_at(lx, lx->pos))) lx->pos++;
    if (peek_at(lx, lx->pos) == '.' && is_digit(peek_at(lx, lx->pos + 1))) {
        lx->pos++;
        while (is_digit(peek_at(lx, lx->pos))) lx->pos++;
    }
    emit_source_text(lx, TOK_NUMBER, start, lx->pos);
    if (is_alpha(peek_at(lx, lx->pos))) {
        report_letters_after_number(lx, start);
    }
}

// --- Strings ----------------------------------------------------------------

static void report_unterminated_string(Lexer *lx, size_t start) {
    diag_error(lx->diag, span_between(start, lx->pos), "This text is missing its closing quote.");
    diag_note(lx->diag, "Add a \" where the text ends. Text has to finish on the line it starts on.");
}

static void report_unknown_escape(Lexer *lx, size_t backslash, size_t seq_len) {
    diag_error(lx->diag, span_between(backslash, backslash + 1 + seq_len), "I don't know the escape \"\\%.*s\".",
               (int)seq_len, lx->src + backslash + 1);
    diag_note(lx->diag, "Inside text you can use \\n (new line), \\t (tab), \\\" (quote) and \\\\ (backslash).");
}

// Handles the escape at lx->pos (a backslash) and appends its value.
static void lex_escape(Lexer *lx, StrBuf *value) {
    size_t backslash = lx->pos;
    char c = lx->src[backslash + 1];
    switch (c) {
    case 'n': sb_append_char(value, '\n'); break;
    case 't': sb_append_char(value, '\t'); break;
    case '"': sb_append_char(value, '"'); break;
    case '\\': sb_append_char(value, '\\'); break;
    default: {
        size_t n = utf8_seq_len(lx->src + backslash + 1, lx->len - backslash - 1);
        if (n == 0) n = 1;
        report_unknown_escape(lx, backslash, n);
        sb_append_n(value, lx->src + backslash, n + 1);  // keep it as written
        lx->pos += n + 1;
        return;
    }
    }
    lx->pos += 2;
}

static void lex_string(Lexer *lx) {
    size_t start = lx->pos++;
    StrBuf value;
    sb_init(&value, lx->arena);
    while (true) {
        if (at_line_end(lx, lx->pos)) {
            report_unterminated_string(lx, start);
            break;
        }
        char c = lx->src[lx->pos];
        if (c == '"') {
            lx->pos++;
            break;
        }
        if (c == '\\' && !at_line_end(lx, lx->pos + 1)) {
            lex_escape(lx, &value);
        } else if (c == '\\') {
            lx->pos++;  // a backslash at the end of the line; the text is unterminated
        } else {
            sb_append_char(&value, c);
            lx->pos++;
        }
    }
    emit_content(lx, TOK_STRING, start, lx->pos, value.data, value.len);
}

// --- Symbols and stray characters ------------------------------------------

static bool is_curly_quote(const char *s, size_t n, unsigned char last) {
    return n == 3 && (unsigned char)s[0] == 0xE2 && (unsigned char)s[1] == 0x80 && (unsigned char)s[2] == last;
}

static void note_for_character(Lexer *lx, const char *s, size_t n) {
    if (is_curly_quote(s, n, 0x98) || is_curly_quote(s, n, 0x99)) {
        diag_note(lx->diag, "That's a curly apostrophe. Use a straight one (') instead.");
    } else if (is_curly_quote(s, n, 0x9C) || is_curly_quote(s, n, 0x9D)) {
        diag_note(lx->diag, "That's a curly quote. Use straight double quotes (\") around text.");
    } else if (n == 1 && s[0] == '_') {
        diag_note(lx->diag, "A word has to start with a letter.");
    } else if (n > 1) {
        diag_note(lx->diag, "Words can only use the letters a to z, digits and underscores. "
                            "Put other text in double quotes.");
    }
}

static void report_unexpected(Lexer *lx) {
    const char *s = lx->src + lx->pos;
    unsigned char c = (unsigned char)*s;
    size_t n = utf8_seq_len(s, lx->len - lx->pos);
    Span span = {lx->pos, n ? n : 1};

    if (c == '\'') {
        diag_error(lx->diag, span, "I don't understand this apostrophe.");
        diag_note(lx->diag, "An apostrophe can only go inside a word, like isn't or guest's. "
                            "Text goes in double quotes, like \"hello\".");
    } else if (c == '!') {
        diag_error(lx->diag, span, "I don't understand \"!\".");
        diag_note(lx->diag, "To check that two things are different, write \"!=\".");
    } else if (n == 0) {
        diag_error(lx->diag, span, "I don't understand the byte 0x%02X. Is this file saved as UTF-8?", c);
    } else if (c < 0x20 || c == 0x7F) {
        diag_error(lx->diag, span, "I don't understand the invisible character U+%04X.", c);
    } else {
        diag_error(lx->diag, span, "I don't understand the character \"%.*s\".", (int)n, s);
        note_for_character(lx, s, n);
    }
    lx->pos += span.length;
}

static TokenKind two_char_symbol(char a, char b) {
    if (b == '=' && a == '<') return TOK_LESS_EQUAL;
    if (b == '=' && a == '>') return TOK_GREATER_EQUAL;
    if (b == '=' && a == '!') return TOK_NOT_EQUAL;
    return TOK_EOF;
}

static TokenKind one_char_symbol(char c) {
    switch (c) {
    case '.': return TOK_PERIOD;
    case ',': return TOK_COMMA;
    case ':': return TOK_COLON;
    case '(': return TOK_LPAREN;
    case ')': return TOK_RPAREN;
    case '+': return TOK_PLUS;
    case '-': return TOK_MINUS;
    case '*': return TOK_STAR;
    case '/': return TOK_SLASH;
    case '%': return TOK_PERCENT;
    case '=': return TOK_EQUAL;
    case '<': return TOK_LESS;
    case '>': return TOK_GREATER;
    default: return TOK_EOF;
    }
}

static void lex_symbol(Lexer *lx) {
    size_t start = lx->pos;
    TokenKind kind = two_char_symbol(peek_at(lx, start), peek_at(lx, start + 1));
    size_t n = 2;
    if (kind == TOK_EOF) {
        kind = one_char_symbol(peek_at(lx, start));
        n = 1;
    }
    if (kind == TOK_EOF) {
        report_unexpected(lx);
        return;
    }
    lx->pos += n;
    emit_source_text(lx, kind, start, lx->pos);
}

// --- Lines ------------------------------------------------------------------

static void skip_to_line_end(Lexer *lx) {
    while (lx->pos < lx->len && lx->src[lx->pos] != '\n') lx->pos++;
}

static void begin_line(Lexer *lx) {
    lx->line_start = lx->pos;
    lx->indent = 0;
    lx->indent_has_tab = false;
    lx->line_has_tokens = false;
    while (lx->pos < lx->len && (lx->src[lx->pos] == ' ' || lx->src[lx->pos] == '\t')) {
        bool tab = lx->src[lx->pos] == '\t';
        lx->indent += tab ? TAB_WIDTH : 1;
        lx->indent_has_tab |= tab;
        lx->pos++;
    }
    lx->content_start = lx->pos;
}

// A line whose first word is "note:" (any case) is a comment.
static bool is_note_line(const Lexer *lx) {
    const char *word = "note";
    for (size_t i = 0; i < 4; i++) {
        if (to_lower(peek_at(lx, lx->pos + i)) != word[i]) return false;
    }
    return peek_at(lx, lx->pos + 4) == ':';
}

static void lex_token(Lexer *lx) {
    char c = lx->src[lx->pos];
    if (is_alpha(c)) {
        lex_word(lx);
    } else if (is_digit(c)) {
        lex_number(lx);
    } else if (c == '"') {
        lex_string(lx);
    } else {
        lex_symbol(lx);
    }
}

// Lexes up to (not including) the newline that ends the current line.
static void lex_line(Lexer *lx) {
    if (is_note_line(lx)) {
        skip_to_line_end(lx);
        return;
    }
    while (lx->pos < lx->len && lx->src[lx->pos] != '\n') {
        char c = lx->src[lx->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lx->pos++;
        } else if (c == '#') {
            skip_to_line_end(lx);
        } else {
            lex_token(lx);
        }
    }
}

// The NEWLINE token covers "\r\n" when the line ends with one.
static void end_line(Lexer *lx) {
    size_t start = lx->pos > lx->line_start && lx->src[lx->pos - 1] == '\r' ? lx->pos - 1 : lx->pos;
    if (lx->line_has_tokens) emit_layout(lx, TOK_NEWLINE, start, lx->pos + 1);
    lx->line_has_tokens = false;
    lx->pos++;
    lx->line++;
    lx->line_start = lx->pos;
}

static void finish(Lexer *lx) {
    if (lx->line_has_tokens) emit_layout(lx, TOK_NEWLINE, lx->len, lx->len);
    while (lx->indents.len > 1) {
        (void)vec_pop(&lx->indents);
        emit_layout(lx, TOK_DEDENT, lx->len, lx->len);
    }
    emit_layout(lx, TOK_EOF, lx->len, lx->len);
}

TokenList lex(Arena *arena, Diag *diag, const char *source, size_t source_len) {
    Lexer lx = {0};
    lx.arena = arena;
    lx.diag = diag;
    lx.src = source;
    lx.len = source_len;
    lx.line = 1;
    lx.col = 1;
    vec_push(arena, &lx.indents, (size_t)0);

    while (lx.pos < lx.len) {
        begin_line(&lx);
        lex_line(&lx);
        if (lx.pos < lx.len) end_line(&lx);
    }
    finish(&lx);

    TokenList tokens = {lx.tokens.items, lx.tokens.len};
    return tokens;
}

// --- Dumping ----------------------------------------------------------------

const char *token_kind_name(TokenKind kind) {
    static const char *const names[] = {
        "EOF", "NEWLINE", "INDENT", "DEDENT", "WORD", "NUMBER", "STRING",
        "PERIOD", "COMMA", "COLON", "LPAREN", "RPAREN", "PLUS", "MINUS",
        "STAR", "SLASH", "PERCENT", "EQUAL", "LESS", "GREATER",
        "LESS_EQUAL", "GREATER_EQUAL", "NOT_EQUAL",
    };
    return names[kind];
}

static void append_quoted(StrBuf *out, const char *s, size_t n) {
    sb_append_char(out, '"');
    for (size_t i = 0; i < n; i++) {
        unsigned char c = (unsigned char)s[i];
        if (c == '\n') sb_append(out, "\\n");
        else if (c == '\t') sb_append(out, "\\t");
        else if (c == '"') sb_append(out, "\\\"");
        else if (c == '\\') sb_append(out, "\\\\");
        else if (c < 0x20 || c == 0x7F) sb_appendf(out, "\\x%02X", c);
        else sb_append_char(out, (char)c);
    }
    sb_append_char(out, '"');
}

#define KIND_WIDTH 13      // strlen("GREATER_EQUAL")
#define POSITION_WIDTH 8

static void dump_token(const Token *token, StrBuf *out) {
    const char *name = token_kind_name(token->kind);
    sb_append(out, name);
    sb_append_repeat(out, ' ', KIND_WIDTH + 1 - strlen(name));
    int pos_len = snprintf(NULL, 0, "%zu:%zu", token->line, token->column);
    sb_appendf(out, "%zu:%zu", token->line, token->column);
    if (token->kind == TOK_STRING || token->text_len > 0) {
        sb_append_repeat(out, ' ', pos_len < POSITION_WIDTH ? (size_t)(POSITION_WIDTH - pos_len) : 1);
        if (token->kind == TOK_STRING) {
            append_quoted(out, token->text, token->text_len);
        } else {
            sb_append_n(out, token->text, token->text_len);
        }
    }
    sb_append_char(out, '\n');
}

void tokens_dump(const TokenList *tokens, StrBuf *out) {
    for (size_t i = 0; i < tokens->len; i++) {
        dump_token(&tokens->items[i], out);
    }
}
