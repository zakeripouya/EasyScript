/* Functions given to built-ins: map, filter and reduce over a million
 * numbers, through function pointers. */
#include <stdio.h>
#include <stdlib.h>

static double double_it(double x) { return x * 2; }
static int divisible_by_three(double x) { return (long long)x % 3 == 0; }
static double sum(double a, double b) { return a + b; }
static double one_more(double x) { return x + 1; }

int main(void) {
    size_t n = 1000000;
    double *numbers = malloc(n * sizeof *numbers), *doubled = malloc(n * sizeof *doubled);
    double *kept = malloc(n * sizeof *kept);
    long long x = 42;
    for (size_t i = 0; i < n; i++) {
        x = x * 16807 % 2147483647;
        numbers[i] = (double)(x % 1000);
    }
    double (*volatile map)(double) = double_it; /* volatile: really called through the pointer */
    int (*volatile keep)(double) = divisible_by_three;
    double (*volatile combine)(double, double) = sum;
    double (*volatile step)(double) = one_more;
    double total = 0;
    for (int round = 0; round < 10; round++) {
        for (size_t i = 0; i < n; i++) doubled[i] = map(numbers[i]);
        size_t m = 0;
        for (size_t i = 0; i < n; i++) {
            if (keep(doubled[i])) kept[m++] = doubled[i];
        }
        double part = 0;
        for (size_t i = 0; i < m; i++) part = combine(part, kept[i]);
        total += part;
        for (size_t i = 0; i < n; i++) numbers[i] = step(numbers[i]);
    }
    printf("%.0f\n", total);
    free(numbers);
    free(doubled);
    free(kept);
    return 0;
}
