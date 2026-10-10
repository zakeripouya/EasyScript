#include <stdio.h>

int main(void) {
    volatile long long total = 0; /* volatile: keep the loop from being folded away */
    for (long long i = 1; i <= 100000000LL; i++) total += i;
    printf("%lld\n", total / 1000000);
    return 0;
}
