#include <stdio.h>

int main(void) {
    volatile long total = 0;
    for (long row = 1; row <= 10000; row++)
        for (long column = 1; column <= 10000; column++)
            if ((row + column) % 3 == 0) total++;
    printf("%ld\n", total);
    return 0;
}
