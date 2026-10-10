// The interactive shell. It keeps the source of everything typed so far and,
// for each new entry, runs the whole session again with `easyscript run`:
//
// - Output that earlier runs already showed is suppressed: the program gets
//   ES_SKIP_OUTPUT (bytes to skip), and its output comes through a pipe so
//   the shell can count what's new.
// - Answers to earlier "ask"s are replayed from the ES_ANSWERS file instead
//   of being asked again; new answers are added to it.
// - An entry that doesn't compile, or stops with an error, is dropped (its
//   errors are shown) and the answers file is cut back.
// - A line ending in ":" starts a block (an if, a loop, a function): lines
//   are read until a blank one.
//
// Input is read one byte at a time, so the shell never reads ahead into lines
// meant for the program it's running (answers to "ask").

#define _XOPEN_SOURCE 700

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common/util.h"
#include "shell.h"

#define CHUNK 4096  // how much program output to copy at a time (not a limit)

typedef struct {
    Arena *arena;
    char *self;
    const char *session_path;
    const char *answers_path;
    StrBuf session;  // everything kept so far
    StrBuf line;     // the line being read
    size_t shown;    // bytes of program output already shown
    char *chunk;
} Shell;

static void print_banner(void) {
    printf("EasyScript interactive shell. Type a sentence, like: say \"hello\"\n");
    printf("End a line with \":\" to start a block (an if, a loop, or a function); a blank line ends it.\n");
    printf("Lines with errors aren't kept. Type exit to leave.\n");
}

// --- Reading -------------------------------------------------------------------

// One line from standard input, without its newline. False at the end of input.
static bool read_line(Shell *sh) {
    sh->line.len = 0;
    sh->line.data[0] = '\0';
    bool any = false;
    char c;
    for (;;) {
        ssize_t n = read(STDIN_FILENO, &c, 1);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        any = true;
        if (c == '\n') break;
        sb_append_char(&sh->line, c);
    }
    if (sh->line.len > 0 && sh->line.data[sh->line.len - 1] == '\r') sh->line.data[--sh->line.len] = '\0';
    return any;
}

static size_t trimmed_length(const char *s, size_t len) {
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t')) len--;
    return len;
}

static bool is_blank(const StrBuf *line) {
    for (size_t i = 0; i < line->len; i++) {
        if (line->data[i] != ' ' && line->data[i] != '\t') return false;
    }
    return true;
}

static bool opens_block(const StrBuf *line) {
    size_t len = trimmed_length(line->data, line->len);
    return len > 0 && line->data[len - 1] == ':';
}

static void prompt(const char *text) {
    fputs(text, stdout);
    fflush(stdout);
}

// One entry: a line, or a block that runs until a blank line. False at the
// end of input. *lines counts its lines.
static bool read_entry(Shell *sh, StrBuf *entry, size_t *lines) {
    entry->len = 0;
    entry->data[0] = '\0';
    *lines = 0;
    prompt(">> ");
    if (!read_line(sh)) return false;
    sb_append_n(entry, sh->line.data, sh->line.len);
    *lines = 1;
    if (!opens_block(&sh->line)) return true;
    for (;;) {
        prompt("... ");
        if (!read_line(sh) || is_blank(&sh->line)) return true;
        sb_append_char(entry, '\n');
        sb_append_n(entry, sh->line.data, sh->line.len);
        (*lines)++;
    }
}

// "exit", "quit", or "stop the program" (any case, an optional period) leave.
static bool is_leave_command(const StrBuf *entry) {
    static const char *const commands[] = {"exit", "quit", "stop the program"};
    size_t len = trimmed_length(entry->data, entry->len);
    if (len > 0 && entry->data[len - 1] == '.') len--;
    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strlen(commands[i]) != len) continue;
        size_t j = 0;
        while (j < len && (entry->data[j] | 0x20) == commands[i][j]) j++;
        if (j == len) return true;
    }
    return false;
}

// --- Running -------------------------------------------------------------------

static bool write_session(const Shell *sh) {
    FILE *file = fopen(sh->session_path, "w");
    if (!file) return false;
    fwrite(sh->session.data, 1, sh->session.len, file);
    return fclose(file) == 0;
}

static long file_size(const char *path) {
    struct stat st;
    return stat(path, &st) == 0 ? (long)st.st_size : 0;
}

// Runs `self run session` with its output piped through the shell. Returns
// its exit status and adds the bytes it printed to *printed.
static int run_session(Shell *sh, size_t *printed) {
    int fds[2];
    if (pipe(fds) != 0) return -1;
    fflush(stdout);
    pid_t pid = fork();
    if (pid < 0) {
        close(fds[0]);
        close(fds[1]);
        return -1;
    }
    if (pid == 0) {
        close(fds[0]);
        dup2(fds[1], STDOUT_FILENO);
        close(fds[1]);
        setenv("ES_SKIP_OUTPUT", arena_sprintf(sh->arena, "%zu", sh->shown), 1);
        setenv("ES_ANSWERS", sh->answers_path, 1);
        char *argv[] = {sh->self, "run", (char *)sh->session_path, NULL};
        execvp(argv[0], argv);
        fprintf(stderr, "I couldn't run %s: %s.\n", argv[0], strerror(errno));
        _exit(127);  // _exit so the child doesn't run the parent's atexit cleanup
    }
    close(fds[1]);
    for (;;) {
        ssize_t n = read(fds[0], sh->chunk, CHUNK);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) break;
        fwrite(sh->chunk, 1, (size_t)n, stdout);
        fflush(stdout);
        *printed += (size_t)n;
    }
    close(fds[0]);
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) return -1;
    }
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

// Tries one entry with everything kept so far; keeps it only if it all works.
static void try_entry(Shell *sh, const StrBuf *entry, size_t lines) {
    size_t kept = sh->session.len;
    long answers = file_size(sh->answers_path);
    sb_append_n(&sh->session, entry->data, entry->len);
    sb_append_char(&sh->session, '\n');
    size_t printed = 0;
    if (write_session(sh) && run_session(sh, &printed) == 0) {
        sh->shown += printed;
        return;
    }
    sh->session.len = kept;
    sh->session.data[kept] = '\0';
    if (truncate(sh->answers_path, answers) != 0) {
        fprintf(stderr, "I couldn't set up the shell's answers file: %s.\n", strerror(errno));
    }
    printf(lines > 1 ? "(That block wasn't kept.)\n" : "(That line wasn't kept.)\n");
}

void shell_run(Arena *arena, char *self, const char *session_path, const char *answers_path) {
    Shell sh = {0};
    sh.arena = arena;
    sh.self = self;
    sh.session_path = session_path;
    sh.answers_path = answers_path;
    sb_init(&sh.session, arena);
    sb_init(&sh.line, arena);
    sh.chunk = arena_alloc(arena, CHUNK);
    FILE *answers = fopen(answers_path, "w");
    if (!answers) {
        fprintf(stderr, "I couldn't set up the shell's answers file: %s.\n", strerror(errno));
        return;
    }
    fclose(answers);

    print_banner();
    StrBuf entry;
    sb_init(&entry, arena);
    size_t lines;
    while (read_entry(&sh, &entry, &lines)) {
        if (is_blank(&entry)) continue;
        if (is_leave_command(&entry)) break;
        try_entry(&sh, &entry, lines);
    }
}
