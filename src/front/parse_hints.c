// Sentences EasyScript can't do yet (sending email, making web pages, using
// the internet, reading Excel files), said plainly instead of as a sentence
// it doesn't understand, and notes for the words people bring from other
// tools ("open grades.csv").

#include <string.h>
#include "front/parse_internal.h"

static bool is(const Token *token, const char *word) {
    return token->kind == TOK_WORD && strcmp(token->text, word) == 0;
}

// (Word lists here are counted, not ended by NULL: they're words the
// messages answer, not words EasyScript accepts.)
#define ONE_OF(token, words) one_of(token, words, sizeof words / sizeof words[0])

static bool one_of(const Token *token, const char *const *words, size_t n) {
    for (size_t i = 0; i < n; i++) {
        if (is(token, words[i])) return true;
    }
    return false;
}

// The sentence's tokens from here, up to its end (a period, a line's end or
// a colon that opens a block).
static size_t sentence_length(const Parser *p) {
    size_t n = 0;
    for (;; n++) {
        TokenKind kind = parser_peek(p, n)->kind;
        if (kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_INDENT || kind == TOK_DEDENT) {
            return n;
        }
    }
}

static bool text_starts(const Token *token, const char *start) {
    size_t n = strlen(start);
    return token->kind == TOK_STRING && token->text_len >= n && strncmp(token->text, start, n) == 0;
}

static bool text_ends(const Token *token, const char *end) {
    size_t n = strlen(end);
    if (token->kind != TOK_STRING || token->text_len < n) return false;
    for (size_t i = 0; i < n; i++) {
        if ((token->text[token->text_len - n + i] | 0x20) != end[i]) return false;
    }
    return true;
}

// The first token in the sentence for which `test` holds, or NULL.
static const Token *find(const Parser *p, size_t n, bool (*test)(const Token *, const char *), const char *what) {
    for (size_t i = 0; i < n; i++) {
        if (test(parser_peek(p, i), what)) return parser_peek(p, i);
    }
    return NULL;
}

static bool email_at(const Parser *p, size_t n) {
    static const char *const sends[] = {"send", "email", "mail"};
    static const char *const events[] = {"every", "whenever", "when", "on", "each"};
    if (ONE_OF(parser_peek(p, 0), events)) {  // every monday at 9am email the list to ...
        for (size_t i = 1; i < n; i++) {  // (not the program's own "send")
            if (ONE_OF(parser_peek(p, i), sends) && !parser_match_name(p, i, false)) return true;
        }
        return false;
    }
    static const char *const describing[] = {"is", "are", "must", "has", "have", "can", "depends"};
    if (!ONE_OF(parser_peek(p, 0), sends)) return false;
    const Token *next = parser_peek(p, 1);  // "an email is text that must ...", "email's ..." aren't asking to send one
    if (next->kind == TOK_POSSESSIVE || ONE_OF(next, describing) || (next->kind != TOK_WORD && next->kind != TOK_STRING)) {
        return false;
    }
    if (!is(parser_peek(p, 0), "send")) return true;
    for (size_t i = 1; i < n; i++) {
        const Token *token = parser_peek(p, i);
        if (is(token, "email") || is(token, "mail") || (token->kind == TOK_STRING && memchr(token->text, '@', token->text_len))) {
            return true;
        }
    }
    return false;
}

static bool web_page_at(const Parser *p, size_t n) {
    static const char *const makes[] = {"make", "create", "build", "start", "host", "serve", "publish"};
    static const char *const webs[] = {"page", "site", "server", "form", "app"};
    if (!ONE_OF(parser_peek(p, 0), makes)) return false;
    for (size_t i = 1; i < n; i++) {
        const Token *token = parser_peek(p, i);
        if (parser_match_name(p, i, false)) continue;  // a name the program made, like "website"
        if (is(token, "website") || is(token, "webpage") || is(token, "webserver") ||
            (is(token, "web") && ONE_OF(parser_peek(p, i + 1), webs))) {
            return true;
        }
    }
    return false;
}

static void report(Parser *p, size_t n, const char *message, const char *note) {
    const Token *first = parser_peek(p, 0), *last = parser_peek(p, n ? n - 1 : 0);
    Span span = {first->span.offset, last->span.offset + last->span.length - first->span.offset};
    if (parser_error(p, span, "%s", message)) {
        diag_note(p->diag, "%s", note);
        diag_info(p->diag, DIAG_NOT_ALLOWED, NULL, NULL);
    }
}

// A sentence asking for something EasyScript can't do yet: reported (true),
// or false to read it as usual. A sentence starting with a definition's
// name is always the definition.
bool parser_cant_do_yet(Parser *p) {
    if (parser_peek(p, 0)->kind != TOK_WORD || parser_match_name(p, 0, true)) return false;
    static const char *const reads[] = {"read", "open", "load", "import"};
    static const char *const gets[] = {"read", "open", "get", "fetch", "visit", "download", "load", "look", "go",
                                       "check", "browse"};
    size_t n = sentence_length(p);
    if (email_at(p, n)) {
        report(p, n, "EasyScript can't send email yet.",
               "It can write the text into a file for you to send, like \"write report in file \"report.txt\"\".");
    } else if (web_page_at(p, n)) {
        report(p, n, "EasyScript can't make web pages yet.",
               "It can ask questions in the terminal, like \"ask \"What's your name?\" and call it name\".");
    } else if (ONE_OF(parser_peek(p, 0), gets) &&
               (find(p, n, text_starts, "http://") || find(p, n, text_starts, "https://"))) {
        report(p, n, "EasyScript can't use the internet yet.",
               "It can read files on this computer, like \"read the table in \"data.csv\" and call it data\".");
    } else if (ONE_OF(parser_peek(p, 0), reads) && !is(parser_peek(p, 1), "file") &&
               (find(p, n, text_ends, ".xlsx") || find(p, n, text_ends, ".xls"))) {  // "read file" reads any file as text
        report(p, n, "EasyScript can't read Excel files yet.",
               "Save the sheet as a csv file in Excel (\"Save As\", then \"CSV\"), and read that.");
    } else {
        return false;
    }
    return true;
}

// A note for a sentence starting with "open", "load", "import" or "get":
// the sentence that reads the file. Gives back whether it gave one.
bool parser_note_opening(Parser *p) {
    static const char *const opens[] = {"open", "load", "import", "get"};
    if (!ONE_OF(parser_peek(p, 0), opens)) return false;
    size_t n = sentence_length(p);
    const Token *file = find(p, n, text_ends, ".csv");
    if (!file) file = find(p, n, text_ends, ".tsv");
    if (file) {
        diag_note(p->diag, "To read a table, write \"read the table in \"%s\" and call it %s\".", file->text,
                  parser_table_name(p, file));
        return true;
    }
    for (size_t i = 0; i < n; i++) {
        if (parser_peek(p, i)->kind == TOK_STRING) {
            diag_note(p->diag, "To read a file, write \"read file \"%s\" and call it notes\".", parser_peek(p, i)->text);
            return true;
        }
    }
    return false;
}

// "grades.csv" -> "grades", "best_months.csv" -> "best months": the name a
// table read from the file would get, or "rows" when that isn't a name.
const char *parser_table_name(Parser *p, const Token *file) {
    const char *text = file->text, *slash = strrchr(text, '/');
    if (slash) text = slash + 1;
    const char *dot = strrchr(text, '.');
    size_t len = dot ? (size_t)(dot - text) : strlen(text);
    if (len == 0 || !((text[0] | 0x20) >= 'a' && (text[0] | 0x20) <= 'z')) return "rows";
    char *name = arena_alloc(p->arena, len + 1);
    for (size_t i = 0; i < len; i++) {
        char c = text[i];
        bool ok = (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == ' ';
        if (!ok) return "rows";
        name[i] = c == '_' || c == '-' ? ' ' : (char)(c | (c >= 'A' && c <= 'Z' ? 0x20 : 0));
    }
    return name;
}
