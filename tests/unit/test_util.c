#include <stdint.h>
#include "common/util.h"
#include "unit.h"

static void test_arena_alignment(TestContext *t) {
    for (size_t size = 1; size < 100; size += 7) {
        void *p = arena_alloc(t->arena, size);
        CHECK(t, (uintptr_t)p % _Alignof(max_align_t) == 0);
    }
}

static void test_arena_large_allocation(TestContext *t) {
    size_t size = 1024 * 1024;  // bigger than one arena block
    unsigned char *p = arena_alloc(t->arena, size);
    CHECK(t, p[0] == 0 && p[size - 1] == 0);
    p[size - 1] = 7;
    CHECK(t, p[size - 1] == 7);
}

static void test_arena_sprintf(TestContext *t) {
    CHECK_STR(t, arena_sprintf(t->arena, "%s-%d", "x", 42), "x-42");
    CHECK_STR(t, arena_strdup(t->arena, "copy"), "copy");
}

static void test_sb_empty(TestContext *t) {
    StrBuf sb;
    sb_init(&sb, t->arena);
    CHECK_SIZE(t, sb.len, 0);
    CHECK_STR(t, sb.data, "");
}

static void test_sb_append(TestContext *t) {
    StrBuf sb;
    sb_init(&sb, t->arena);
    sb_append(&sb, "hello");
    sb_append_char(&sb, ',');
    sb_append_char(&sb, ' ');
    sb_append_n(&sb, "world!!!", 5);
    CHECK_STR(t, sb.data, "hello, world");
    CHECK_SIZE(t, sb.len, 12);
}

static void test_sb_appendf(TestContext *t) {
    StrBuf sb;
    sb_init(&sb, t->arena);
    sb_appendf(&sb, "Line %d: %s", 4, "oops");
    sb_appendf(&sb, " (%zu)", (size_t)99);
    CHECK_STR(t, sb.data, "Line 4: oops (99)");
}

static void test_sb_repeat(TestContext *t) {
    StrBuf sb;
    sb_init(&sb, t->arena);
    sb_append_repeat(&sb, '^', 4);
    sb_append_repeat(&sb, '-', 0);
    CHECK_STR(t, sb.data, "^^^^");
}

static void test_sb_grows(TestContext *t) {
    StrBuf sb;
    sb_init(&sb, t->arena);
    for (int i = 0; i < 10000; i++) {
        sb_append_char(&sb, (char)('a' + i % 26));
    }
    CHECK_SIZE(t, sb.len, 10000);
    CHECK(t, sb.data[10000] == '\0');
    int ok = 1;
    for (int i = 0; i < 10000; i++) {
        if (sb.data[i] != (char)('a' + i % 26)) ok = 0;
    }
    CHECK(t, ok);

    StrBuf big;
    sb_init(&big, t->arena);
    sb_appendf(&big, "%5000s|", "x");
    CHECK_SIZE(t, big.len, 5001);
    CHECK(t, big.data[4999] == 'x' && big.data[5000] == '|');
}

static void test_vec_push(TestContext *t) {
    Vec(int) numbers = {0};
    for (int i = 0; i < 1000; i++) {
        vec_push(t->arena, &numbers, i * 3);
    }
    CHECK_SIZE(t, numbers.len, 1000);
    CHECK(t, numbers.cap >= 1000);
    int ok = 1;
    for (int i = 0; i < 1000; i++) {
        if (numbers.items[i] != i * 3) ok = 0;
    }
    CHECK(t, ok);
    CHECK(t, vec_last(&numbers) == 999 * 3);
}

static void test_vec_of_structs(TestContext *t) {
    typedef struct { const char *name; int line; } Entry;
    Vec(Entry) entries = {0};
    Entry a = {"total", 1};
    Entry b = {"count", 2};
    vec_push(t->arena, &entries, a);
    vec_push(t->arena, &entries, b);
    CHECK_SIZE(t, entries.len, 2);
    CHECK_STR(t, entries.items[0].name, "total");
    CHECK(t, entries.items[1].line == 2);
}

static void test_edit_distance(TestContext *t) {
    CHECK_SIZE(t, edit_distance(t->arena, "", ""), 0);
    CHECK_SIZE(t, edit_distance(t->arena, "abc", ""), 3);
    CHECK_SIZE(t, edit_distance(t->arena, "", "abc"), 3);
    CHECK_SIZE(t, edit_distance(t->arena, "total", "total"), 0);
    CHECK_SIZE(t, edit_distance(t->arena, "totl", "total"), 1);   // insertion
    CHECK_SIZE(t, edit_distance(t->arena, "totall", "total"), 1); // deletion
    CHECK_SIZE(t, edit_distance(t->arena, "tital", "total"), 1);  // substitution
    CHECK_SIZE(t, edit_distance(t->arena, "kitten", "sitting"), 3);
    CHECK_SIZE(t, edit_distance(t->arena, "flaw", "lawn"), 2);
    CHECK_SIZE(t, edit_distance(t->arena, "A", "a"), 1);          // case-sensitive
}

static void test_edit_distance_symmetric(TestContext *t) {
    const char *words[] = {"", "a", "print", "prnit", "count", "counter", "total"};
    size_t n = sizeof(words) / sizeof(words[0]);
    int ok = 1;
    for (size_t i = 0; i < n; i++) {
        for (size_t j = 0; j < n; j++) {
            if (edit_distance(t->arena, words[i], words[j]) !=
                edit_distance(t->arena, words[j], words[i])) {
                ok = 0;
            }
        }
    }
    CHECK(t, ok);
}

void util_tests(TestRunner *runner) {
    unit_run(runner, "arena/alignment", test_arena_alignment);
    unit_run(runner, "arena/large-allocation", test_arena_large_allocation);
    unit_run(runner, "arena/sprintf", test_arena_sprintf);
    unit_run(runner, "strbuf/empty", test_sb_empty);
    unit_run(runner, "strbuf/append", test_sb_append);
    unit_run(runner, "strbuf/appendf", test_sb_appendf);
    unit_run(runner, "strbuf/repeat", test_sb_repeat);
    unit_run(runner, "strbuf/grows", test_sb_grows);
    unit_run(runner, "vec/push", test_vec_push);
    unit_run(runner, "vec/structs", test_vec_of_structs);
    unit_run(runner, "edit-distance/values", test_edit_distance);
    unit_run(runner, "edit-distance/symmetric", test_edit_distance_symmetric);
}
