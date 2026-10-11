/* EasyScript runtime, part 6: text tools. Parts 1 to 5 (es_memory.h,
 * es_program.h, es_runtime.h, es_list.h, es_sort.h) come first.
 *
 * A character is a Unicode code point, stored as UTF-8: "café" has 4
 * characters and 5 bytes. Positions count characters from 1, like list
 * items, and taking characters uses the same rules as taking items (see
 * es_list.h). Text that's plain ASCII (its character count, kept in the
 * text, equals its length) is worked on byte by byte; other text is scanned
 * for where its characters start.
 *
 * Text never changes: every function here makes new text (or a list) and,
 * like the rest of the runtime, consumes the text it's given. A single ASCII
 * character is a static immortal text (es_ascii_text), so taking the
 * characters of ASCII text allocates nothing but the list.
 *
 * Letters, uppercase and lowercase cover English, the accented Latin letters
 * (Latin-1 and Latin Extended-A), Greek and Cyrillic; other characters are
 * not letters and don't change case. Digits are 0 to 9. Spaces are space,
 * tab, new line, carriage return, form feed and vertical tab. */
#ifndef ES_TEXT_H
#define ES_TEXT_H

#ifndef ES_LIST_H
#include "es_list.h"
#endif

/* --- Characters ---------------------------------------------------------------- */

static bool es_text_ascii(EsText *t) {
    return es_text_chars(t) == t->len;
}

/* One ASCII character as text: static, immortal, made the first time it's needed. */
static EsText es_ascii[128];
static char es_ascii_chars[256];

static EsText *es_ascii_text(unsigned char c) {
    EsText *t = &es_ascii[c];
    if (t->len == 0) {
        es_ascii_chars[2 * c] = (char)c;
        t->header.kind = ES_OBJ_TEXT;
        t->header.refs = ES_IMMORTAL;
        t->len = 1;
        t->chars = &es_ascii_chars[2 * c];
        t->count = 1;
    }
    return t;
}

/* New text from n bytes that hold `chars` characters. */
static EsText *es_text_piece(const char *p, size_t n, size_t chars) {
    if (n == 0) return &es_empty_text;
    if (n == 1 && (unsigned char)p[0] < 128) return es_ascii_text((unsigned char)p[0]);
    EsText *t = es_text_copy(p, n);
    t->count = chars;
    return t;
}

/* Where character i (from 0) starts, in bytes; t's length for i = its count. */
static size_t es_char_offset(EsText *t, size_t i) {
    if (es_text_ascii(t)) return i;
    size_t k = 0;
    for (size_t at = 0; at < t->len; at++) {
        if (es_continuation_byte(t->chars[at])) continue;
        if (k++ == i) return at;
    }
    return t->len;
}

/* The bytes of character i, from es_char_offset's start. */
static size_t es_char_bytes(const EsText *t, size_t start) {
    size_t end = start + 1;
    while (end < t->len && es_continuation_byte(t->chars[end])) end++;
    return end - start;
}

/* Characters i to i + n - 1 as new text (consuming t). */
static EsText *es_text_keep(EsText *t, size_t i, size_t n) {
    if (i == 0 && n == es_text_chars(t)) return t;
    size_t start = es_char_offset(t, i);
    size_t end = es_text_ascii(t) ? start + n : start;
    for (size_t k = 0; k < n && !es_text_ascii(t); k++) end += es_char_bytes(t, end);
    EsText *piece = es_text_piece(t->chars + start, end - start, n);
    es_release(t);
    return piece;
}

static _Noreturn void es_bad_char_position(int site, double pos) {
    es_fail(site, NULL, "The position of a character has to be a whole number, but it's %s.", es_shown(pos));
}

/* The index (from 0) of character `pos`, the first or the last, in text of n characters. */
static size_t es_char_index(int site, size_t n, int where, double pos) {
    if (where != ES_AT) {
        if (n == 0) es_fail(site, NULL, "There's no %s character: the text is empty.", where == ES_FIRST ? "first" : "last");
        return where == ES_FIRST ? 0 : n - 1;
    }
    if (pos != floor(pos)) es_bad_char_position(site, pos);
    if (!(pos >= 1 && pos <= (double)n)) {
        const char *hint = pos < 1 ? "Positions start at 1: the first character is character 1." : NULL;
        if (n == 0) es_fail(site, hint, "There's no character %s in the text: it's empty.", es_shown(pos));
        es_fail(site, hint, "There's no character %s in the text: it has %zu character%s.", es_shown(pos), n,
                n == 1 ? "" : "s");
    }
    return (size_t)pos - 1;
}

/* "character P of T", "the first character of T", "the last character of T" */
static EsText *es_char_at(int site, EsText *t, int where, double pos) {
    return es_text_keep(t, es_char_index(site, es_text_chars(t), where, pos), 1);
}

/* "characters FROM to TO of T" (to_end: "to the end") */
static EsText *es_text_range(int site, EsText *t, double from, double to, bool to_end) {
    size_t n = es_text_chars(t);
    size_t a = es_char_index(site, n, ES_AT, from);
    size_t b = to_end ? n - 1 : es_char_index(site, n, ES_AT, to);
    if (a > b) {
        es_fail(site, NULL, "I can't take characters %zu to %zu: the first position is after the last one.", a + 1, b + 1);
    }
    return es_text_keep(t, a, b - a + 1);
}

/* "the first N characters of T" / "the last N characters of T": all of them when there are fewer. */
static EsText *es_text_end(int site, EsText *t, double count, bool last) {
    if (count < 0) es_fail(site, NULL, "The number of characters can't be negative, but it's %s.", es_shown(count));
    if (count != floor(count)) {
        es_fail(site, NULL, "The number of characters has to be a whole number, but it's %s.", es_shown(count));
    }
    size_t n = es_text_chars(t);
    size_t k = count >= (double)n ? n : (size_t)count;
    return es_text_keep(t, last ? n - k : 0, k);
}

/* "T without the first character" / "T without the last character" */
static EsText *es_text_without(int site, EsText *t, bool last) {
    size_t n = es_text_chars(t);
    es_char_index(site, n, last ? ES_LAST : ES_FIRST, 0); /* there has to be one */
    return es_text_keep(t, last ? 0 : 1, n - 1);
}

/* "every second character of T": characters step, 2 * step, ...;
 * "every other character of T": 1, 3, 5, ... (start is the first position). */
static EsText *es_text_every(EsText *t, size_t step, size_t start) {
    size_t n = es_text_chars(t);
    EsBuf b = {NULL, 0, 0};
    size_t kept = 0;
    for (size_t i = start - 1, at = es_char_offset(t, i); i < n; i += step) {
        size_t bytes = es_char_bytes(t, at);
        es_buf_add(&b, t->chars + at, bytes);
        kept++;
        for (size_t k = 0; k < step && at < t->len; k++) at += es_char_bytes(t, at);
    }
    EsText *result = kept > 0 ? es_text_piece(b.data, b.len, kept) : &es_empty_text;
    free(b.data);
    es_release(t);
    return result;
}

/* --- Lists from text --------------------------------------------------------------- */

/* "the characters of T": a list of text, one character each. */
static EsList *es_text_characters(EsText *t) {
    size_t n = es_text_chars(t);
    EsList *l = es_list_new(ES_ITEMS_TEXT, n);
    for (size_t at = 0; at < t->len;) {
        size_t bytes = es_char_bytes(t, at);
        l->at.t[l->len++] = es_text_piece(t->chars + at, bytes, 1);
        at += bytes;
    }
    es_release(t);
    return l;
}

static bool es_space_byte(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\v';
}

/* Adds bytes [start, end) of t to the list as new text. */
static void es_push_piece(EsList *l, const EsText *t, size_t start, size_t end) {
    size_t chars = 0;
    for (size_t i = start; i < end; i++) chars += !es_continuation_byte(t->chars[i]);
    es_push_t(l, es_text_piece(t->chars + start, end - start, chars));
}

/* "the words of T": the pieces between spaces (any number of them). */
static EsList *es_text_words(EsText *t) {
    EsList *l = es_list_new(ES_ITEMS_TEXT, 0);
    for (size_t i = 0; i < t->len;) {
        while (i < t->len && es_space_byte(t->chars[i])) i++;
        size_t start = i;
        while (i < t->len && !es_space_byte(t->chars[i])) i++;
        if (i > start) es_push_piece(l, t, start, i);
    }
    es_list_shrink(l);
    es_release(t);
    return l;
}

/* "the lines of T": the pieces between new lines (a "\r" before one is
 * dropped). A new line at the very end doesn't start another line, and
 * empty text has no lines. */
static EsList *es_text_lines(EsText *t) {
    EsList *l = es_list_new(ES_ITEMS_TEXT, 0);
    for (size_t i = 0; i < t->len;) {
        size_t start = i;
        while (i < t->len && t->chars[i] != '\n') i++;
        size_t end = i > start && t->chars[i - 1] == '\r' ? i - 1 : i;
        es_push_piece(l, t, start, end);
        i++; /* past the new line */
    }
    es_list_shrink(l);
    es_release(t);
    return l;
}

/* Where `x` next appears in t at or after byte `from`, or t's length. Only looks. */
static size_t es_find(const EsText *t, const EsText *x, size_t from) {
    if (x->len == 0) return from;
    for (size_t i = from; i + x->len <= t->len; i++) {
        const char *hit = memchr(t->chars + i, x->chars[0], t->len - x->len + 1 - i);
        if (!hit) break;
        i = (size_t)(hit - t->chars);
        if (memcmp(hit, x->chars, x->len) == 0) return i;
    }
    return t->len;
}

/* "T split by S": the pieces between each S (empty ones too). Empty text has no pieces. */
static EsList *es_text_split(int site, EsText *t, EsText *by) {
    if (by->len == 0) {
        es_fail(site, "To take each character, write \"the characters of ...\".", "I can't split text by empty text.");
    }
    EsList *l = es_list_new(ES_ITEMS_TEXT, 0);
    for (size_t start = 0; t->len > 0;) {
        size_t at = es_find(t, by, start);
        es_push_piece(l, t, start, at);
        if (at == t->len) break;
        start = at + by->len;
    }
    es_list_shrink(l);
    es_release(by);
    es_release(t);
    return l;
}

/* "L joined with S": the items as text, with S between each two. */
static EsText *es_list_joined(EsList *l, EsText *with) {
    EsBuf b = {NULL, 0, 0};
    for (size_t i = 0; i < l->len; i++) {
        if (i > 0) es_buf_add(&b, with->chars, with->len);
        EsText *item = l->items == ES_ITEMS_NUMBER  ? es_number_text(l->at.n[i])
                       : l->items == ES_ITEMS_YESNO ? es_yesno_text(l->at.b[i])
                                                    : l->at.t[i];
        es_buf_add(&b, item->chars, item->len);
        if (l->items == ES_ITEMS_NUMBER) es_release(item);
    }
    EsText *t = b.len > 0 ? es_text_copy(b.data, b.len) : &es_empty_text;
    free(b.data);
    es_release(with);
    es_list_release(l);
    return t;
}

/* --- Searching ---------------------------------------------------------------------- */

/* "T contains X" */
static bool es_text_contains(EsText *t, EsText *x) {
    bool found = es_find(t, x, 0) < t->len || x->len == 0;
    es_release(x);
    es_release(t);
    return found;
}

/* "T starts with X" / "T ends with X" */
static bool es_text_starts(EsText *t, EsText *x, bool end) {
    bool yes = x->len <= t->len && memcmp(t->chars + (end ? t->len - x->len : 0), x->chars, x->len) == 0;
    es_release(x);
    es_release(t);
    return yes;
}

/* "the position of X in T": the character where X first starts, or 0 if
 * it isn't there (empty text is found at position 1). */
static double es_text_position(EsText *x, EsText *t) {
    size_t at = es_find(t, x, 0);
    double position = 0;
    if (at < t->len || x->len == 0) {
        size_t chars = 0;
        for (size_t i = 0; i < at; i++) chars += !es_continuation_byte(t->chars[i]);
        position = (double)chars + 1;
    }
    es_release(x);
    es_release(t);
    return position;
}

/* --- Changing text (into new text) -------------------------------------------------- */

/* "T with X replaced by Y": every X, from the left, without overlaps. */
static EsText *es_text_replace(int site, EsText *t, EsText *x, EsText *y) {
    if (x->len == 0) es_fail(site, NULL, "I can't replace empty text: there's nothing to look for.");
    size_t at = es_find(t, x, 0);
    EsText *result = t;
    if (at < t->len) {
        EsBuf b = {NULL, 0, 0};
        for (size_t start = 0;; at = es_find(t, x, start)) {
            es_buf_add(&b, t->chars + start, at - start);
            if (at == t->len) break;
            es_buf_add(&b, y->chars, y->len);
            start = at + x->len;
        }
        result = b.len > 0 ? es_text_copy(b.data, b.len) : &es_empty_text;
        free(b.data);
        es_release(t);
    }
    es_release(x);
    es_release(y);
    return result;
}

/* "T without extra spaces": without the spaces at its start and end. */
static EsText *es_text_trim(EsText *t) {
    size_t start = 0, end = t->len;
    while (start < end && es_space_byte(t->chars[start])) start++;
    while (end > start && es_space_byte(t->chars[end - 1])) end--;
    if (start == 0 && end == t->len) return t;
    size_t chars = 0;
    for (size_t i = start; i < end; i++) chars += !es_continuation_byte(t->chars[i]);
    EsText *trimmed = es_text_piece(t->chars + start, end - start, chars);
    es_release(t);
    return trimmed;
}

/* The code point that starts at p (n bytes in all). */
static uint32_t es_decode(const char *p, size_t n) {
    unsigned char c = (unsigned char)p[0];
    if (n == 1) return c;
    uint32_t cp = c & (0x7F >> n);
    for (size_t i = 1; i < n; i++) cp = (cp << 6) | ((unsigned char)p[i] & 0x3F);
    return cp;
}

/* Appends code point cp as UTF-8. */
static void es_encode(EsBuf *b, uint32_t cp) {
    char out[4];
    size_t n;
    if (cp < 0x80) {
        out[0] = (char)cp, n = 1;
    } else if (cp < 0x800) {
        out[0] = (char)(0xC0 | (cp >> 6)), out[1] = (char)(0x80 | (cp & 0x3F)), n = 2;
    } else if (cp < 0x10000) {
        out[0] = (char)(0xE0 | (cp >> 12)), out[1] = (char)(0x80 | ((cp >> 6) & 0x3F));
        out[2] = (char)(0x80 | (cp & 0x3F)), n = 3;
    } else {
        out[0] = (char)(0xF0 | (cp >> 18)), out[1] = (char)(0x80 | ((cp >> 12) & 0x3F));
        out[2] = (char)(0x80 | ((cp >> 6) & 0x3F)), out[3] = (char)(0x80 | (cp & 0x3F)), n = 4;
    }
    es_buf_add(b, out, n);
}

/* The uppercase letter of cp, or cp itself. */
static uint32_t es_upper(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') return cp - 32;
    if (cp < 0xB5) return cp;
    if ((cp >= 0xE0 && cp <= 0xFE && cp != 0xF7) || (cp >= 0x3B1 && cp <= 0x3C9 && cp != 0x3C2) ||
        (cp >= 0x430 && cp <= 0x44F)) {
        return cp - 32;
    }
    if (cp == 0xFF) return 0x178;
    if (cp == 0x3C2) return 0x3A3; /* final sigma */
    if (cp >= 0x450 && cp <= 0x45F) return cp - 80;
    if (cp == 0x3AC) return 0x386;
    if (cp >= 0x3AD && cp <= 0x3AF) return cp - 37;
    if (cp == 0x3CC) return 0x38C;
    if (cp == 0x3CD || cp == 0x3CE) return cp - 63;
    if ((cp >= 0x100 && cp <= 0x12F) || (cp >= 0x132 && cp <= 0x137) || (cp >= 0x14A && cp <= 0x177)) {
        return cp & ~1u; /* pairs: upper even, lower odd */
    }
    if ((cp >= 0x139 && cp <= 0x148) || (cp >= 0x179 && cp <= 0x17E)) return cp % 2 == 0 ? cp - 1 : cp;
    return cp;
}

/* The lowercase letter of cp, or cp itself. */
static uint32_t es_lower(uint32_t cp) {
    if (cp >= 'A' && cp <= 'Z') return cp + 32;
    if (cp < 0xC0) return cp;
    if ((cp >= 0xC0 && cp <= 0xDE && cp != 0xD7) || (cp >= 0x391 && cp <= 0x3A9 && cp != 0x3A2) ||
        (cp >= 0x410 && cp <= 0x42F)) {
        return cp + 32;
    }
    if (cp == 0x178) return 0xFF;
    if (cp >= 0x400 && cp <= 0x40F) return cp + 80;
    if (cp == 0x386) return 0x3AC;
    if (cp >= 0x388 && cp <= 0x38A) return cp + 37;
    if (cp == 0x38C) return 0x3CC;
    if (cp == 0x38E || cp == 0x38F) return cp + 63;
    if ((cp >= 0x100 && cp <= 0x12F) || (cp >= 0x132 && cp <= 0x137) || (cp >= 0x14A && cp <= 0x177)) return cp | 1u;
    if ((cp >= 0x139 && cp <= 0x148) || (cp >= 0x179 && cp <= 0x17E)) return cp % 2 == 1 ? cp + 1 : cp;
    return cp;
}

/* "T in uppercase" / "T in lowercase" */
static EsText *es_text_case(EsText *t, bool upper) {
    if (es_text_ascii(t)) {
        size_t i = 0;
        while (i < t->len && t->chars[i] == (char)(upper ? es_upper : es_lower)((unsigned char)t->chars[i])) i++;
        if (i == t->len) return t; /* nothing changes */
        char *p;
        EsText *changed = es_text_new(t->len, &p);
        for (size_t k = 0; k < t->len; k++) p[k] = (char)(upper ? es_upper : es_lower)((unsigned char)t->chars[k]);
        changed->count = t->len;
        es_release(t);
        return changed;
    }
    EsBuf b = {NULL, 0, 0};
    for (size_t at = 0; at < t->len;) {
        size_t n = es_char_bytes(t, at);
        uint32_t cp = es_decode(t->chars + at, n);
        uint32_t to = upper ? es_upper(cp) : es_lower(cp);
        if (to == cp) {
            es_buf_add(&b, t->chars + at, n); /* unchanged, byte for byte */
        } else {
            es_encode(&b, to);
        }
        at += n;
    }
    EsText *changed = es_text_piece(b.data, b.len, es_text_chars(t));
    free(b.data);
    es_release(t);
    return changed;
}

/* --- Characters and their codes --------------------------------------------------- */

/* Text of exactly one character: its code point; otherwise UINT32_MAX. Only looks. */
static uint32_t es_single(EsText *t) {
    if (es_text_chars(t) != 1) return UINT32_MAX;
    size_t start = es_char_offset(t, 0);
    return es_decode(t->chars + start, es_char_bytes(t, start));
}

/* "the code of C": the number of its one character. */
static double es_text_code(int site, EsText *t) {
    uint32_t cp = es_single(t);
    if (cp == UINT32_MAX) {
        if (t->len == 0) es_fail(site, NULL, "The code of a character needs exactly one character, but this text is empty.");
        es_fail(site, NULL, "The code of a character needs exactly one character, but this text has %zu.",
                es_text_chars(t));
    }
    es_release(t);
    return (double)cp;
}

/* "the character with code N" */
static EsText *es_text_from_code(int site, double code) {
    if (!(code >= 1 && code <= 0x10FFFF) || code != floor(code) || (code >= 0xD800 && code <= 0xDFFF)) {
        es_fail(site, "Character codes are whole numbers from 1 to 1114111, like 65 for \"A\".",
                "There's no character with code %s.", es_shown(code));
    }
    if (code < 128) return es_ascii_text((unsigned char)code);
    EsBuf b = {NULL, 0, 0};
    es_encode(&b, (uint32_t)code);
    EsText *t = es_text_piece(b.data, b.len, 1);
    free(b.data);
    return t;
}

static bool es_letter_code(uint32_t cp) {
    if ((cp >= 'A' && cp <= 'Z') || (cp >= 'a' && cp <= 'z')) return true;
    if (cp < 0xC0) return cp == 0xAA || cp == 0xBA;
    if (cp <= 0x24F) return cp != 0xD7 && cp != 0xF7; /* Latin-1 letters, Latin Extended-A and B */
    if (cp >= 0x386 && cp <= 0x3CE) return cp != 0x387 && cp != 0x38B && cp != 0x38D && cp != 0x3A2;
    return (cp >= 0x400 && cp <= 0x481) || (cp >= 0x48A && cp <= 0x52F);
}

/* "C is a letter", "C is a digit", "C is a space": yes only for text of exactly one such character. */
static bool es_text_is_letter(EsText *t) {
    uint32_t cp = es_single(t);
    es_release(t);
    return cp != UINT32_MAX && es_letter_code(cp);
}

static bool es_text_is_digit(EsText *t) {
    bool yes = t->len == 1 && es_is_digit(t->chars[0]);
    es_release(t);
    return yes;
}

static bool es_text_is_space(EsText *t) {
    bool yes = t->len == 1 && es_space_byte(t->chars[0]);
    es_release(t);
    return yes;
}

#endif
