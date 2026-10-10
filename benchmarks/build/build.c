#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A buffer that doubles when it's full, the usual way to build text in C. */
int main(void) {
    size_t len = 0, cap = 16;
    char *text = malloc(cap);
    if (!text) return 1;
    for (int i = 0; i < 1000000; i++) {
        if (len + 5 > cap) {
            cap *= 2;
            char *grown = realloc(text, cap);
            if (!grown) return 1;
            text = grown;
        }
        memcpy(text + len, "word ", 5);
        len += 5;
    }
    printf("%zu\n", len);
    free(text);
    return 0;
}
