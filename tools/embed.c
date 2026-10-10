// Build tool: turns a file into a C source file defining
//     const unsigned char NAME[];  const size_t NAME_len;
// so the compiler can carry the runtime inside itself.
//
//     embed INPUT OUTPUT NAME

#include <stdio.h>
#include <stdlib.h>

static char *read_all(const char *path, long *size) {
    FILE *f = fopen(path, "rb");
    if (!f || fseek(f, 0, SEEK_END) != 0 || (*size = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0) {
        fprintf(stderr, "embed: can't read %s\n", path);
        exit(1);
    }
    char *data = malloc((size_t)*size + 1);
    if (!data || fread(data, 1, (size_t)*size, f) != (size_t)*size) {
        fprintf(stderr, "embed: can't read %s\n", path);
        exit(1);
    }
    fclose(f);
    return data;
}

int main(int argc, char **argv) {
    if (argc != 4) {
        fprintf(stderr, "usage: embed INPUT OUTPUT NAME\n");
        return 2;
    }
    long size;
    char *data = read_all(argv[1], &size);
    FILE *out = fopen(argv[2], "w");
    if (!out) {
        fprintf(stderr, "embed: can't write %s\n", argv[2]);
        return 1;
    }
    fprintf(out, "// Generated from %s by tools/embed.c. Do not edit.\n", argv[1]);
    fprintf(out, "#include <stddef.h>\n\nconst unsigned char %s[] = {", argv[3]);
    for (long i = 0; i < size; i++) {
        fprintf(out, "%s%u,", i % 16 == 0 ? "\n    " : " ", (unsigned char)data[i]);
    }
    fprintf(out, "\n    0\n};\nconst size_t %s_len = %ld;\n", argv[3], size);
    free(data);
    if (fclose(out) != 0) {
        fprintf(stderr, "embed: can't write %s\n", argv[2]);
        return 1;
    }
    return 0;
}
