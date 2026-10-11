// Kinds of values, as written in definitions, fields and empty lists and
// maps: "a number", "text", "yes or no", "a list of numbers", "a map from
// text to numbers", "a dog", "a dog or nothing", "a shape". And the
// definitions of kinds with variants:
//
//   a shape is one of:
//       a circle with a radius (a number).
//       a rectangle with a width (a number) and a height (a number).
//       a point.
//
// The pre-scan (parse_names.c) finds every kind and variant first, so they
// can be used anywhere, also before they're defined.

#include <string.h>
#include "front/parse_internal.h"

static const char *const sum_example = "a shape is one of:";
static const char *const variant_example = "a rectangle with a width (a number) and a height (a number).";

// --- Names of kinds and variants ------------------------------------------------------

// The kind with variants whose name (or plural) the tokens `ahead` spell, or -1.
static int match_sum(const Parser *p, size_t ahead, bool plural, size_t *words) {
    int sum = -1;
    *words = 0;
    for (size_t i = 0; i < p->records->sums.len; i++) {
        const RecordKind *kind = &p->records->sums.items[i];
        size_t n = parser_match_words_at(p, ahead, plural ? kind->plural : kind->name);
        if (n > *words) *words = n, sum = (int)i;
    }
    return sum;
}

Type parser_match_kind_name(const Parser *p, size_t ahead, bool plural, size_t *words) {
    size_t record_words, sum_words;
    int record = parser_match_record(p, ahead, plural, &record_words);
    int sum = match_sum(p, ahead, plural, &sum_words);
    if (sum >= 0 && sum_words > record_words) {
        *words = sum_words;
        return type_sum((size_t)sum);
    }
    *words = record >= 0 ? record_words : 0;
    return record >= 0 ? type_record((size_t)record) : TYPE_UNKNOWN;
}

int parser_match_variant(const Parser *p, size_t ahead, size_t *words) {
    int best = -1;
    *words = 0;
    for (size_t i = 0; i < p->records->variants.len; i++) {
        size_t n = parser_match_words_at(p, ahead, p->records->variants.items[i].name);
        if (n > *words) *words = n, best = (int)i;
    }
    return best;
}

// --- Kinds of values ------------------------------------------------------------------

static bool report_kind(Parser *p, const char *kinds, const char *missing) {
    const Token *word = parser_peek(p, 0);
    if (word->kind == TOK_COLON || word->kind == TOK_RPAREN || parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "%s", missing);
        }
    } else if (parser_error(p, word->span, "I don't know the kind of value %s.", parser_describe(p, word))) {
        diag_note(p->diag, "%s", kinds);
    }
    return false;
}

// What a list holds, in the plural: "numbers", "text", "yes/no values" (or
// "yes or no values"), "lists of" ..., a record kind's plural, or "maps from" ... .
bool parser_parse_items_kind(Parser *p, Type *type) {
    size_t words = 0;
    if (parser_at_word(p, 0, "numbers")) {
        *type = TYPE_NUMBER, words = 1;
    } else if (parser_at_word(p, 0, "text")) {
        *type = TYPE_TEXT, words = 1;
    } else if ((words = parser_match_words(p, "yes or no values")) > 0) {
        *type = TYPE_YESNO;
    } else if (parser_at_word(p, 0, "yes") && parser_at(p, 1, TOK_SLASH) && parser_at_word(p, 2, "no") &&
               parser_at_word(p, 3, "values")) {
        *type = TYPE_YESNO, words = 4;
    } else if (parser_match_words(p, "lists of")) {
        parser_consume(p, 2);
        if (!parser_parse_items_kind(p, type)) return false;
        *type = type_list_of(*type);
        return true;
    } else if (parser_at_map_kind(p, true)) {
        return parser_parse_map_kind(p, type);
    } else {
        Type kind = parser_match_kind_name(p, 0, true, &words);
        if (words > 0) *type = kind;
    }
    if (words > 0) {
        parser_consume(p, words);
        return true;
    }
    return report_kind(p, "Lists and maps hold \"numbers\", \"text\", \"yes/no values\", \"lists of\" something, or "
                          "\"maps from\" something, like \"a list of numbers\".",
                       "Say what the list holds, like \"a list of numbers\".");
}

// "or nothing" after a kind: only a record can be nothing.
static bool parse_or_nothing(Parser *p, Type *type) {
    size_t words = parser_match_words(p, "or nothing");
    if (words == 0) return true;
    Span span = parser_consume(p, words);
    if (type_is_record(*type) || type_is_sum(*type)) {
        *type += TYPE_MAYBE;
        return true;
    }
    if (parser_error(p, span, "Only a record can be \"or nothing\", not %s.", ast_type_name(p->arena, p->records, *type))) {
        diag_note(p->diag, "A number, text, yes/no value or list always has a value. To say there's none, use 0, "
                           "empty text (\"\"), no, or an empty list.");
    }
    return false;
}

bool parser_parse_kind(Parser *p, bool nothing_ok, Type *type) {
    size_t words = 0;
    if (parser_match_words(p, "list of")) {
        parser_consume(p, 2);
        if (!parser_parse_items_kind(p, type)) return false;
        *type = type_list_of(*type);
        return true;
    }
    if (parser_at_map_kind(p, false)) return parser_parse_map_kind(p, type) && parse_or_nothing(p, type);
    if (parser_at_word(p, 0, "number")) {
        *type = TYPE_NUMBER, words = 1;
    } else if (parser_at_word(p, 0, "text")) {
        *type = TYPE_TEXT, words = 1;
    } else if ((words = parser_match_words(p, "yes or no")) > 0) {
        *type = TYPE_YESNO;
    } else if (parser_at_word(p, 0, "yes") && parser_at(p, 1, TOK_SLASH) && parser_at_word(p, 2, "no")) {
        *type = TYPE_YESNO, words = 3;
    } else if (nothing_ok && parser_at_word(p, 0, "nothing")) {
        *type = TYPE_NOTHING, words = 1;
    } else {
        Type kind = parser_match_kind_name(p, 0, false, &words);
        if (words > 0) *type = kind;
    }
    if (words > 0) {
        parser_consume(p, words);
        return parse_or_nothing(p, type);
    }
    return report_kind(p,
                       nothing_ok ? "The kinds are \"a number\", \"text\", \"yes or no\", lists like \"a list of "
                                    "numbers\", and maps like \"a map from text to numbers\" (and \"nothing\" for a "
                                    "function that gives back no value)."
                                  : "The kinds are \"a number\", \"text\", \"yes or no\", lists like \"a list of "
                                    "numbers\", and maps like \"a map from text to numbers\".",
                       "Say which kind of value: \"a number\", \"text\", \"yes or no\", or a list like \"a list of "
                       "numbers\".");
}


// --- Kinds with variants ------------------------------------------------------------

bool parser_at_sum_definition(const Parser *p) {
    size_t n = 0, words;
    while (parser_is_name_word(parser_peek(p, n))) n++;
    return n > 0 && parser_match_words_at(p, n, "is one of") == 3 && parser_at(p, n + 3, TOK_COLON) &&
           match_sum(p, 0, false, &words) >= 0 && words == n;
}

static bool report_variant(Parser *p, const char *message, Span span) {
    if (parser_error(p, span, message, parser_describe(p, parser_peek(p, 0)))) {
        diag_note(p->diag, "Write each variant on its own line, like \"%s\"", variant_example);
    }
    return false;
}

// NAME (KIND): one field of a variant.
static bool parse_variant_field(Parser *p, Field *field) {
    static const char *const after[] = {"and", NULL};
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    if (!parser_parse_name(p, &field->name, after)) return false;
    if (!parser_at(p, 0, TOK_LPAREN)) {
        if (parser_error(p, field->name.pos.span, "Say what kind of value \"%s\" holds, in parentheses after it.",
                         field->name.display)) {
            diag_note(p->diag, "Write it like \"%s (a number)\".", field->name.display);
        }
        return false;
    }
    parser_advance(p);
    if (!parser_parse_kind(p, false, &field->type)) return false;
    if (!parser_at(p, 0, TOK_RPAREN)) {
        return report_variant(p, "I expected \")\" after the kind of value, but found %s.", parser_peek(p, 0)->span);
    }
    parser_advance(p);
    field->pos = ast_pos_join(start, (SourcePos){p->prev_span, 0, 0});
    return true;
}

// NAME [with FIELD (KIND) [and | ,] FIELD (KIND) ...]
static bool parse_variant(Parser *p, VariantDef *variant, size_t sum) {
    static const char *const with[] = {"with", NULL};
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    if (!parser_parse_name(p, &variant->name, with)) return false;
    variant->id = (size_t)-1;  // when the kind is defined twice (the checker says so)
    for (size_t i = 0; i < p->records->variants.len; i++) {
        const VariantKind *known = &p->records->variants.items[i];
        if (known->sum == sum && strcmp(known->name, variant->name.text) == 0) {
            variant->id = i;
            break;
        }
    }
    if (parser_at_word(p, 0, "with")) {
        parser_advance(p);
        do {
            Field field = {0};
            if (!parse_variant_field(p, &field)) return false;
            vec_push(p->arena, &variant->fields, field);
        } while ((parser_at_word(p, 0, "and") || parser_at(p, 0, TOK_COMMA)) && parser_advance(p));
    }
    variant->pos = ast_pos_join(start, (SourcePos){p->prev_span, 0, 0});
    if (parser_at_sentence_end(p)) return true;
    return report_variant(p, variant->fields.len > 0 ? "I expected the variant to end here, but found %s."
                                                     : "I expected \"with\" or the end of the variant here, but "
                                                       "found %s.",
                          parser_peek(p, 0)->span);
}

// The indented lines after the colon, one variant per sentence, up to and
// including the DEDENT that ends them.
static void parse_variants(Parser *p, Stmt *stmt) {
    while (!parser_at(p, 0, TOK_EOF)) {
        TokenKind kind = parser_peek(p, 0)->kind;
        if (kind == TOK_DEDENT) {
            parser_advance(p);
            return;
        }
        if (kind == TOK_NEWLINE || kind == TOK_PERIOD) {
            parser_advance(p);
            continue;
        }
        if (kind == TOK_INDENT) {  // reported, then skipped up to the end of its block
            const Token *indent = parser_advance(p);
            if (parser_error(p, indent->span, "This line is indented more than the variants above it.")) {
                diag_note(p->diag, "Line the variants up with each other.");
            }
            for (size_t depth = 1; depth > 0 && !parser_at(p, 0, TOK_EOF);) {
                TokenKind next = parser_advance(p)->kind;
                depth += next == TOK_INDENT ? 1 : next == TOK_DEDENT ? (size_t)-1 : 0;
            }
            p->sentence_failed = false;
            continue;
        }
        VariantDef variant = {0};
        if (parse_variant(p, &variant, stmt->as.sum.index)) vec_push(p->arena, &stmt->as.sum.variants, variant);
        while (!parser_at_sentence_end(p)) parser_advance(p);
        p->sentence_failed = false;
    }
}

Stmt *parse_sum(Parser *p) {
    static const char *const is[] = {"is", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_SUM, parser_peek(p, 0));
    size_t words;
    stmt->as.sum.index = (size_t)match_sum(p, 0, false, &words);
    bool ok = parser_parse_name(p, &stmt->as.sum.name, is);
    if (ok) {
        parser_consume(p, 4);  // is one of :
        stmt->pos = ast_pos_join(stmt->pos, (SourcePos){p->prev_span, 0, 0});
        if (!parser_at(p, 0, TOK_NEWLINE) || !parser_at(p, 1, TOK_INDENT)) {
            Span span = parser_at(p, 0, TOK_NEWLINE) ? p->prev_span : parser_peek(p, 0)->span;
            if (parser_error(p, span, "The variants of \"%s\" go on the lines below, indented.",
                             stmt->as.sum.name.display)) {
                diag_note(p->diag, "Write each one on its own line after \"%s\", like \"    %s\"", sum_example,
                          variant_example);
            }
            ok = false;
        }
    }
    if (!ok) {
        parser_skip_line_and_block(p);
        p->ended_with_block = true;
        return NULL;
    }
    parser_consume(p, 2);
    parse_variants(p, stmt);
    p->ended_with_block = true;
    RecordKind *kind = &p->records->sums.items[stmt->as.sum.index];
    if (!kind->definition) kind->definition = stmt;
    return stmt;
}
