// Names, which can be several words: "let late fee be 25", "keep hourly rate
// as 150", "to add tax to amount:", "count from 1 to 3 as session number:".
//
// Where a sentence introduces a name, the name runs up to the word that
// follows it in that sentence ("be", "to", "as", ...) or to the end of the
// sentence. Everywhere else (inside values, and at the start of a sentence
// for functions), a name of several words is recognized because a pre-scan
// found where the program introduces it; the longest known name wins.
//
// Words that end a name somewhere (be, to, of, with, in, ...), operator words
// (plus, is, and, ...) and value words (yes, it, ...) can't be part of a name
// of several words, so recognizing a name never changes what a sentence means.

#include <string.h>
#include "front/parse_internal.h"

// Words that end a name in some sentence ("let X be", "set X to", "for each
// X in", "area of X", "if X then", "only X that", "to F, giving back").
static const char *const stop_words[] = {"be", "equal", "to", "from", "by", "of", "with", "in",
                                         "at", "then", "that", "giving", NULL};

static bool word_among(const Token *token, const char *const *words) {
    if (token->kind != TOK_WORD || !words) return false;
    for (size_t i = 0; words[i]; i++) {
        if (strcmp(token->text, words[i]) == 0) return true;
    }
    return false;
}

bool parser_is_name_word(const Token *token) {
    return token->kind == TOK_WORD && !parser_is_operator_word(token->text) && !parser_is_value_word(token->text);
}

// A word that can be part of a name of several words.
static bool is_inner_word(const Token *token) {
    return parser_is_name_word(token) && !word_among(token, stop_words);
}

// "late fee" (as matched) or "Late Fee" (as written) for tokens[at, at + n).
static const char *join_words(const Parser *p, size_t at, size_t n, bool written, char separator) {
    StrBuf sb;
    sb_init(&sb, p->arena);
    for (size_t i = 0; i < n; i++) {
        const Token *token = &p->tokens[at + i];
        if (i > 0) sb_append_char(&sb, separator);
        if (written) {
            sb_append_n(&sb, p->source + token->span.offset, token->span.length);
        } else {
            sb_append(&sb, token->text);
        }
    }
    return sb.data;
}

// --- Known names (pre-scan) ------------------------------------------------------

// How many words of a name start at index `i` (the first may be any name word).
static size_t name_words(const Parser *p, size_t i) {
    if (i >= p->count || !parser_is_name_word(&p->tokens[i])) return 0;
    size_t n = 1;
    while (i + n < p->count && is_inner_word(&p->tokens[i + n])) n++;
    return n;
}

static bool word_at(const Parser *p, size_t i, const char *word) {
    return i < p->count && p->tokens[i].kind == TOK_WORD && strcmp(p->tokens[i].text, word) == 0;
}

static void add_known(Parser *p, const char *text, size_t n, bool function, bool to_inputs, bool method) {
    for (size_t i = 0; i < p->names.len; i++) {
        KnownName *known = &p->names.items[i];
        if (strcmp(known->text, text) == 0) {
            if (function && !known->function) vec_push(p->arena, &p->functions, text);
            known->function |= function;
            known->to_inputs |= to_inputs;
            known->method |= method;
            return;
        }
    }
    KnownName known = {text, n, function, to_inputs, method};
    vec_push(p->arena, &p->names, known);
    if (function) vec_push(p->arena, &p->functions, text);
}

static void add_name(Parser *p, size_t at, size_t n, bool function, bool to_inputs) {
    if (n == 0 || (n == 1 && !function)) return;  // one-word variables need no recognizing
    add_known(p, join_words(p, at, n, false, ' '), n, function, to_inputs, false);
}

static bool at_line_start(const Parser *p, size_t index) {
    if (index == 0) return true;
    TokenKind before = p->tokens[index - 1].kind;
    return before == TOK_NEWLINE || before == TOK_INDENT || before == TOK_DEDENT;
}

static bool at_sentence_end_index(const Parser *p, size_t i) {
    if (i >= p->count) return true;
    TokenKind kind = p->tokens[i].kind;
    return kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT || kind == TOK_INDENT;
}

// to NAME [[,] for KIND] [with | of | using | to] INPUT [(KIND)] [and | ,] INPUT ...
static void find_function(Parser *p, size_t i) {
    size_t n = name_words(p, i + 1), words;
    for (size_t k = 1; k < n; k++) {  // a method's name ends before "for" and a kind of record
        if (word_at(p, i + 1 + k, "for") && parser_match_record(p, i + 2 + k, false, &words) >= 0) {
            n = k;
            break;
        }
    }
    if (n == 0) return;
    size_t j = i + 1 + n;
    bool method = (word_at(p, j, "for") && parser_match_record(p, j + 1, false, &words) >= 0) ||
                  (p->tokens[j].kind == TOK_COMMA && word_at(p, j + 1, "for"));
    if (method) {  // skip "for KIND" and a comma after it
        while (j < p->count && p->tokens[j].kind != TOK_COLON && !at_sentence_end_index(p, j) &&
               !word_at(p, j, "with") && !word_at(p, j, "of") && !word_at(p, j, "using") && !word_at(p, j, "to")) {
            j++;
        }
    }
    static const char *const intros[] = {"with", "of", "using", "to", NULL};
    bool introduced = word_among(&p->tokens[j], intros);
    add_known(p, join_words(p, i + 1, n, false, ' '), n, true, introduced && word_at(p, j, "to"), method);
    if (!introduced) return;
    j++;
    for (;;) {
        size_t m = name_words(p, j);
        add_name(p, j, m, false, false);
        j += m;
        if (j < p->count && p->tokens[j].kind == TOK_LPAREN) {  // (KIND)
            while (j < p->count && p->tokens[j].kind != TOK_RPAREN && p->tokens[j].kind != TOK_NEWLINE) j++;
            j++;
        }
        if (m == 0 || j >= p->count || !(word_at(p, j, "and") || p->tokens[j].kind == TOK_COMMA)) return;
        j++;
    }
}

// "dog" -> "dogs", "box" -> "boxes", "berry" -> "berries" (the last word).
static const char *plural_of(Arena *arena, const char *name) {
    size_t len = strlen(name);
    char last = name[len - 1], before = len > 1 ? name[len - 2] : ' ';
    if (last == 'y' && !strchr("aeiou ", before)) return arena_sprintf(arena, "%.*sies", (int)(len - 1), name);
    bool es = strchr("sxz", last) || ((last == 'h') && (before == 'c' || before == 's'));
    return arena_sprintf(arena, "%s%s", name, es ? "es" : "s");
}

// NAME is a kind of thing (the "a" before "kind" is dropped)
static size_t record_definition_words(const Parser *p, size_t i) {
    if (!at_line_start(p, i)) return 0;
    size_t n = name_words(p, i);
    if (n == 0) return 0;
    size_t j = i + n;
    bool kind = word_at(p, j, "is") && word_at(p, j + 1, "kind") && word_at(p, j + 2, "of") && word_at(p, j + 3, "thing");
    return kind ? n : 0;
}

// Every record kind, in order, and every field name: "NAME, which is".
static void find_records(Parser *p) {
    for (size_t i = 0; i < p->count; i++) {
        size_t n = record_definition_words(p, i);
        if (n > 0) {
            const char *name = join_words(p, i, n, false, ' ');
            bool known = false;
            for (size_t r = 0; r < p->records->len; r++) known |= strcmp(p->records->items[r].name, name) == 0;
            if (!known) {
                RecordKind kind = {name, join_words(p, i, n, true, ' '), plural_of(p->arena, name), NULL};
                vec_push(p->arena, p->records, kind);
                add_known(p, name, n, false, false, false);  // a method's input is called this
            }
            continue;
        }
        n = at_line_start(p, i) ? name_words(p, i) : 0;
        if (n > 0 && i + n + 2 < p->count && p->tokens[i + n].kind == TOK_COMMA && word_at(p, i + n + 1, "which") &&
            word_at(p, i + n + 2, "is")) {
            KnownName field = {join_words(p, i, n, false, ' '), n, false, false, false};
            vec_push(p->arena, &p->fields, field);
        }
    }
}

bool parser_is_method(const Parser *p, const char *name) {
    for (size_t i = 0; i < p->names.len; i++) {
        if (strcmp(p->names.items[i].text, name) == 0) return p->names.items[i].method;
    }
    return false;
}

void parser_find_names(Parser *p) {
    find_records(p);
    for (size_t i = 0; i + 1 < p->count; i++) {
        if (p->tokens[i].kind != TOK_WORD) continue;
        const char *word = p->tokens[i].text;
        size_t n = name_words(p, i + 1), after = i + 1 + n;
        if (strcmp(word, "to") == 0 && at_line_start(p, i)) {
            find_function(p, i);
        } else if (strcmp(word, "let") == 0 && (word_at(p, after, "be") || word_at(p, after, "equal"))) {
            add_name(p, i + 1, n, false, false);
        } else if (strcmp(word, "keep") == 0 && (word_at(p, after, "as") || word_at(p, after, "at"))) {
            add_name(p, i + 1, n, false, false);
        } else if (strcmp(word, "as") == 0 && after < p->count && p->tokens[after].kind == TOK_COLON) {
            add_name(p, i + 1, n, false, false);  // count ... as NAME:
        } else if (strcmp(word, "each") == 0 && i > 0 && word_at(p, i - 1, "for") && word_at(p, after, "and") &&
                   word_at(p, after + 1 + name_words(p, after + 1), "in")) {  // for each NAME and NAME in
            add_name(p, i + 1, n, false, false);
            add_name(p, after + 1, name_words(p, after + 1), false, false);
        } else if ((strcmp(word, "each") == 0 && i > 0 && word_at(p, i - 1, "for") &&
                    (word_at(p, after, "in") || word_at(p, after, "from"))) ||
                   (strcmp(word, "every") == 0 && word_at(p, after, "in") &&
                    !(n == 2 && word_at(p, i + 2, "item")))) {  // not "every other item in" (a slice)
            add_name(p, i + 1, n, false, false);
        } else if (strcmp(word, "call") == 0) {  // and call the answer / call it / call each number NAME
            size_t skip = word_at(p, i + 1, "answer") || word_at(p, i + 1, "it")          ? 1
                          : word_at(p, i + 1, "each") && word_at(p, i + 2, "number") ? 2
                                                                                      : 0;
            size_t m = skip ? name_words(p, i + 1 + skip) : 0;
            if (m && (at_sentence_end_index(p, i + 1 + skip + m) || p->tokens[i + 1 + skip + m].kind == TOK_COLON)) {
                add_name(p, i + 1 + skip, m, false, false);
            }
        }
    }
}

const KnownName *parser_match_name(const Parser *p, size_t ahead, bool functions_only) {
    const KnownName *best = NULL;
    for (size_t i = 0; i < p->names.len; i++) {
        const KnownName *known = &p->names.items[i];
        if ((functions_only && !known->function) || (best && known->words <= best->words)) continue;
        const char *text = known->text;
        size_t w = 0;
        for (; w < known->words; w++) {
            const Token *token = parser_peek(p, ahead + w);
            size_t len = strcspn(text, " ");
            if (token->kind != TOK_WORD || token->text_len != len || memcmp(token->text, text, len) != 0) break;
            text += len + (text[len] == ' ');
        }
        if (w == known->words) best = known;
    }
    return best;
}

bool parser_is_function(const Parser *p, const char *name) {
    for (size_t i = 0; i < p->functions.len; i++) {
        if (strcmp(p->functions.items[i], name) == 0) return true;
    }
    return false;
}

void parser_take_name(Parser *p, size_t words, Name *name) {
    size_t at = p->pos;
    name->text = join_words(p, at, words, false, ' ');
    name->display = join_words(p, at, words, true, ' ');
    SourcePos first = parser_token_pos(parser_peek(p, 0));
    parser_consume(p, words);
    name->pos = ast_pos_join(first, (SourcePos){p->prev_span, 0, 0});
}

// --- Names where a sentence introduces or changes one --------------------------------

static void report_filler_name(Parser *p, Span filler) {
    if (parser_error(p, filler, "\"%s\" can't be used as a name.", parser_text(p, filler))) {
        diag_note(p->diag, "EasyScript ignores the words \"a\", \"an\" and \"the\" wherever they appear, "
                           "so they can't be names.");
        diag_note(p->diag, "Try a name that says what it holds, like \"total\" or \"answer\".");
    }
}

static void report_reserved_name(Parser *p, const Token *token) {
    if (parser_error(p, token->span, "\"%s\" can't be used as a name.", token->text)) {
        diag_note(p->diag, "\"%s\" already means something in EasyScript.", token->text);
        diag_note(p->diag, "Try a different name, like \"my_%s\".", token->text);
    }
}

static void report_name_missing_after_prev(Parser *p) {
    if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Add a name there, like \"total\".");
    }
}

// The name's n words are followed by a word that means something else and
// then more name words ("let price plus tax be", "rate of pay"): the name
// can't contain it. Suggests the words joined with underscores.
static bool reject_reserved_inside(Parser *p, size_t n, const char *const *next) {
    const Token *word = parser_peek(p, n);
    if (word->kind != TOK_WORD || word_among(word, next) || is_inner_word(word) || !is_inner_word(parser_peek(p, n + 1))) {
        return false;
    }
    size_t total = n + 1;
    while (is_inner_word(parser_peek(p, total)) && !word_among(parser_peek(p, total), next)) total++;
    if (parser_error(p, word->span, "\"%s\" can't be part of a name, because it already means something in EasyScript.",
                     word->text)) {
        diag_note(p->diag, "Try a name without it, like \"%s\".", join_words(p, p->pos, total, true, '_'));
    }
    return true;
}

// Parses a name: one word, or several up to one of `next` (the words that
// follow the name in this sentence, like "be" after "let X"), or to the end
// of the sentence when `next` is NULL. A filler word dropped right before
// `next` ("let a be 5") was meant as the name.
bool parser_parse_name(Parser *p, Name *name, const char *const *next) {
    const Token *token = parser_peek(p, 0);
    bool is_next_word = word_among(token, next);
    if (parser_is_name_word(token) && !is_next_word) {
        size_t n = 1;
        while (is_inner_word(parser_peek(p, n)) && !word_among(parser_peek(p, n), next)) n++;
        if (reject_reserved_inside(p, n, next)) return false;
        parser_take_name(p, n, name);
        return true;
    }
    if (token->filler.length > 0 && (!parser_is_name_word(token) || is_next_word)) {
        report_filler_name(p, token->filler);
    } else if (token->kind == TOK_WORD && !is_next_word) {
        report_reserved_name(p, token);
    } else if (is_next_word || parser_at_sentence_end(p) || parser_at(p, 0, TOK_COLON)) {
        report_name_missing_after_prev(p);
    } else if (parser_error(p, token->span, "I expected a name here, but found %s.", parser_describe(p, token))) {
        diag_note(p->diag, "A name is a single word, like \"total\" or \"player_2\", or several, like \"late fee\".");
    }
    return false;
}
