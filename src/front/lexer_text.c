// Text in quotes: escapes, values in brackets, and text blocks. Part of the
// lexer (lexer.c); shares its state through lexer_internal.h.

#include <stdint.h>
#include <string.h>
#include "front/lexer_internal.h"

static void report_unterminated_string(Lexer *lx, size_t start) {
    diag_error(lx->diag, lexer_span(start, lx->pos), "This text is missing its closing quote.");
    diag_note(lx->diag, "Add a \" where the text ends. Text has to finish on the line it starts on.");
}

static void report_unknown_escape(Lexer *lx, size_t backslash, size_t seq_len) {
    diag_error(lx->diag, lexer_span(backslash, backslash + 1 + seq_len), "I don't know the escape \"\\%.*s\".",
               (int)seq_len, lx->src + backslash + 1);
    diag_note(lx->diag, "Inside text you can use \\n (new line), \\t (tab), \\r (back to the line's start), "
                        "\\\" (quote), \\\\ (backslash) and \\u{e9} (a character by its code).");
}

static int hex_digit(char c) {
    return c >= '0' && c <= '9' ? c - '0' : c >= 'a' && c <= 'f' ? c - 'a' + 10 : c >= 'A' && c <= 'F' ? c - 'A' + 10 : -1;
}

// "\u{1F600}" or "\u00e9" (four hex digits): the character with that code, as UTF-8.
static void lex_unicode(Lexer *lx, StrBuf *value) {
    size_t at = lx->pos + 2, digits = 0;
    bool braces = lexer_peek(lx, at) == '{';
    if (braces) at++;
    unsigned long code = 0;
    while (hex_digit(lexer_peek(lx, at)) >= 0 && digits < (braces ? 6 : 4)) {
        code = code * 16 + (unsigned long)hex_digit(lexer_peek(lx, at++));
        digits++;
    }
    bool ok = braces ? digits > 0 && lexer_peek(lx, at) == '}' : digits == 4;
    if (braces && lexer_peek(lx, at) == '}') at++;
    if (!ok || code == 0 || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) {
        diag_error(lx->diag, lexer_span(lx->pos, at), "\"%.*s\" isn't a character I know.", (int)(at - lx->pos),
                   lx->src + lx->pos);
        diag_note(lx->diag, "Write the character's code in hex, like \\u{e9} for \"\u00e9\" or \\u{1F600}. Codes go "
                            "from 1 to 10FFFF.");
        diag_info(lx->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        sb_append_n(value, lx->src + lx->pos, at - lx->pos);
        lx->pos = at;
        return;
    }
    char bytes[4];
    size_t n = code < 0x80 ? 1 : code < 0x800 ? 2 : code < 0x10000 ? 3 : 4;
    static const unsigned char first[] = {0, 0, 0xC0, 0xE0, 0xF0};
    for (size_t i = n; i > 1; i--) {
        bytes[i - 1] = (char)(0x80 | (code & 0x3F));
        code >>= 6;
    }
    bytes[0] = (char)(first[n] | code);
    sb_append_n(value, bytes, n);
    lx->pos = at;
}

// Handles the escape at lx->pos (a backslash) and appends its value.
static void lex_escape(Lexer *lx, StrBuf *value) {
    size_t backslash = lx->pos;
    char c = lx->src[backslash + 1];
    switch (c) {
    case 'n': sb_append_char(value, '\n'); break;
    case 't': sb_append_char(value, '\t'); break;
    case 'r': sb_append_char(value, '\r'); break;
    case 'u':
        lex_unicode(lx, value);
        return;
    case '"': sb_append_char(value, '"'); break;
    case '\\': sb_append_char(value, '\\'); break;
    default: {
        size_t n = lexer_utf8_len(lx->src + backslash + 1, lx->len - backslash - 1);
        if (n == 0) n = 1;
        report_unknown_escape(lx, backslash, n);
        sb_append_n(value, lx->src + backslash, n + 1);  // keep it as written
        lx->pos += n + 1;
        return;
    }
    }
    lx->pos += 2;
}

void lexer_skip_to_line_end(Lexer *lx) {
    while (lx->pos < lx->len && lx->src[lx->pos] != '\n') lx->pos++;
}

// "[VALUE]" inside text: LBRACKET, the value's tokens, RBRACKET. A quote or
// the end of the line before the "]" is an error: the rest of the line is
// skipped, and an RBRACKET with no text tells the parser not to report it again.
// The index after the text in quotes starting at `quote`, or 0 if it doesn't end on the line.
static size_t after_quoted(const Lexer *lx, size_t quote) {
    size_t i = quote + 1;
    while (!lexer_at_line_end(lx, i) && lx->src[i] != '"') i += lx->src[i] == '\\' && !lexer_at_line_end(lx, i + 1) ? 2 : 1;
    return lexer_at_line_end(lx, i) ? 0 : i + 1;
}

// A quote inside a value in brackets starts text of its own ("[the note, or
// "none"]") when that text ends on the line and a "]" comes after it (with
// any more text in quotes skipped); otherwise the "[" is the mistake (a
// missing "]").
static bool quote_in_value(const Lexer *lx) {
    size_t i = after_quoted(lx, lx->pos);
    while (i && !lexer_at_line_end(lx, i)) {
        if (lx->src[i] == ']') return true;
        i = lx->src[i] == '"' ? after_quoted(lx, i) : i + 1;  // more text in quotes in the value
    }
    return false;
}

static void lex_text_value(Lexer *lx) {
    size_t open = lx->pos++;
    lexer_emit_content(lx, TOK_LBRACKET, open, open + 1, "[", 1);
    for (;;) {
        while (lx->pos < lx->len && (lx->src[lx->pos] == ' ' || lx->src[lx->pos] == '\t')) lx->pos++;
        if (lexer_at_line_end(lx, lx->pos) || (lx->src[lx->pos] == '"' && !quote_in_value(lx))) {
            diag_error(lx->diag, lexer_span(open, open + 1),
                       "This \"[\" starts a value inside the text, but there's no \"]\" to end it.");
            diag_note(lx->diag, "End the value with \"]\", like \"Hello, [name]!\". To put a \"[\" in text, write it "
                                "twice: \"[[\".");
            diag_info(lx->diag, DIAG_MISSING, "\"]\"", NULL);
            diag_choice(lx->diag, "]");
            diag_choice(lx->diag, "[[");
            lexer_skip_to_line_end(lx);
            lexer_emit_content(lx, TOK_RBRACKET, lx->pos, lx->pos, "", 0);
            return;
        }
        if (lx->src[lx->pos] == ']') {
            lexer_emit_content(lx, TOK_RBRACKET, lx->pos, lx->pos + 1, "]", 1);
            lx->pos++;
            return;
        }
        lexer_token(lx);
    }
}

// Text in quotes. A value in brackets splits it: STRING, LBRACKET, the
// value's tokens, RBRACKET, STRING (each STRING possibly empty). "[[" is "["
// and "]]" is "]".
void lexer_string(Lexer *lx) {
    size_t start = lx->pos++;
    StrBuf value;
    sb_init(&value, lx->arena);
    while (true) {
        if (lexer_at_line_end(lx, lx->pos)) {
            report_unterminated_string(lx, start);
            break;
        }
        char c = lx->src[lx->pos];
        if (c == '"') {
            lx->pos++;
            break;
        }
        if ((c == '[' || c == ']') && lexer_peek(lx, lx->pos + 1) == c) {
            sb_append_char(&value, c);
            lx->pos += 2;
        } else if (c == ']') {
            diag_error(lx->diag, lexer_span(lx->pos, lx->pos + 1), "This \"]\" doesn't end a value.");
            diag_note(lx->diag, "To put a \"]\" in text, write it twice: \"]]\". A value in text goes between "
                                "\"[\" and \"]\", like \"Hello, [name]!\".");
            diag_info(lx->diag, DIAG_NOT_ALLOWED, NULL, NULL);
            diag_choice(lx->diag, "]]");
            sb_append_char(&value, c);
            lx->pos++;
        } else if (c == '[') {
            lexer_emit_content(lx, TOK_STRING, start, lx->pos, value.data, value.len);
            lex_text_value(lx);
            start = lx->pos;
            sb_init(&value, lx->arena);
            if (vec_last(&lx->tokens).text_len == 0) break;  // a broken value: the line is skipped
        } else if (c == '\\' && !lexer_at_line_end(lx, lx->pos + 1)) {
            lex_escape(lx, &value);
        } else if (c == '\\') {
            lx->pos++;  // a backslash at the end of the line; the text is unterminated
        } else {
            sb_append_char(&value, c);
            lx->pos++;
        }
    }
    lexer_emit_content(lx, TOK_STRING, start, lx->pos, value.data, value.len);
}


// --- Text blocks ---------------------------------------------------------------------

static bool word_is(const Token *token, const char *word) {
    return token->kind == TOK_WORD && strcmp(token->text, word) == 0;
}

// A line ending with "the text below:" (lx->pos at its end): the lines
// indented under it are the text, exactly as written (no escapes, no values
// in brackets), without their shared indentation and joined by new lines.
// It becomes one STRING in place of "text below :"; the lines are skipped.
bool lexer_text_block(Lexer *lx) {
    size_t n = lx->tokens.len;
    if (!lx->line_has_tokens || n < 3 || !word_is(&lx->tokens.items[n - 3], "text") ||
        !word_is(&lx->tokens.items[n - 2], "below") || lx->tokens.items[n - 1].kind != TOK_COLON) {
        return false;
    }
    Token text = lx->tokens.items[n - 3];
    lx->tokens.len -= 3;
    size_t outer = lx->indent, at = lx->pos, last = lx->pos, last_start = lx->line_start, shared = SIZE_MAX;
    for (size_t line = at < lx->len ? at + 1 : at; line < lx->len;) {  // which lines belong to it
        size_t end = line, width = 0;
        while (end < lx->len && lx->src[end] != '\n') end++;
        size_t content = line;
        while (content < end && (lx->src[content] == ' ' || lx->src[content] == '\t')) {
            width += lx->src[content] == '\t' ? 4 : 1;
            content++;
        }
        bool blank = content == end || (content + 1 == end && lx->src[content] == '\r');
        if (!blank && width <= outer) break;
        if (!blank && width < shared) shared = width;
        if (!blank) last = end, last_start = line;
        line = end + 1;
    }
    StrBuf value;
    sb_init(&value, lx->arena);
    size_t used = 0;  // lines taken, up to the last one with words
    for (size_t line = at + 1; line <= last && line < lx->len; used++) {
        size_t end = line;
        while (end < lx->len && lx->src[end] != '\n') end++;
        size_t content = line, width = 0;
        while (content < end && width < shared && (lx->src[content] == ' ' || lx->src[content] == '\t')) {
            width += lx->src[content] == '\t' ? 4 : 1;
            content++;
        }
        size_t stop = end > content && lx->src[end - 1] == '\r' ? end - 1 : end;
        if (value.len || line > at + 1) sb_append_char(&value, '\n');
        sb_append_n(&value, lx->src + content, stop - content);
        line = end + 1;
    }
    lx->pending_filler = text.filler;
    lexer_emit_content(lx, TOK_STRING, text.span.offset, last, value.data ? value.data : "", value.len);
    lx->line += used;  // the next line is the one after the text
    lx->line_start = last_start;
    lx->pos = last;
    return true;
}
