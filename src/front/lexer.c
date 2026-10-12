#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "front/lexer.h"

#include "front/lexer_internal.h"

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
size_t lexer_utf8_len(const char *s, size_t avail) {
    unsigned char c = (unsigned char)s[0];
    size_t n = c < 0x80 ? 1 : (c >> 5) == 0x6 ? 2 : (c >> 4) == 0xE ? 3 : (c >> 3) == 0x1E ? 4 : 0;
    if (n == 0 || n > avail) return 0;
    for (size_t i = 1; i < n; i++) {
        if (!is_continuation_byte(s[i])) return 0;
    }
    return n;
}

// --- Positions and emitting tokens -----------------------------------------

char lexer_peek(const Lexer *lx, size_t offset) {
    return offset < lx->len ? lx->src[offset] : '\0';
}

// True at a newline, a CR before a newline, or the end of input.
bool lexer_at_line_end(const Lexer *lx, size_t offset) {
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
    Token token = {kind, text, text_len, {start, end - start}, lx->line, column(lx, start), {0, 0}, lx->file};
    if (kind != TOK_INDENT && kind != TOK_DEDENT) {
        token.filler = lx->pending_filler;
        lx->pending_filler.length = 0;
    }
    vec_push(lx->arena, &lx->tokens, token);
}

static void emit_layout(Lexer *lx, TokenKind kind, size_t start, size_t end) {
    emit(lx, kind, start, end, "", 0);
}

Span lexer_span(size_t start, size_t end) {
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
    diag_error(lx->diag, lexer_span(lx->line_start, lx->content_start),
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

void lexer_emit_content(Lexer *lx, TokenKind kind, size_t start, size_t end, const char *text, size_t text_len) {
    if (!lx->line_has_tokens) {
        lx->line_has_tokens = true;
        apply_indentation(lx);
    }
    emit(lx, kind, start, end, text, text_len);
}

static void emit_source_text(Lexer *lx, TokenKind kind, size_t start, size_t end) {
    char *text = arena_alloc(lx->arena, end - start + 1);
    memcpy(text, lx->src + start, end - start);
    lexer_emit_content(lx, kind, start, end, text, end - start);
}

// --- Words and numbers ------------------------------------------------------

static bool is_filler(const char *word) {
    return strcmp(word, "the") == 0 || strcmp(word, "a") == 0 || strcmp(word, "an") == 0;
}

// The word from start to the current position: lowercased, or dropped as filler.
static void lex_plain_word(Lexer *lx, size_t start) {
    size_t n = lx->pos - start;
    char *text = arena_alloc(lx->arena, n + 1);
    for (size_t i = 0; i < n; i++) {
        text[i] = to_lower(lx->src[start + i]);
    }
    if (is_filler(text)) {
        lx->pending_filler = lexer_span(start, lx->pos);
        return;
    }
    lexer_emit_content(lx, TOK_WORD, start, lx->pos, text, n);
}

// An apostrophe belongs to a word only between letters: isn't, rock'n'roll.
// A word ending in 's is the word and then a possessive: "rex's" is "rex", 's.
static void lex_word(Lexer *lx) {
    size_t start = lx->pos;
    while (is_word_char(lexer_peek(lx, lx->pos)) ||
           (lexer_peek(lx, lx->pos) == '\'' && is_alpha(lexer_peek(lx, lx->pos + 1)))) {
        lx->pos++;
    }
    size_t end = lx->pos;
    bool possessive = end - start > 2 && lx->src[end - 2] == '\'' && to_lower(lx->src[end - 1]) == 's';
    if (possessive) lx->pos = end - 2;
    lex_plain_word(lx, start);
    if (!possessive) return;
    lx->pos = end;
    lexer_emit_content(lx, TOK_POSSESSIVE, end - 2, end, "'s", 2);
}

static void report_letters_after_number(Lexer *lx, size_t start) {
    size_t number_end = lx->pos;
    size_t end = number_end;
    while (is_word_char(lexer_peek(lx, end))) end++;
    int number_len = (int)(number_end - start);
    int letters_len = (int)(end - number_end);
    diag_error(lx->diag, lexer_span(start, end), "A number can't have letters stuck to it: \"%.*s\".",
               (int)(end - start), lx->src + start);
    diag_note(lx->diag, "Put a space between them, like \"%.*s %.*s\".", number_len, lx->src + start,
              letters_len, lx->src + number_end);
}

// The suffix a whole number takes as an ordinal: 1st, 2nd, 3rd, 4th, 11th, 21st.
static const char *ordinal_suffix(const char *digits, size_t n) {
    int last = digits[n - 1] - '0';
    int tens = n >= 2 ? digits[n - 2] - '0' : 0;
    if (tens == 1) return "th";
    return last == 1 ? "st" : last == 2 ? "nd" : last == 3 ? "rd" : "th";
}

static bool is_any_suffix(const char *s) {
    return (s[0] == 's' && s[1] == 't') || (s[0] == 'n' && s[1] == 'd') || (s[0] == 'r' && s[1] == 'd') ||
           (s[0] == 't' && s[1] == 'h');
}

// A whole number directly followed by two letters that make an ordinal:
// "7th". The suffix has to be the right one ("2th" is an error).
static bool lex_ordinal(Lexer *lx, size_t start) {
    size_t end = lx->pos;
    char suffix[2] = {to_lower(lexer_peek(lx, end)), to_lower(lexer_peek(lx, end + 1))};
    if (!is_any_suffix(suffix) || is_word_char(lexer_peek(lx, end + 2))) return false;
    const char *right = ordinal_suffix(lx->src + start, end - start);
    lx->pos = end + 2;
    if (suffix[0] != right[0] || suffix[1] != right[1]) {
        diag_error(lx->diag, lexer_span(start, lx->pos), "\"%.*s\" isn't how that ordinal is written.",
                   (int)(lx->pos - start), lx->src + start);
        diag_note(lx->diag, "Write \"%.*s%s\".", (int)(end - start), lx->src + start, right);
    }
    emit_source_text(lx, TOK_ORDINAL, start, lx->pos);
    return true;
}

// Integers and decimals. A period counts as a decimal point only when a digit
// follows, so "5." is the number 5 and then the end of a sentence. A whole
// number with an ordinal suffix is an ordinal ("3rd").
static void lex_number(Lexer *lx) {
    size_t start = lx->pos;
    while (is_digit(lexer_peek(lx, lx->pos))) lx->pos++;
    if (is_alpha(lexer_peek(lx, lx->pos)) && lex_ordinal(lx, start)) return;
    if (lexer_peek(lx, lx->pos) == '.' && is_digit(lexer_peek(lx, lx->pos + 1))) {
        lx->pos++;
        while (is_digit(lexer_peek(lx, lx->pos))) lx->pos++;
    }
    emit_source_text(lx, TOK_NUMBER, start, lx->pos);
    char a = to_lower(lexer_peek(lx, lx->pos)), m = to_lower(lexer_peek(lx, lx->pos + 1));
    bool clock = (a == 'a' || a == 'p') && m == 'm' && !is_word_char(lexer_peek(lx, lx->pos + 2));  // 9am: 9, am
    if (is_alpha(lexer_peek(lx, lx->pos)) && !clock) report_letters_after_number(lx, start);
}

// --- Strings ----------------------------------------------------------------

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
    size_t n = lexer_utf8_len(s, lx->len - lx->pos);
    Span span = {lx->pos, n ? n : 1};

    if (c == '\'') {
        diag_error(lx->diag, span, "I don't understand this apostrophe.");
        diag_note(lx->diag, "An apostrophe can only go inside a word, like isn't or rex's. "
                            "Text goes in double quotes, like \"hello\".");
    } else if (c == '!') {
        diag_error(lx->diag, span, "I don't understand \"!\".");
        diag_note(lx->diag, "To check that two things are different, write \"!=\".");
    } else if (n == 0) {
        diag_error(lx->diag, span, "I don't understand the byte 0x%02X. Is this file saved as UTF-8?", c);
    } else if (c < 0x20 || c == 0x7F) {
        diag_error(lx->diag, span, "I can't read this hidden control code, U+%04X.", c);
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
    if (b == '=' && a == '=') return TOK_EQUAL_EQUAL;
    return TOK_EOF;
}

static TokenKind one_char_symbol(char c) {
    switch (c) {
    case '.':
    case '?': return TOK_PERIOD;  // "what is total?": a question ends a sentence too
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
    TokenKind kind = two_char_symbol(lexer_peek(lx, start), lexer_peek(lx, start + 1));
    size_t n = 2;
    if (kind == TOK_EOF) {
        kind = one_char_symbol(lexer_peek(lx, start));
        n = 1;
    }
    // "(first item of people)'s name": a possessive after a closing parenthesis.
    bool after_paren = start > 0 && lx->src[start - 1] == ')';
    if (kind == TOK_EOF && after_paren && lexer_peek(lx, start) == '\'' && to_lower(lexer_peek(lx, start + 1)) == 's' &&
        !is_word_char(lexer_peek(lx, start + 2))) {
        lx->pos += 2;
        lexer_emit_content(lx, TOK_POSSESSIVE, start, start + 2, "'s", 2);
        return;
    }
    if (kind == TOK_EOF) {
        report_unexpected(lx);
        return;
    }
    lx->pos += n;
    emit_source_text(lx, kind, start, lx->pos);
}

// --- Lines ------------------------------------------------------------------


static void begin_line(Lexer *lx) {
    lx->line_start = lx->pos;
    lx->indent = 0;
    lx->indent_has_tab = false;
    lx->line_has_tokens = lx->continued;  // a line carrying on a sentence has no indentation of its own
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
        if (to_lower(lexer_peek(lx, lx->pos + i)) != word[i]) return false;
    }
    return lexer_peek(lx, lx->pos + 4) == ':';
}

void lexer_token(Lexer *lx) {
    char c = lx->src[lx->pos];
    if (is_alpha(c)) {
        lex_word(lx);
    } else if (is_digit(c)) {
        lex_number(lx);
    } else if (c == '"') {
        lexer_string(lx);
    } else {
        lex_symbol(lx);
    }
}

// Lexes up to (not including) the newline that ends the current line.
static void lex_line(Lexer *lx) {
    if (is_note_line(lx)) {
        lexer_skip_to_line_end(lx);
        return;
    }
    while (lx->pos < lx->len && lx->src[lx->pos] != '\n') {
        char c = lx->src[lx->pos];
        if (c == ' ' || c == '\t' || c == '\r') {
            lx->pos++;
        } else if (c == '#') {
            lexer_skip_to_line_end(lx);
        } else {
            lexer_token(lx);
        }
    }
}

// A sentence whose line ends with ",", "then" or "and" carries on to the
// next line that has words.
static bool carries_on(const Lexer *lx) {
    if (!lx->line_has_tokens) return false;
    const Token *last = &vec_last(&lx->tokens);
    return last->kind == TOK_COMMA ||
           (last->kind == TOK_WORD && (strcmp(last->text, "then") == 0 || strcmp(last->text, "and") == 0));
}

// The NEWLINE token covers "\r\n" when the line ends with one.
static void end_line(Lexer *lx) {
    lexer_text_block(lx);  // "let json be the text below:" and the lines under it
    size_t start = lx->pos > lx->line_start && lx->src[lx->pos - 1] == '\r' ? lx->pos - 1 : lx->pos;
    lx->continued = carries_on(lx);
    if (lx->continued) {
        lx->pos++;
        lx->line++;
        lx->line_start = lx->pos;
        return;
    }
    if (lx->line_has_tokens) emit_layout(lx, TOK_NEWLINE, start, lx->pos + 1);
    lx->line_has_tokens = false;
    lx->pending_filler.length = 0;  // a filler-only line gives nothing to the next line
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

// At the end of a file: its last line's NEWLINE, and a DEDENT for each block still open.
static void close_blocks(Lexer *lx, size_t at) {
    if (lx->line_has_tokens) emit_layout(lx, TOK_NEWLINE, at, at);
    lx->line_has_tokens = lx->continued = false;
    while (lx->indents.len > 1) {
        (void)vec_pop(&lx->indents);
        emit_layout(lx, TOK_DEDENT, at, at);
    }
}

TokenList lex(Arena *arena, Diag *diag, const char *source, size_t source_len) {
    size_t start = 0;
    return lex_files(arena, diag, source, source_len, &start, 1);
}

TokenList lex_files(Arena *arena, Diag *diag, const char *source, size_t source_len, const size_t *starts,
                    size_t files) {
    Lexer lx = {0};
    lx.arena = arena;
    lx.diag = diag;
    lx.src = source;
    lx.len = source_len;
    lx.line = 1;
    lx.col = 1;
    vec_push(arena, &lx.indents, (size_t)0);

    size_t next = 1;  // the next file to start
    while (lx.pos < lx.len) {
        if (next < files && lx.pos >= starts[next]) {
            close_blocks(&lx, lx.pos);
            lx.file = (unsigned)next++;
            lx.line = 1;
        }
        begin_line(&lx);
        lex_line(&lx);
        if (lx.pos < lx.len) end_line(&lx);
    }
    finish(&lx);

    TokenList tokens = {lx.tokens.items, lx.tokens.len};
    return tokens;
}
