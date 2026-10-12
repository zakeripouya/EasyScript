// Definitions with inputs in the middle of their first line, and their calls:
//
//   to give a customer a discount of some percent:      give ann a discount of 10 percent.
//       reduce the customer's price by the percent percent.
//   to introduce a person to another person:            introduce ann to bob.
//                                                       give every loyal customer a discount of 10 percent.
//
// In the first line, "a KIND" (a kind the program has, with its "a" or "an"),
// "another KIND" and "some NAME" are inputs, called "the KIND", "the other
// KIND" and "the NAME" inside. Every other word is fixed, and a call says
// exactly those words, with a value where each input goes. Such a line is a
// pattern only when an input comes before any word that introduces inputs the
// usual way ("with", "of", "to", ...), and when it has no comma ("to bark,
// for a dog:" is a method). The pre-scan finds them first, so a sentence can
// use one defined further down. A call is found by its words: each fixed word
// is the first one after the input before it, at the sentence's top level,
// so finding it never depends on what the values mean.

#include <string.h>
#include "front/parse_internal.h"

static bool word_at(const Parser *p, size_t i, const char *word) {
    return i < p->count && p->tokens[i].kind == TOK_WORD && strcmp(p->tokens[i].text, word) == 0;
}

static bool sentence_end_at(const Parser *p, size_t i) {
    if (i >= p->count) return true;
    TokenKind kind = p->tokens[i].kind;
    return kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT || kind == TOK_INDENT;
}

static bool after_a(const Parser *p, size_t i) {  // the token had "a" or "an" before it
    Span filler = p->tokens[i].filler;
    if (filler.length != 1 && filler.length != 2) return false;
    const char *text = p->source + filler.offset;
    return (text[0] == 'a' || text[0] == 'A') && (filler.length == 1 || text[1] == 'n' || text[1] == 'N');
}

static const RecordKind *kind_of(const Parser *p, Type type) {
    if (type_is_textkind(type)) return &p->records->texts.items[type_textkind_index(type)];
    if (type_is_iface(type)) return &p->records->interfaces.items[type_iface_index(type)];
    if (type_is_sum(type)) return &p->records->sums.items[type_sum_index(type)];
    return &p->records->items[type_record_index(type)];
}

static Type kind_at(const Parser *p, size_t i, size_t *words) {
    Parser at = *p;
    at.pos = i;
    at.limit = 0;
    return parser_match_kind_name(&at, 0, false, words);
}

// An input of a pattern's first line at token i: its part, or false.
static bool input_at(const Parser *p, size_t i, PatternPart *part) {
    size_t words;
    Type kind;
    memset(part, 0, sizeof *part);
    part->token = i;
    if (word_at(p, i, "another") && (kind = kind_at(p, i + 1, &words)) != TYPE_UNKNOWN) {
        const RecordKind *record = kind_of(p, kind);
        part->name = arena_sprintf(p->arena, "other %s", record->name);
        part->shown = arena_sprintf(p->arena, "another %s", record->display);
        part->tokens = 1 + words;
        return true;
    }
    if (word_at(p, i, "some") && i + 1 < p->count && parser_is_name_word(&p->tokens[i + 1])) {
        part->name = p->tokens[i + 1].text;
        part->shown = arena_sprintf(p->arena, "some %s", p->tokens[i + 1].text);
        part->some = true;
        part->tokens = 2;
        if (i + 2 < p->count && p->tokens[i + 2].kind == TOK_LPAREN) {  // some percent (a number)
            size_t j = i + 2;
            while (j < p->count && p->tokens[j].kind != TOK_RPAREN && !sentence_end_at(p, j)) j++;
            part->tokens = j + 1 - i;
        }
        return true;
    }
    if (p->tokens[i].kind == TOK_WORD && after_a(p, i) && (kind = kind_at(p, i, &words)) != TYPE_UNKNOWN) {
        const RecordKind *record = kind_of(p, kind);
        part->name = record->name;
        part->shown = arena_sprintf(p->arena, "%s %s", strchr("aeiou", record->display[0]) ? "an" : "a",
                                    record->display);
        part->tokens = words;
        return true;
    }
    return false;
}

// "to ..." at token i: the line as a pattern, or false.
static bool pattern_line(Parser *p, size_t i, Pattern *pattern) {
    memset(pattern, 0, sizeof *pattern);
    pattern->at = i;
    pattern->line = p->tokens[i].line;
    bool inputs = false, introduced = false;
    for (size_t j = i + 1; j < p->count && p->tokens[j].kind != TOK_COLON;) {
        if (sentence_end_at(p, j) || p->tokens[j].kind == TOK_COMMA) return false;
        PatternPart part;
        if (input_at(p, j, &part)) {
            if (j == i + 1 || introduced) return false;  // starts with an input, or "with a dog": the usual inputs
            inputs = true;
            if (!pattern->parts.items[pattern->parts.len - 1].word) pattern->broken = true;
            vec_push(p->arena, &pattern->parts, part);
            j += part.tokens;
            continue;
        }
        if (p->tokens[j].kind != TOK_WORD) return false;
        size_t words;
        if (word_at(p, j, "for") && kind_at(p, j + 1, &words) != TYPE_UNKNOWN) return false;  // a method
        for (size_t k = 0; parser_input_intros[k]; k++) {
            if (strcmp(p->tokens[j].text, parser_input_intros[k]) == 0 && !inputs) introduced = true;
        }
        memset(&part, 0, sizeof part);
        part.word = p->tokens[j].text;
        part.token = j;
        part.tokens = 1;
        vec_push(p->arena, &pattern->parts, part);
        j++;
    }
    if (!inputs) return false;
    StrBuf text;
    sb_init(&text, p->arena);
    for (size_t k = 0; k < pattern->parts.len; k++) {
        const PatternPart *part = &pattern->parts.items[k];
        if (k) sb_append(&text, " ");
        Span filler = part->word ? p->tokens[part->token].filler : (Span){0, 0};
        for (size_t c = 0; c < filler.length; c++) {  // "a discount": its "a" as well
            char ch = p->source[filler.offset + c];
            sb_appendf(&text, "%c", ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
        }
        if (filler.length) sb_append(&text, " ");
        sb_append(&text, part->word ? part->word : part->shown);
    }
    pattern->text = text.data;
    return true;
}

void parser_find_patterns(Parser *p) {
    for (size_t i = 0; i + 1 < p->count; i++) {
        if (!word_at(p, i, "to")) continue;
        bool line_start = i == 0 || p->tokens[i - 1].kind == TOK_NEWLINE || p->tokens[i - 1].kind == TOK_INDENT ||
                          p->tokens[i - 1].kind == TOK_DEDENT;
        Pattern pattern;
        if (!(line_start || parser_after_privately(p, i)) || !pattern_line(p, i, &pattern)) continue;
        vec_push(p->arena, &p->patterns, pattern);
        for (size_t k = 0; k < pattern.parts.len; k++) {  // "other person" is a name of two words
            const PatternPart *part = &pattern.parts.items[k];
            if (part->name && strchr(part->name, ' ')) {
                size_t words = 1;
                for (const char *c = part->name; *c; c++) words += *c == ' ';
                parser_add_name_words(p, part->name, words);
            }
        }
    }
}

const Pattern *parser_pattern_at(const Parser *p, size_t to) {
    for (size_t i = 0; i < p->patterns.len; i++) {
        if (p->patterns.items[i].at == to) return &p->patterns.items[i];
    }
    return NULL;
}

// --- The definition ------------------------------------------------------------------

// A fixed word that's a kind ("to feed customer ...") was probably meant as an input.
static bool reject_kind_word(Parser *p, const PatternPart *part) {
    size_t words;
    Type kind = kind_at(p, part->token, &words);
    if (kind == TYPE_UNKNOWN) return false;
    const RecordKind *record = kind_of(p, kind);
    if (parser_error(p, p->tokens[part->token].span, "\"%s\" is a kind, so I can't tell whether it's an input here.",
                     part->word)) {
        diag_note(p->diag, "To make it an input, write \"a %s\". To keep the word, use one that isn't a kind.",
                  record->display);
        diag_info(p->diag, DIAG_TWO_MEANINGS, NULL, "an input or a word");
        diag_choice(p->diag, "a %s", record->display);
        diag_choice(p->diag, "another word");
    }
    return true;
}

static bool check_parts(Parser *p, const Pattern *pattern) {
    for (size_t k = 0; k < pattern->parts.len; k++) {
        const PatternPart *part = &pattern->parts.items[k];
        if (part->word) {
            if (reject_kind_word(p, part)) return false;
            continue;
        }
        if (k > 0 && !pattern->parts.items[k - 1].word) {
            if (parser_error(p, p->tokens[part->token].span, "Two inputs in a row can't be told apart in a call.")) {
                diag_note(p->diag, "Put a word between them, like \"to introduce a person to another person:\".");
                diag_info(p->diag, DIAG_NOT_ALLOWED, NULL, NULL);
            }
            return false;
        }
        for (size_t m = 0; m < k; m++) {
            const PatternPart *before = &pattern->parts.items[m];
            if (before->name && strcmp(before->name, part->name) == 0) {
                if (parser_error(p, p->tokens[part->token].span, "There are two inputs called \"%s\" here.",
                                 part->name)) {
                    diag_note(p->diag, "Write \"another %s\" for the second one, and call it \"the other %s\".",
                              part->name, part->name);
                    diag_info(p->diag, DIAG_NOT_ALLOWED, NULL, NULL);
                }
                return false;
            }
        }
    }
    return true;
}

static bool add_input(Parser *p, Stmt *stmt, const PatternPart *part) {
    Name name = {part->name, part->name, parser_token_pos(&p->tokens[part->token])};
    Type declared = TYPE_UNKNOWN;
    if (part->some) {
        if (part->tokens > 2) {  // "some percent (a number)"
            p->pos = part->token + 2;
            if (!parser_parse_param_kind(p, &declared)) return false;
        }
    } else {
        size_t words;
        declared = kind_at(p, part->token + (word_at(p, part->token, "another") ? 1 : 0), &words);
    }
    vec_push(p->arena, &stmt->as.function.params, name);
    vec_push(p->arena, &stmt->as.function.joiners, (const char *)NULL);
    vec_push(p->arena, &stmt->as.function.declared, declared);
    return true;
}

Stmt *parser_parse_pattern_definition(Parser *p, const Token *verb, const Pattern *pattern) {
    Stmt *stmt = parser_new_stmt(p, STMT_FUNCTION, verb);
    stmt->as.function.pattern = true;
    const Token *first = &p->tokens[pattern->parts.items[0].token];
    const PatternPart *last = &pattern->parts.items[pattern->parts.len - 1];
    const Token *end = &p->tokens[last->token + last->tokens - 1];
    stmt->as.function.name.text = pattern->text;
    stmt->as.function.name.display = pattern->text;
    stmt->as.function.name.pos = ast_pos_join(parser_token_pos(first), parser_token_pos(end));
    bool ok = check_parts(p, pattern);
    for (size_t k = 0; ok && k < pattern->parts.len; k++) {
        if (!pattern->parts.items[k].word) ok = add_input(p, stmt, &pattern->parts.items[k]);
    }
    if (ok) {
        p->pos = last->token + last->tokens;  // the colon
        SourcePos header = stmt->pos;
        ok = parser_parse_block(p, &stmt->as.function.body, &header, NULL, "this definition");
    } else {
        parser_skip_line_and_block(p);
    }
    p->ended_with_block = true;  // set last: statements inside the block reset it
    return ok ? stmt : NULL;
}

// --- Calls ---------------------------------------------------------------------------

typedef struct {
    const Pattern *pattern;
    Vec(size_t) from, to;  // per input: its tokens
    size_t end;            // the sentence's end
} Match;

// Where the sentence starting here ends: its end, ", then ..." or "and then ...".
static size_t sentence_end(const Parser *p) {
    int depth = 0;
    for (size_t i = p->pos;; i++) {
        if (sentence_end_at(p, i)) return i;
        TokenKind kind = p->tokens[i].kind;
        if (kind == TOK_LPAREN) depth++;
        if (kind == TOK_RPAREN && depth > 0) depth--;
        if (depth > 0) continue;
        if (kind == TOK_COMMA && (word_at(p, i + 1, "then") || word_at(p, i + 1, "after") ||
                                  word_at(p, i + 1, "next") || word_at(p, i + 1, "finally") ||
                                  (word_at(p, i + 1, "and") && word_at(p, i + 2, "then")))) {
            return i;
        }
        if (word_at(p, i, "and")) {
            Parser at = *p;
            at.pos = i;
            if (parser_at_next_action(&at)) return i;
        }
    }
}

// The first `word` from `from` to `end` at the top level, or end.
static size_t find_word(const Parser *p, size_t from, size_t end, const char *word) {
    int depth = 0;
    for (size_t i = from; i < end; i++) {
        if (p->tokens[i].kind == TOK_LPAREN) depth++;
        if (p->tokens[i].kind == TOK_RPAREN && depth > 0) depth--;
        if (depth == 0 && word_at(p, i, word)) return i;
    }
    return end;
}

static bool match_pattern(const Parser *p, const Pattern *pattern, size_t end, Match *match) {
    memset(match, 0, sizeof *match);
    match->pattern = pattern;
    size_t at = p->pos;
    for (size_t k = 0; k < pattern->parts.len; k++) {
        const PatternPart *part = &pattern->parts.items[k];
        if (part->word) {
            if (at >= end || !word_at(p, at, part->word)) return false;
            at++;
            continue;
        }
        const PatternPart *next = k + 1 < pattern->parts.len ? &pattern->parts.items[k + 1] : NULL;
        size_t stop = next ? find_word(p, at + 1, end, next->word) : end;
        if (stop <= at || (next && stop == end)) return false;
        size_t value_end = stop;
        if (part->some && value_end - at > 1 && word_at(p, value_end - 1, part->name)) value_end--;  // "10 percent"
        vec_push(p->arena, &match->from, at);
        vec_push(p->arena, &match->to, value_end);
        at = stop;
    }
    match->end = end;
    return at == end;
}

static size_t find_matches(const Parser *p, Match **matches) {
    const Token *first = parser_peek(p, 0);
    if (first->kind != TOK_WORD || p->limit) return 0;
    size_t end = sentence_end(p), count = 0;
    for (size_t i = 0; i < p->patterns.len; i++) {
        const Pattern *pattern = &p->patterns.items[i];
        if (pattern->broken || strcmp(pattern->parts.items[0].word, first->text) != 0) continue;
        Match match;
        if (!match_pattern(p, pattern, end, &match)) continue;
        if (matches) {
            Match *grown = arena_alloc(p->arena, sizeof(Match) * (count + 1));
            if (count) memcpy(grown, *matches, sizeof(Match) * count);
            grown[count] = match;
            *matches = grown;
        }
        count++;
    }
    return count;
}

// A pattern this sentence seems meant for but doesn't fit: one with its first
// word whose second fixed word is in the sentence (or that has only one).
static const Pattern *near_miss(const Parser *p) {
    const Token *first = parser_peek(p, 0);
    if (first->kind != TOK_WORD || p->limit) return NULL;
    size_t end = sentence_end(p);
    for (size_t i = 0; i < p->patterns.len; i++) {
        const Pattern *pattern = &p->patterns.items[i];
        if (pattern->broken || strcmp(pattern->parts.items[0].word, first->text) != 0) continue;
        const char *second = NULL;
        for (size_t k = 1; k < pattern->parts.len && !second; k++) second = pattern->parts.items[k].word;
        if (!second || find_word(p, p->pos + 1, end, second) < end) return pattern;
    }
    return NULL;
}

bool parser_at_pattern_call(const Parser *p) {
    return p->patterns.len > 0 && (find_matches(p, NULL) > 0 || near_miss(p));
}

// Where the sentence stops fitting the pattern, as an error.
static Stmt *report_near_miss(Parser *p, const Pattern *pattern) {
    size_t end = sentence_end(p), at = p->pos;
    Span span = parser_peek(p, 0)->span;
    const char *message = NULL;
    for (size_t k = 0; k < pattern->parts.len && !message; k++) {
        const PatternPart *part = &pattern->parts.items[k];
        if (part->word) {
            if (at < end && word_at(p, at, part->word)) {
                span = p->tokens[at++].span;
                continue;
            }
            message = arena_sprintf(p->arena, "I expected \"%s\" here, but found %s.", part->word,
                                    at < end ? parser_quoted(p, p->tokens[at].span) : "the end of the sentence");
            if (at < end) span = p->tokens[at].span;
            continue;
        }
        const PatternPart *next = k + 1 < pattern->parts.len ? &pattern->parts.items[k + 1] : NULL;
        size_t stop = next ? find_word(p, at + 1, end, next->word) : end;
        if (at >= end || stop <= at || (next && word_at(p, at, next->word))) {
            message = arena_sprintf(p->arena, "Something is missing after %s.", parser_quoted(p, span));
            break;
        }
        at = stop;
    }
    if (!message) message = arena_sprintf(p->arena, "I expected the sentence to end after %s.", parser_quoted(p, span));
    if (parser_error(p, span, "%s", message)) {
        diag_note(p->diag, "This sentence looks like \"to %s:\" on line %zu. Put a value where each input goes.",
                  pattern->text, pattern->line);
        diag_info(p->diag, DIAG_MISSING, "a value for each input", NULL);
    }
    return NULL;
}

static Stmt *report_two_meanings(Parser *p, const Match *matches, size_t count, const KnownName *function) {
    Span span = parser_peek(p, 0)->span;
    if (parser_error(p, span, "This sentence could mean two things.")) {
        diag_info(p->diag, DIAG_TWO_MEANINGS, NULL, "a definition");
        for (size_t i = 0; i < count; i++) {
            diag_note(p->diag, "It fits \"to %s:\" on line %zu.", matches[i].pattern->text, matches[i].pattern->line);
            diag_choice(p->diag, "to %s", matches[i].pattern->text);
        }
        if (function) {
            diag_note(p->diag, "It also calls \"%s\".", function->text);
            diag_choice(p->diag, "%s", function->text);
        }
        diag_note(p->diag, "Change the words of one of them so each sentence fits only one.");
    }
    return NULL;
}

// "every loyal customer": the input is each one, and the sentence is done for each.
static Expr *parse_group_input(Parser *p, size_t to) {
    const Token *every = parser_advance(p);
    if (p->group_list) {
        if (parser_error(p, every->span, "Only one input can be \"every ...\".")) {
            diag_note(p->diag, "Give the others one at a time, or use \"for each\".");
            diag_info(p->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        }
        return parser_error_expr(p);
    }
    size_t words = 0;
    while (p->pos + words < to && parser_is_name_word(&p->tokens[p->pos + words])) words++;
    if (words == 0) {
        if (parser_error(p, every->span, "I expected the items after \"every\", like \"every customer\".")) {
            diag_info(p->diag, DIAG_MISSING, "which items", NULL);
        }
        return parser_error_expr(p);
    }
    Name item;
    parser_take_name(p, words, &item);
    p->group_list = parser_implied_list(p, &item);
    p->group_item = item;
    p->group_every = every->span;
    return ast_new_expr(p->arena, EXPR_IT, item.pos);
}

static Expr *parse_input(Parser *p, const Match *match, size_t k, const PatternPart *next) {
    size_t from = match->from.items[k], to = match->to.items[k];
    p->pos = from;
    p->limit = to;
    p->limit_token = p->tokens[to];
    p->limit_token.kind = TOK_NEWLINE;
    p->limit_token.filler = (Span){0, 0};
    Expr *value;
    if (word_at(p, from, "every")) {
        value = parse_group_input(p, to);
    } else {
        p->call_sentence = true;
        value = parse_join(p);
        p->call_sentence = false;
    }
    if (!p->sentence_failed && p->pos < to) {
        const Token *token = &p->tokens[p->pos];
        if (parser_error(p, token->span, "I expected %s here, but found %s.",
                         next ? arena_sprintf(p->arena, "\"%s\"", next->word) : "the sentence to end",
                         parser_describe(p, token))) {
            diag_note(p->diag, "This sentence fits \"to %s:\" on line %zu. Put each value where its input goes.",
                      match->pattern->text, match->pattern->line);
            diag_info(p->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        }
    }
    p->limit = 0;
    return value;
}

Stmt *parser_parse_pattern_call(Parser *p) {
    Match *matches = NULL;
    size_t count = find_matches(p, &matches);
    if (count == 0) return report_near_miss(p, near_miss(p));
    const KnownName *function = parser_match_name(p, 0, true);
    if (count > 1 || function) return report_two_meanings(p, matches, count, function);
    const Match *match = &matches[0];
    const Token *verb = parser_peek(p, 0);
    Stmt *stmt = parser_new_stmt(p, STMT_CALL, verb);
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, parser_token_pos(verb));
    call->as.call.name = match->pattern->text;
    call->display = match->pattern->text;
    size_t input = 0;
    for (size_t k = 0; k < match->pattern->parts.len; k++) {
        const PatternPart *part = &match->pattern->parts.items[k];
        if (part->word) continue;
        const PatternPart *next = k + 1 < match->pattern->parts.len ? &match->pattern->parts.items[k + 1] : NULL;
        Expr *value = parse_input(p, match, input, next);
        vec_push(p->arena, &call->as.call.args, value);
        input++;
    }
    size_t end = match->end;
    p->pos = end;
    p->prev_span = p->tokens[end - 1].span;
    call->pos = ast_pos_join(call->pos, parser_token_pos(&p->tokens[end - 1]));
    stmt->as.call = call;
    stmt->pos = call->pos;
    if (p->group_list) stmt = parser_finish_group(p, stmt);
    return stmt;
}
