// Kinds of values, as written in definitions, fields and empty lists and
// maps: "a number", "text", "yes or no", "a list of numbers", "a map from
// text to numbers", "a dog", "a dog or nothing".

#include <string.h>
#include "front/parse_internal.h"

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
        int record = parser_match_record(p, 0, true, &words);
        if (record >= 0) *type = type_record((size_t)record);
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
    if (type_is_record(*type)) {
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
        int record = parser_match_record(p, 0, false, &words);
        if (record >= 0) *type = type_record((size_t)record);
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

