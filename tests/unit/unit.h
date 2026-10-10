#ifndef UNIT_H
#define UNIT_H

// Minimal unit test harness. Each test is a function taking a TestContext;
// CHECK* macros record failures and keep going so one run shows them all.
// Output is one "PASS name" / "FAIL name" line per test, with failure details
// indented underneath, which tests/run.sh folds into its summary.

#include <stdio.h>
#include <string.h>
#include "common/arena.h"

typedef struct {
    Arena *arena;  // fresh per test
    int failures;  // in the current test
} TestContext;

typedef struct {
    int passed;
    int failed;
} TestRunner;

typedef void (*TestFn)(TestContext *t);

void unit_run(TestRunner *runner, const char *name, TestFn fn);
void unit_fail(TestContext *t, const char *file, int line, const char *fmt, ...);
void unit_check_str(TestContext *t, const char *file, int line, const char *expr,
                    const char *got, const char *want);

#define CHECK(t, cond)                                                   \
    do {                                                                 \
        if (!(cond)) unit_fail((t), __FILE__, __LINE__, "%s", #cond);    \
    } while (0)

#define CHECK_SIZE(t, got, want)                                                       \
    do {                                                                               \
        size_t got_ = (got), want_ = (want);                                           \
        if (got_ != want_)                                                             \
            unit_fail((t), __FILE__, __LINE__, "%s: got %zu, want %zu", #got, got_, want_); \
    } while (0)

#define CHECK_STR(t, got, want) unit_check_str((t), __FILE__, __LINE__, #got, (got), (want))

// Registration functions, one per test file.
void util_tests(TestRunner *runner);
void diag_tests(TestRunner *runner);
void lexer_tests(TestRunner *runner);

#endif
