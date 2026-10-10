#include <stdarg.h>
#include <string.h>
#include "front/parse_internal.h"

// --- Tokens ----------------------------------------------------------------

const Token *parser_peek(const Parser *p, size_t ahead) {
    size_t index = p->pos + ahead;
    return &p->tokens[index < p->count ? index : p->count - 1];
}

bool parser_at(const Parser *p, size_t ahead, TokenKind kind) {
    return parser_peek(p, ahead)->kind == kind;
}

bool parser_at_word(const Parser *p, size_t ahead, const char *word) {
    const Token *token = parser_peek(p, ahead);
    return token->kind == TOK_WORD && strcmp(token->text, word) == 0;
}

const Token *parser_advance(Parser *p) {
    const Token *token = parser_peek(p, 0);
    if (p->pos < p->count - 1) p->pos++;
    p->prev_span = token->span;
    return token;
}

Span parser_consume(Parser *p, size_t n) {
    Span first = parser_peek(p, 0)->span;
    Span last = first;
    for (size_t i = 0; i < n; i++) {
        last = parser_advance(p)->span;
    }
    Span span = {first.offset, last.offset + last.length - first.offset};
    p->prev_span = span;
    return span;
}

SourcePos parser_token_pos(const Token *token) {
    SourcePos pos = {token->span, token->line, token->column};
    return pos;
}

bool parser_at_sentence_end(const Parser *p) {
    TokenKind kind = parser_peek(p, 0)->kind;
    return kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT ||
           kind == TOK_INDENT;
}

static const char *const operator_words[] = {
    "and", "or", "not", "is", "isn't", "equals", "reaches", "plus", "minus",
    "times", "multiplied", "divided", "mod", "followed", "as", "using",
};

bool parser_is_operator_word(const char *word) {
    for (size_t i = 0; i < sizeof(operator_words) / sizeof(operator_words[0]); i++) {
        if (strcmp(word, operator_words[i]) == 0) return true;
    }
    return false;
}

bool parser_is_value_word(const char *word) {
    static const char *const words[] = {"yes", "no", "true", "false", "nothing", "it"};
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); i++) {
        if (strcmp(word, words[i]) == 0) return true;
    }
    return false;
}

bool parser_starts_operand(const Token *token) {
    switch (token->kind) {
    case TOK_NUMBER:
    case TOK_STRING:
    case TOK_LPAREN:
    case TOK_MINUS: return true;
    case TOK_WORD: return !parser_is_operator_word(token->text);
    default: return false;
    }
}

// --- Phrases ---------------------------------------------------------------

// Words and symbols can be part of a phrase; numbers and text can't.
static bool is_phrase_token(const Token *token) {
    return token->kind == TOK_WORD || token->kind >= TOK_PERIOD;
}

// How many of the phrase's leading words the next tokens match, and (in
// *total) how many words the phrase has.
static size_t leading_words(const Parser *p, const char *words, size_t *total) {
    size_t matched = 0;
    bool matching = true;
    *total = 0;
    while (*words) {
        const char *space = strchr(words, ' ');
        size_t n = space ? (size_t)(space - words) : strlen(words);
        const Token *token = parser_peek(p, matched);
        if (matching && is_phrase_token(token) && token->text_len == n && memcmp(token->text, words, n) == 0) {
            matched++;
        } else {
            matching = false;
        }
        (*total)++;
        words += n;
        if (*words == ' ') words++;
    }
    return matched;
}

size_t parser_leading_words(const Parser *p, const char *words) {
    size_t total;
    return leading_words(p, words, &total);
}

size_t parser_match_words(const Parser *p, const char *words) {
    size_t total;
    size_t matched = leading_words(p, words, &total);
    return matched == total ? total : 0;
}

// "a", "a" or "b", "a", "b" or "c"
static void append_choices(StrBuf *out, const Phrase *table, size_t n, const bool *chosen) {
    size_t count = 0;
    size_t total = 0;
    for (size_t i = 0; i < n; i++) total += chosen[i];
    for (size_t i = 0; i < n; i++) {
        if (!chosen[i]) continue;
        count++;
        if (count > 1) sb_append(out, count == total ? " or " : ", ");
        sb_appendf(out, "\"%s\"", table[i].display);
    }
}

static void report_missing_word(Parser *p, const Phrase *table, size_t n, size_t prefix) {
    bool *chosen = arena_alloc(p->arena, n * sizeof(bool));
    for (size_t i = 0; i < n; i++) {
        size_t total;
        chosen[i] = leading_words(p, table[i].words, &total) == prefix && total == prefix + 1;
    }
    Span span = parser_consume(p, prefix);
    if (parser_error(p, span, "Something is missing after \"%s\".", parser_text(p, span))) {
        StrBuf choices;
        sb_init(&choices, p->arena);
        append_choices(&choices, table, n, chosen);
        diag_note(p->diag, "Did you mean %s?", choices.data);
    }
}

const Phrase *parser_match_phrase(Parser *p, const Phrase *table, size_t n, size_t *len) {
    const Phrase *best = NULL;
    size_t best_len = 0;
    size_t near_len = 0;
    for (size_t i = 0; i < n; i++) {
        size_t total;
        size_t matched = leading_words(p, table[i].words, &total);
        bool operand_ok = !table[i].needs_operand || parser_starts_operand(parser_peek(p, total));
        if (matched == total && total > best_len && operand_ok) {
            best = &table[i];
            best_len = total;
        } else if (matched + 1 == total && matched > near_len) {
            near_len = matched;
        }
    }
    if (near_len > best_len) {
        report_missing_word(p, table, n, near_len);
        return NULL;
    }
    *len = best_len;
    return best;
}

// --- Errors ----------------------------------------------------------------

bool parser_error(Parser *p, Span span, const char *fmt, ...) {
    if (p->sentence_failed) return false;
    p->sentence_failed = true;
    va_list args;
    va_start(args, fmt);
    const char *message = arena_vsprintf(p->arena, fmt, args);
    va_end(args);
    diag_error(p->diag, span, "%s", message);
    return true;
}

Expr *parser_error_expr(Parser *p) {
    return ast_new_expr(p->arena, EXPR_ERROR, parser_token_pos(parser_peek(p, 0)));
}

const char *parser_text(const Parser *p, Span span) {
    char *text = arena_alloc(p->arena, span.length + 1);
    memcpy(text, p->source + span.offset, span.length);
    return text;
}

const char *parser_quoted(const Parser *p, Span span) {
    const char *text = parser_text(p, span);
    if (text[0] == '"') return arena_sprintf(p->arena, "the text %s", text);
    return arena_sprintf(p->arena, "\"%s\"", text);
}

const char *parser_describe(const Parser *p, const Token *token) {
    switch (token->kind) {
    case TOK_EOF: return "the end of the file";
    case TOK_NEWLINE: return "the end of the line";
    case TOK_INDENT: return "an indented line";
    case TOK_DEDENT: return "the end of the block";
    case TOK_STRING: {
        StrBuf sb;
        sb_init(&sb, p->arena);
        sb_append(&sb, "the text ");
        sb_append_quoted(&sb, token->text, token->text_len);
        return sb.data;
    }
    default: return arena_sprintf(p->arena, "\"%s\"", token->text);
    }
}

const char *parser_closest_words(const Parser *p, const char *word, const char *const *candidates, size_t n) {
    size_t limit = strlen(word) <= 3 ? 1 : 2;
    size_t best = limit + 1;
    for (size_t i = 0; i < n; i++) {
        size_t d = edit_distance(p->arena, word, candidates[i]);
        if (d < best) best = d;
    }
    if (best > limit) return NULL;
    StrBuf sb;
    sb_init(&sb, p->arena);
    for (size_t i = 0; i < n; i++) {
        if (edit_distance(p->arena, word, candidates[i]) != best) continue;
        if (sb.len > 0) sb_append(&sb, " or ");
        sb_appendf(&sb, "\"%s\"", candidates[i]);
    }
    return sb.data;
}

bool parser_reject_bare_decimal(Parser *p) {
    const Token *period = parser_peek(p, 0);
    const Token *number = parser_peek(p, 1);
    if (period->kind != TOK_PERIOD || number->kind != TOK_NUMBER ||
        number->span.offset != period->span.offset + 1) {
        return false;
    }
    Span span = parser_consume(p, 2);
    if (parser_error(p, span, "A number needs a digit before its decimal point.")) {
        diag_note(p->diag, "Write 0.%s instead of .%s.", number->text, number->text);
    }
    return true;
}

void parser_report_leftover(Parser *p) {
    const Token *token = parser_peek(p, 0);
    if (token->kind == TOK_RPAREN) {
        if (parser_error(p, token->span, "There's a \")\" here without a matching \"(\".")) {
            diag_note(p->diag, "Remove it, or add a \"(\" where the group starts.");
        }
        return;
    }
    if (parser_at_word(p, 0, "and") && parser_at_word(p, 1, "call")) {
        Span span = {token->span.offset, parser_peek(p, 1)->span.offset + parser_peek(p, 1)->span.length - token->span.offset};
        if (parser_error(p, span, "I don't understand \"and call\" here.")) {
            diag_note(p->diag, "\"and call\" only follows the question in \"ask\" or the file in \"read file\".");
        }
        return;
    }
    if (token->kind == TOK_WORD && parser_is_operator_word(token->text)) {
        // An operator with no value after it, e.g. "3 times" at the end of a line.
        if (parser_error(p, token->span, "Something is missing after \"%s\".", token->text)) {
            diag_note(p->diag, "Add a value there, like a number, some text in quotes, or a name.");
        }
        return;
    }
    if (!parser_error(p, token->span, "I don't understand %s here.", parser_describe(p, token))) return;
    size_t n = sizeof(operator_words) / sizeof(operator_words[0]);
    const char *closest = token->kind == TOK_WORD ? parser_closest_words(p, token->text, operator_words, n) : NULL;
    if (closest) {
        diag_note(p->diag, "Did you mean %s?", closest);
    } else {
        diag_note(p->diag, "After a value I expected an operator like \"plus\" or \"is\", or the end of the sentence.");
    }
}
