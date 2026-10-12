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

static void add_known(Parser *p, const char *text, size_t n, bool function, unsigned intros, bool method) {
    for (size_t i = 0; i < p->names.len; i++) {
        KnownName *known = &p->names.items[i];
        if (strcmp(known->text, text) == 0) {
            if (function && !known->function) vec_push(p->arena, &p->functions, text);
            known->function |= function;
            known->intros |= intros;
            known->method |= method;
            return;
        }
    }
    KnownName known = {text, n, function, intros, method};
    vec_push(p->arena, &p->names, known);
    if (function) vec_push(p->arena, &p->functions, text);
}

static void add_name(Parser *p, size_t at, size_t n, bool function) {
    if (n == 0 || (n == 1 && !function)) return;  // one-word variables need no recognizing
    add_known(p, join_words(p, at, n, false, ' '), n, function, 0, false);
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
// (or KIND can NAME ...: `can` is at i, and it's a method)
static void find_function(Parser *p, size_t i, bool can) {
    size_t n = name_words(p, i + 1), words;
    for (size_t k = 1; k < n; k++) {  // the name ends before "for" (a method's kind, or inputs)
        if (word_at(p, i + 1 + k, "for")) {
            n = k;
            break;
        }
    }
    if (n == 0) return;
    size_t j = i + 1 + n;
    bool method = (word_at(p, j, "for") && parser_match_kind_name(p, j + 1, false, &words)) ||
                  (p->tokens[j].kind == TOK_COMMA && word_at(p, j + 1, "for"));
    if (can) {
        method = true;
    } else if (method) {  // skip "for KIND" and a comma after it
        j += p->tokens[j].kind == TOK_COMMA ? 2 : 1;
        while (j < p->count && p->tokens[j].kind != TOK_COLON && !at_sentence_end_index(p, j) &&
               (word_at(p, j, "for") || !word_among(&p->tokens[j], parser_input_intros))) {
            j++;
        }
    }
    bool introduced = word_among(&p->tokens[j], parser_input_intros);
    add_known(p, join_words(p, i + 1, n, false, ' '), n, true, introduced ? parser_intro_bit(p->tokens[j].text) : 0,
              method);
    if (!introduced) return;
    j++;
    for (;;) {
        size_t m = name_words(p, j);
        add_name(p, j, m, false);
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
const char *parser_plural_of(Arena *arena, const char *name) {
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

// "a dog has a name (text) and an age.": the kind's words, at the start of a
// line before "has" and a field's name (0 if it isn't one). The first word
// can't start sentences, so "if ages has ..." and "while ..." stay sentences.
size_t parser_has_definition_words(const Parser *p, size_t i) {
    if (!at_line_start(p, i) || !parser_is_name_word(&p->tokens[i]) || parser_is_statement_word(p->tokens[i].text) ||
        word_at(p, i, "otherwise") || word_at(p, i, "else") || word_at(p, i, "when")) {
        return 0;
    }
    size_t n = 1;
    while (i + n < p->count && is_inner_word(&p->tokens[i + n]) && !word_at(p, i + n, "has")) n++;
    return word_at(p, i + n, "has") && (at_sentence_end_index(p, i + n + 1) || parser_is_name_word(&p->tokens[i + n + 1]))
               ? n
               : 0;
}

static void add_field_name(Parser *p, size_t i, size_t n);

// The fields after "has": NAME [(...)] separated by "," and "and". Returns
// the index after the sentence.
static size_t find_has_fields(Parser *p, size_t i) {
    for (;;) {
        if (word_at(p, i, "some") && name_words(p, i) > 1) i++;  // "some pets": like "a" or "an"
        size_t n = name_words(p, i);
        if (n == 0) return i;
        add_field_name(p, i, n);
        i += n;
        if (i < p->count && p->tokens[i].kind == TOK_LPAREN) {
            while (i < p->count && p->tokens[i].kind != TOK_RPAREN && !at_sentence_end_index(p, i)) i++;
            if (i < p->count && p->tokens[i].kind == TOK_RPAREN) i++;
        }
        if (i < p->count && p->tokens[i].kind == TOK_COMMA) i++;
        if (word_at(p, i, "and")) i++;
        if (at_sentence_end_index(p, i)) return i;
    }
}

static void add_field_name(Parser *p, size_t i, size_t n) {
    for (size_t k = 0; k < p->fields.len; k++) {
        if (p->fields.items[k].words == n && strcmp(p->fields.items[k].text, join_words(p, i, n, false, ' ')) == 0) return;
    }
    KnownName field = {join_words(p, i, n, false, ' '), n, false, false, false};
    vec_push(p->arena, &p->fields, field);
}

// A kind (record or with variants) named by tokens [i, i + n): adds it,
// unless one has the name already (defining it twice is the checker's
// error). Returns whether it was added.
static bool add_kind(Parser *p, size_t i, size_t n, bool sum) {
    const char *name = join_words(p, i, n, false, ' ');
    for (size_t r = 0; r < p->records->len; r++) {
        if (strcmp(p->records->items[r].name, name) == 0 && !sum) return false;
    }
    for (size_t r = 0; r < p->records->sums.len; r++) {
        if (strcmp(p->records->sums.items[r].name, name) == 0 && sum) return false;
    }
    RecordKind kind = {name, join_words(p, i, n, true, ' '), parser_plural_of(p->arena, name), NULL};
    if (sum) {
        vec_push(p->arena, &p->records->sums, kind);
    } else {
        vec_push(p->arena, p->records, kind);
    }
    add_known(p, name, n, false, false, false);  // a method's input is called this
    return true;
}

// NAME is one of: (the "a" before NAME is dropped)
static size_t sum_definition_words(const Parser *p, size_t i) {
    if (!at_line_start(p, i)) return 0;
    size_t n = name_words(p, i);
    size_t j = i + n;
    bool sum = n > 0 && word_at(p, j, "is") && word_at(p, j + 1, "one") && word_at(p, j + 2, "of") &&
               j + 3 < p->count && p->tokens[j + 3].kind == TOK_COLON;
    return sum ? n : 0;
}

// NAME is made of ...: a kind with one variant of the same name.
size_t parser_made_of_words(const Parser *p, size_t i) {
    if (!at_line_start(p, i)) return 0;
    size_t n = name_words(p, i);
    return n > 0 && word_at(p, i + n, "is") && word_at(p, i + n + 1, "made") && word_at(p, i + n + 2, "of") ? n : 0;
}

static void find_variant_fields(Parser *p, size_t j);

// One variant's line: NAME [with FIELD (KIND) [and | ,] FIELD (KIND) ...].
static void find_variant(Parser *p, size_t i, size_t sum) {
    size_t n = name_words(p, i);
    if (n == 0) return;
    VariantKind variant = {join_words(p, i, n, false, ' '), join_words(p, i, n, true, ' '), sum, 0};
    for (size_t v = 0; v < p->records->variants.len; v++) variant.index += p->records->variants.items[v].sum == sum;
    vec_push(p->arena, &p->records->variants, variant);
    size_t j = i + n;
    if (word_at(p, j, "with")) find_variant_fields(p, j + 1);
}

// FIELD (KIND) [and | ,] FIELD (KIND) ...: the field names.
static void find_variant_fields(Parser *p, size_t j) {
    for (;;) {
        size_t m = name_words(p, j);
        if (m == 0) return;
        add_field_name(p, j, m);
        j += m;
        if (j < p->count && p->tokens[j].kind == TOK_LPAREN) {  // (KIND)
            while (j < p->count && p->tokens[j].kind != TOK_RPAREN && p->tokens[j].kind != TOK_NEWLINE) j++;
            j++;
        }
        if (!(word_at(p, j, "and") || (j < p->count && p->tokens[j].kind == TOK_COMMA))) return;
        j++;
    }
}

// The variants indented below "NAME is one of:" (the colon at index `colon`),
// one per line. Returns the index after the block.
static size_t find_variants(Parser *p, size_t colon, size_t sum, bool added) {
    size_t i = colon + 1;
    if (i + 1 >= p->count || p->tokens[i].kind != TOK_NEWLINE || p->tokens[i + 1].kind != TOK_INDENT) return i;
    i += 2;
    for (size_t depth = 1; depth > 0 && i < p->count; i++) {
        TokenKind kind = p->tokens[i].kind;
        if (kind == TOK_INDENT) depth++;
        if (kind == TOK_DEDENT) depth--;
        if (depth == 1 && added && kind == TOK_WORD && at_line_start(p, i)) find_variant(p, i, sum);
    }
    return i;
}

// NAME is anything with: (NAME is every word before "is anything with", so
// it can be "something that can speak"); its token count, or 0.
size_t parser_interface_words(const Parser *p, size_t i) {
    if (!at_line_start(p, i)) return 0;
    size_t n = 0;
    while (i + n < p->count && p->tokens[i + n].kind == TOK_WORD) {
        size_t j = i + n;
        if (word_at(p, j, "is") && word_at(p, j + 1, "anything") && word_at(p, j + 2, "with") && j + 3 < p->count &&
            p->tokens[j + 3].kind == TOK_COLON) {
            return n;
        }
        n++;
    }
    return 0;
}

// "something that can speak" -> "things that can speak"; others like a record kind's.
static const char *interface_plural(Arena *arena, const char *name) {
    if (strncmp(name, "something ", 10) == 0) return arena_sprintf(arena, "things %s", name + 10);
    if (strncmp(name, "anything ", 9) == 0) return arena_sprintf(arena, "things %s", name + 9);
    return parser_plural_of(arena, name);
}

// One way's line: "way to NAME [with ...]" (the "a" is dropped): its name is
// a method's name, so calls of it parse like method calls.
static void find_way(Parser *p, size_t i) {
    if (!word_at(p, i, "way") || !word_at(p, i + 1, "to")) return;
    size_t j = i + 2, n = 0;
    while (j + n < p->count && is_inner_word(&p->tokens[j + n]) && !word_among(&p->tokens[j + n], parser_input_intros) &&
           !word_at(p, j + n, "giving")) {
        n++;
    }
    if (n == 0 && j < p->count && parser_is_name_word(&p->tokens[j])) n = 1;  // a one-word name that's a stop word
    if (n == 0) return;
    const Token *after = j + n < p->count ? &p->tokens[j + n] : NULL;
    unsigned intro = after && word_among(after, parser_input_intros) ? parser_intro_bit(after->text) : 0;
    add_known(p, join_words(p, j, n, false, ' '), n, true, intro, true);
}

// Adds the interface named by tokens [i, i + n) and finds its ways; returns
// the index after its block.
static size_t find_interface(Parser *p, size_t i, size_t n) {
    const char *name = join_words(p, i, n, false, ' ');
    bool known = false;
    for (size_t k = 0; k < p->records->interfaces.len; k++) known |= strcmp(p->records->interfaces.items[k].name, name) == 0;
    if (!known) {
        RecordKind kind = {name, join_words(p, i, n, true, ' '), interface_plural(p->arena, name), NULL};
        vec_push(p->arena, &p->records->interfaces, kind);
    }
    size_t j = i + n + 4;  // is anything with :
    if (j + 1 >= p->count || p->tokens[j].kind != TOK_NEWLINE || p->tokens[j + 1].kind != TOK_INDENT) return j;
    j += 2;
    for (size_t depth = 1; depth > 0 && j < p->count; j++) {
        TokenKind kind = p->tokens[j].kind;
        if (kind == TOK_INDENT) depth++;
        if (kind == TOK_DEDENT) depth--;
        if (depth == 1 && kind == TOK_WORD && at_line_start(p, j)) find_way(p, j);
    }
    return j;
}

// Every record kind and kind with variants, in order, every variant, and
// every field name: "NAME, which is" in a record, "with NAME (KIND)" in a
// variant; and every interface with the names of its ways.
static void find_records(Parser *p) {
    for (size_t i = 0; i < p->count; i++) {
        size_t interface = parser_interface_words(p, i);
        if (interface > 0) {
            i = find_interface(p, i, interface) - 1;
            continue;
        }
        size_t n = record_definition_words(p, i);
        if (n > 0) {
            add_kind(p, i, n, false);
            continue;
        }
        n = parser_has_definition_words(p, i);
        if (n > 0) {
            add_kind(p, i, n, false);
            i = find_has_fields(p, i + n + 1) - 1;
            continue;
        }
        n = parser_made_of_words(p, i);
        if (n > 0) {  // a point is made of an x (a number) and a y (a number).
            if (add_kind(p, i, n, true)) {
                size_t sum = p->records->sums.len - 1;
                VariantKind variant = {p->records->sums.items[sum].name, p->records->sums.items[sum].display, sum, 0};
                vec_push(p->arena, &p->records->variants, variant);
                find_variant_fields(p, i + n + 3);
            }
            continue;
        }
        n = sum_definition_words(p, i);
        if (n > 0) {
            bool added = add_kind(p, i, n, true);
            i = find_variants(p, i + n + 3, p->records->sums.len - 1, added) - 1;
            continue;
        }
        n = at_line_start(p, i) ? name_words(p, i) : 0;
        if (n > 0 && i + n + 2 < p->count && p->tokens[i + n].kind == TOK_COMMA && word_at(p, i + n + 1, "which") &&
            word_at(p, i + n + 2, "is")) {
            add_field_name(p, i, n);
        }
    }
}

bool parser_add_builtin_kind(Parser *p, const char *name, size_t words) {
    for (size_t r = 0; r < p->records->len; r++) {
        if (strcmp(p->records->items[r].name, name) == 0) return false;
    }
    RecordKind kind = {name, name, parser_plural_of(p->arena, name), NULL};
    vec_push(p->arena, p->records, kind);
    add_known(p, name, words, false, false, false);
    return true;
}

void parser_add_field_name(Parser *p, const char *name, size_t words) {
    for (size_t k = 0; k < p->fields.len; k++) {
        if (strcmp(p->fields.items[k].text, name) == 0) return;
    }
    KnownName field = {name, words, false, false, false};
    vec_push(p->arena, &p->fields, field);
}

bool parser_is_method(const Parser *p, const char *name) {
    for (size_t i = 0; i < p->names.len; i++) {
        if (strcmp(p->names.items[i].text, name) == 0) return p->names.items[i].method;
    }
    return false;
}

void parser_find_names(Parser *p) {
    find_records(p);
    parser_find_program_result(p);
    for (size_t i = 0; i + 1 < p->count; i++) {
        if (p->tokens[i].kind != TOK_WORD) continue;
        const char *word = p->tokens[i].text;
        size_t n = name_words(p, i + 1), after = i + 1 + n;
        size_t kind_words;
        if (strcmp(word, "to") == 0 && (at_line_start(p, i) || parser_after_privately(p, i))) {
            find_function(p, i, false);
        } else if (at_line_start(p, i) && parser_match_kind_name(p, i, false, &kind_words) &&
                   word_at(p, i + kind_words, "can")) {  // a dog can bark:
            find_function(p, i + kind_words, true);
        } else if (strcmp(word, "let") == 0 && (word_at(p, after, "be") || word_at(p, after, "equal"))) {
            add_name(p, i + 1, n, false);
        } else if (strcmp(word, "keep") == 0 && (word_at(p, after, "as") || word_at(p, after, "at"))) {
            add_name(p, i + 1, n, false);
        } else if (strcmp(word, "as") == 0 && after < p->count && p->tokens[after].kind == TOK_COLON) {
            add_name(p, i + 1, n, false);  // count ... as NAME:
        } else if (strcmp(word, "each") == 0 && i > 0 && word_at(p, i - 1, "for") && word_at(p, after, "and") &&
                   word_at(p, after + 1 + name_words(p, after + 1), "in")) {  // for each NAME and NAME in
            add_name(p, i + 1, n, false);
            add_name(p, after + 1, name_words(p, after + 1), false);
        } else if ((strcmp(word, "each") == 0 && i > 0 && word_at(p, i - 1, "for") &&
                    (word_at(p, after, "in") || word_at(p, after, "from") ||
                     (after < p->count && p->tokens[after].kind == TOK_COLON))) ||
                   (strcmp(word, "every") == 0 && word_at(p, after, "in") &&
                    !(n == 2 && word_at(p, i + 2, "item")))) {  // not "every other item in" (a slice)
            add_name(p, i + 1, n, false);
        } else if (strcmp(word, "call") == 0) {  // and call the answer / call it / call each number NAME
            size_t skip = word_at(p, i + 1, "answer") || word_at(p, i + 1, "it")          ? 1
                          : word_at(p, i + 1, "each") && word_at(p, i + 2, "number") ? 2
                                                                                      : 0;
            size_t m = skip ? name_words(p, i + 1 + skip) : 0;
            if (m && (at_sentence_end_index(p, i + 1 + skip + m) || p->tokens[i + 1 + skip + m].kind == TOK_COLON)) {
                add_name(p, i + 1 + skip, m, false);
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
    name->pos = ast_pos_join(first, (SourcePos){p->prev_span, 0, 0, 0});
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
        diag_note(p->diag, "Try a name that says what it holds, like \"total\" or \"answer\".");
    }
}

static void report_name_missing_after_prev(Parser *p) {
    if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Add a name there, like \"total\".");
    }
}

// The name's n words are followed by a word that means something else and
// then more name words ("let price plus tax be", "rate of pay"): the name
// can't contain it. Suggests the name's other words: "pay rate" for "rate of
// pay", as English puts it, and "price tax" for "price plus tax".
static bool reject_reserved_inside(Parser *p, size_t n, const char *const *next) {
    const Token *word = parser_peek(p, n);
    if (word->kind != TOK_WORD || word_among(word, next) || is_inner_word(word) || !is_inner_word(parser_peek(p, n + 1))) {
        return false;
    }
    Parser ahead = *p;  // "add 5 to total and say total": the name ends before the next action
    ahead.pos += n;
    if (parser_at_next_action(&ahead)) return false;
    size_t total = n + 1;
    while (is_inner_word(parser_peek(p, total)) && !word_among(parser_peek(p, total), next)) total++;
    if (parser_error(p, word->span, "\"%s\" can't be part of a name, because it already means something in EasyScript.",
                     word->text)) {
        const char *before = join_words(p, p->pos, n, true, ' ');
        const char *after = join_words(p, p->pos + n + 1, total - n - 1, true, ' ');
        bool of = strcmp(word->text, "of") == 0;
        diag_note(p->diag, "Try a name without it, like \"%s %s\".", of ? after : before, of ? before : after);
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
        diag_note(p->diag, "A name is a single word, like \"total\", or several, like \"late fee\".");
    }
    return false;
}
