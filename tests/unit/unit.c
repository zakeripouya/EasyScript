#include <stdarg.h>
#include "unit.h"

void unit_fail(TestContext *t, const char *file, int line, const char *fmt, ...) {
    t->failures++;
    printf("    %s:%d: ", file, line);
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
    printf("\n");
}

// Prints a string with each line indented, so multi-line diffs stay readable.
static void print_block(const char *label, const char *s) {
    printf("      %s:\n        |", label);
    for (; *s; s++) {
        if (*s == '\n') {
            printf("$\n        |");
        } else {
            putchar(*s);
        }
    }
    printf("\n");
}

void unit_check_str(TestContext *t, const char *file, int line, const char *expr,
                    const char *got, const char *want) {
    if (strcmp(got, want) == 0) return;
    unit_fail(t, file, line, "%s does not match", expr);
    print_block("got", got);
    print_block("want", want);
}

void unit_run(TestRunner *runner, const char *name, TestFn fn) {
    TestContext t = {arena_new(), 0};
    // Failure details print as checks fail, so the verdict line comes last.
    printf("RUN  %s\n", name);
    fn(&t);
    arena_free(t.arena);
    if (t.failures == 0) {
        runner->passed++;
        printf("PASS %s\n", name);
    } else {
        runner->failed++;
        printf("FAIL %s\n", name);
    }
}

int main(void) {
    TestRunner runner = {0, 0};
    util_tests(&runner);
    diag_tests(&runner);
    return runner.failed == 0 ? 0 : 1;
}
