#include <string.h>
#include "front/lexer.h"
#include "unit.h"

typedef struct {
    Diag *diag;
    TokenList tokens;
} Lexed;

static Lexed lex_string(TestContext *t, const char *source) {
    Lexed result;
    result.diag = diag_new(t->arena, source, strlen(source));
    result.tokens = lex(t->arena, result.diag, source, strlen(source));
    return result;
}

static const char *span_text(TestContext *t, const char *source, Span span) {
    char *text = arena_alloc(t->arena, span.length + 1);
    memcpy(text, source + span.offset, span.length);
    return text;
}

static void test_spans_cover_source_text(TestContext *t) {
    const char *source = "Say \"a\\tb\" <= 12.5.\n";
    Lexed lexed = lex_string(t, source);
    const char *want[] = {"Say", "\"a\\tb\"", "<=", "12.5", ".", "\n", ""};
    CHECK_SIZE(t, lexed.tokens.len, 7);
    for (size_t i = 0; i < lexed.tokens.len && i < 7; i++) {
        CHECK_STR(t, span_text(t, source, lexed.tokens.items[i].span), want[i]);
    }
    CHECK_SIZE(t, diag_count(lexed.diag), 0);
}

static void test_word_text_is_lowercased(TestContext *t) {
    Lexed lexed = lex_string(t, "GuEsT's");
    CHECK(t, lexed.tokens.items[0].kind == TOK_WORD);
    CHECK_STR(t, lexed.tokens.items[0].text, "guest's");
    CHECK_SIZE(t, lexed.tokens.items[0].text_len, 7);
}

static void test_string_value_is_decoded(TestContext *t) {
    Lexed lexed = lex_string(t, "\"x\\n\\\"y\\\\\"");
    Token token = lexed.tokens.items[0];
    CHECK(t, token.kind == TOK_STRING);
    CHECK_SIZE(t, token.text_len, 5);
    CHECK(t, memcmp(token.text, "x\n\"y\\", 5) == 0);
}

static void test_filler_leaves_no_trace(TestContext *t) {
    Lexed lexed = lex_string(t, "The A an THE\n");
    CHECK_SIZE(t, lexed.tokens.len, 1);
    CHECK(t, lexed.tokens.items[0].kind == TOK_EOF);
}

static void test_always_ends_with_eof(TestContext *t) {
    const char *sources[] = {"", "\n", "say", "if x:\n    say \"unterminated", "@@@"};
    for (size_t i = 0; i < sizeof(sources) / sizeof(sources[0]); i++) {
        Lexed lexed = lex_string(t, sources[i]);
        CHECK(t, lexed.tokens.len > 0);
        CHECK(t, lexed.tokens.items[lexed.tokens.len - 1].kind == TOK_EOF);
    }
}

static void test_indents_and_dedents_balance(TestContext *t) {
    Lexed lexed = lex_string(t, "a1:\n  b:\n      c\n  d\n    e\nf");
    int depth = 0;
    int min_depth = 0;
    for (size_t i = 0; i < lexed.tokens.len; i++) {
        if (lexed.tokens.items[i].kind == TOK_INDENT) depth++;
        if (lexed.tokens.items[i].kind == TOK_DEDENT) depth--;
        if (depth < min_depth) min_depth = depth;
    }
    CHECK(t, depth == 0);
    CHECK(t, min_depth == 0);
}

static void test_errors_are_collected(TestContext *t) {
    Lexed lexed = lex_string(t, "say @.\nsay \"open\nsay 3x.\n");
    CHECK_SIZE(t, diag_count(lexed.diag), 3);
}

static void test_utf8_columns(TestContext *t) {
    Lexed lexed = lex_string(t, "say \"h\xc3\xa9\" to x");
    CHECK_SIZE(t, lexed.tokens.items[2].column, 10);  // "to", after a 4-character string
}

void lexer_tests(TestRunner *runner) {
    unit_run(runner, "lexer/spans", test_spans_cover_source_text);
    unit_run(runner, "lexer/lowercase", test_word_text_is_lowercased);
    unit_run(runner, "lexer/string-decoding", test_string_value_is_decoded);
    unit_run(runner, "lexer/filler", test_filler_leaves_no_trace);
    unit_run(runner, "lexer/eof", test_always_ends_with_eof);
    unit_run(runner, "lexer/indent-balance", test_indents_and_dedents_balance);
    unit_run(runner, "lexer/errors-collected", test_errors_are_collected);
    unit_run(runner, "lexer/utf8-columns", test_utf8_columns);
}
