#include <string.h>
#include "common/diag.h"
#include "unit.h"

static Diag *diag_for(TestContext *t, const char *source) {
    return diag_new(t->arena, source, strlen(source));
}

// Span covering the first occurrence of `needle` at or after `from`.
static Span span_of(const char *source, const char *needle, size_t from) {
    const char *p = strstr(source + from, needle);
    Span span = {(size_t)(p - source), strlen(needle)};
    return span;
}

static const char *render(TestContext *t, const Diag *diag) {
    StrBuf out;
    sb_init(&out, t->arena);
    diag_render(diag, &out);
    return out.data;
}

static void test_spec_example(TestContext *t) {
    const char *source =
        "set total to 0.\n"
        "print total.\n"
        "\n"
        "add 5 to totl.\n";
    Diag *diag = diag_for(t, source);
    Span made = span_of(source, "total", 0);
    diag_error(diag, span_of(source, "totl", 0), "I don't know anything called \"%s\".", "totl");
    diag_note(diag, "Did you mean \"%s\"? You made it on line %zu.", "total", diag_line(diag, made.offset));

    CHECK_SIZE(t, diag_count(diag), 1);
    CHECK_STR(t, render(t, diag),
              "Line 4: I don't know anything called \"totl\".\n"
              "    add 5 to totl.\n"
              "             ^^^^\n"
              "Did you mean \"total\"? You made it on line 1.\n");
}

static void test_no_errors(TestContext *t) {
    Diag *diag = diag_for(t, "print 1.\n");
    CHECK_SIZE(t, diag_count(diag), 0);
    CHECK_STR(t, render(t, diag), "");
}

static void test_multiple_errors_in_report_order(TestContext *t) {
    const char *source = "say hi.\nmake x be 1.\nsay y.\n";
    Diag *diag = diag_for(t, source);
    diag_error(diag, span_of(source, "y", 24), "Second.");  // the "y" in "say y."
    diag_error(diag, span_of(source, "hi", 0), "First.");
    diag_note(diag, "Note one.");
    diag_note(diag, "Note two.");

    CHECK_SIZE(t, diag_count(diag), 2);
    CHECK_STR(t, render(t, diag),
              "Line 3: Second.\n"
              "    say y.\n"
              "        ^\n"
              "\n"
              "Line 1: First.\n"
              "    say hi.\n"
              "        ^^\n"
              "Note one.\n"
              "Note two.\n");
}

static void test_line_numbers(TestContext *t) {
    const char *source = "a\nbb\n\nccc";
    Diag *diag = diag_for(t, source);
    CHECK_SIZE(t, diag_line(diag, 0), 1);
    CHECK_SIZE(t, diag_line(diag, 1), 1);   // the newline belongs to line 1
    CHECK_SIZE(t, diag_line(diag, 2), 2);
    CHECK_SIZE(t, diag_line(diag, 5), 3);
    CHECK_SIZE(t, diag_line(diag, 6), 4);
    CHECK_SIZE(t, diag_line(diag, 9), 4);   // end of file
    CHECK_SIZE(t, diag_line(diag, 100), 4); // past the end clamps
}

static void test_zero_length_span_at_end_of_line(TestContext *t) {
    const char *source = "say hi\nnext.\n";
    Diag *diag = diag_for(t, source);
    Span end = {6, 0};  // the newline after "say hi"
    diag_error(diag, end, "Expected a period.");
    CHECK_STR(t, render(t, diag),
              "Line 1: Expected a period.\n"
              "    say hi\n"
              "          ^\n");
}

static void test_span_clamped_to_line(TestContext *t) {
    const char *source = "say \"unterminated\nnext.\n";
    Diag *diag = diag_for(t, source);
    Span span = {4, 100};
    diag_error(diag, span, "This text never ends.");
    CHECK_STR(t, render(t, diag),
              "Line 1: This text never ends.\n"
              "    say \"unterminated\n"
              "        ^^^^^^^^^^^^^\n");
}

static void test_tabs_are_kept_for_alignment(TestContext *t) {
    const char *source = "repeat 3 times:\n\tsay totl.\n";
    Diag *diag = diag_for(t, source);
    diag_error(diag, span_of(source, "totl", 0), "Unknown.");
    CHECK_STR(t, render(t, diag),
              "Line 2: Unknown.\n"
              "    \tsay totl.\n"
              "    \t    ^^^^\n");
}

static void test_utf8_counts_characters(TestContext *t) {
    const char *source = "say \"h\xc3\xa9llo\" to wrld.\n";  // "héllo"
    Diag *diag = diag_for(t, source);
    diag_error(diag, span_of(source, "\"h\xc3\xa9llo\"", 0), "Quoted.");
    diag_error(diag, span_of(source, "wrld", 0), "After.");
    CHECK_STR(t, render(t, diag),
              "Line 1: Quoted.\n"
              "    say \"h\xc3\xa9llo\" to wrld.\n"
              "        ^^^^^^^\n"
              "\n"
              "Line 1: After.\n"
              "    say \"h\xc3\xa9llo\" to wrld.\n"
              "                   ^^^^\n");
}

static void test_crlf_and_last_line_without_newline(TestContext *t) {
    const char *source = "say hi.\r\nsay byee";
    Diag *diag = diag_for(t, source);
    diag_error(diag, span_of(source, "hi", 0), "One.");
    diag_error(diag, span_of(source, "byee", 0), "Two.");
    CHECK_STR(t, render(t, diag),
              "Line 1: One.\n"
              "    say hi.\n"
              "        ^^\n"
              "\n"
              "Line 2: Two.\n"
              "    say byee\n"
              "        ^^^^\n");
}

static void test_empty_source(TestContext *t) {
    Diag *diag = diag_for(t, "");
    Span span = {0, 0};
    diag_error(diag, span, "Nothing to run.");
    CHECK_STR(t, render(t, diag),
              "Line 1: Nothing to run.\n"
              "    \n"
              "    ^\n");
}

void diag_tests(TestRunner *runner) {
    unit_run(runner, "diag/spec-example", test_spec_example);
    unit_run(runner, "diag/no-errors", test_no_errors);
    unit_run(runner, "diag/multiple-errors", test_multiple_errors_in_report_order);
    unit_run(runner, "diag/line-numbers", test_line_numbers);
    unit_run(runner, "diag/zero-length-span", test_zero_length_span_at_end_of_line);
    unit_run(runner, "diag/span-clamped", test_span_clamped_to_line);
    unit_run(runner, "diag/tabs", test_tabs_are_kept_for_alignment);
    unit_run(runner, "diag/utf8", test_utf8_counts_characters);
    unit_run(runner, "diag/crlf-and-eof", test_crlf_and_last_line_without_newline);
    unit_run(runner, "diag/empty-source", test_empty_source);
}
