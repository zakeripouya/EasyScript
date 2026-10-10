#include <stdio.h>
#include <stdlib.h>

static int compare(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

int main(void) {
    enum { N = 1000000 };
    double *numbers = malloc(N * sizeof *numbers);
    if (!numbers) return 1;
    double x = 42;
    for (int i = 0; i < N; i++) {
        x = (double)(((long long)x * 16807) % 2147483647);
        numbers[i] = x;
    }
    qsort(numbers, N, sizeof *numbers, compare);
    printf("%.0f %.0f %.0f\n", numbers[0], numbers[N / 2 - 1], numbers[N - 1]);
    free(numbers);
    return 0;
}
