// "easyscript new NAME [--template T]" and "easyscript new --list": a new
// program's folder, started from one of the templates in templates/ (each
// built into the compiler by tools/embed, so it works anywhere).

#define _POSIX_C_SOURCE 200809L

#include "new.h"

#include "common/arena.h"

#include <errno.h>
#include <stddef.h>
#include <string.h>
#include <sys/stat.h>

extern const unsigned char es_template_blank[], es_template_report[], es_template_quiz[], es_template_game[],
    es_template_tool[];
extern const size_t es_template_blank_len, es_template_report_len, es_template_quiz_len, es_template_game_len,
    es_template_tool_len;

static const struct {
    const char *name;
    const char *what;
    const unsigned char *text;
    const size_t *len;
} templates[] = {
    {"blank", "a program that says hello, to start from scratch", es_template_blank, &es_template_blank_len},
    {"report", "reads a table from a csv file, adds it up, and saves a chart", es_template_report, &es_template_report_len},
    {"quiz", "asks questions and keeps the score", es_template_quiz, &es_template_quiz_len},
    {"game", "a guessing game", es_template_game, &es_template_game_len},
    {"tool", "uses the words it's given (easyscript run main.es Ada Grace)", es_template_tool, &es_template_tool_len},
};

void new_list(FILE *out) {
    fprintf(out, "Templates for \"easyscript new NAME --template T\":\n");
    for (size_t i = 0; i < sizeof templates / sizeof templates[0]; i++) {
        fprintf(out, "  %-8s %s\n", templates[i].name, templates[i].what);
    }
    fprintf(out, "Without --template, the program starts from blank.\n");
}

int new_program(Arena *arena, const char *name, const char *template_name) {
    size_t t = 0, count = sizeof templates / sizeof templates[0];
    while (t < count && strcmp(templates[t].name, template_name) != 0) t++;
    if (t == count) {
        fprintf(stderr, "I don't know a template called \"%s\". The templates are blank, report, quiz, game and tool.\n",
                template_name);
        return 2;
    }
    struct stat st;
    if (stat(name, &st) == 0) {
        fprintf(stderr, "There's already a %s called \"%s\" here, so I didn't change it. Pick another name.\n",
                S_ISDIR(st.st_mode) ? "folder" : "file", name);
        return 1;
    }
    if (mkdir(name, 0777) != 0) {
        fprintf(stderr, "I couldn't make the folder \"%s\": %s.\n", name, strerror(errno));
        return 1;
    }
    const char *path = arena_sprintf(arena, "%s/main.es", name);
    FILE *file = fopen(path, "wb");
    if (!file || fwrite(templates[t].text, 1, *templates[t].len, file) != *templates[t].len || fclose(file) != 0) {
        fprintf(stderr, "I couldn't write \"%s\": %s.\n", path, strerror(errno));
        return 1;
    }
    printf("Made %s from the %s template. To run it:\n  cd %s\n  easyscript run main.es\n", path, templates[t].name, name);
    return 0;
}
