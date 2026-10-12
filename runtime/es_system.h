/* EasyScript runtime, part 16: what the program is given when it starts,
 * settings, the error output, and running other programs.
 *
 * Other programs are run from a list of words, never through a shell: the
 * first word is the program (found in the folders of PATH, like a terminal
 * does, unless it has a "/"), and each other word reaches it exactly as
 * written, so nothing in them (spaces, quotes, ";", "$", "*") means anything
 * special. */
#ifndef ES_SYSTEM_H
#define ES_SYSTEM_H

#ifndef ES_KEYS_H
#include "es_keys.h"
#endif

#include <fcntl.h>
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>

/* --- The words given to the program ----------------------------------------- */

static int es_argc;
static char **es_argv;

static void es_args(int argc, char **argv) {
    es_argc = argc;
    es_argv = argv;
}

static size_t es_word_count(void) {
    return es_argc > 1 ? (size_t)es_argc - 1 : 0;
}

/* "the words given to the program": a new list of text. */
static EsList *es_program_words(void) {
    size_t n = es_word_count();
    EsList *words = es_list_new(ES_ITEMS_TEXT, n);
    for (size_t i = 0; i < n; i++) es_push_t(words, es_text_copy(es_argv[i + 1], strlen(es_argv[i + 1])));
    return words;
}

/* "word N given to the program". */
static EsText *es_program_word(int site, double n) {
    size_t count = es_word_count();
    if (n != floor(n)) es_fail(site, NULL, "The position of a word has to be a whole number, but it's %s.", es_shown(n));
    if (!(n >= 1 && n <= (double)count)) {
        const char *hint = "Positions start at 1: the first word is word 1.";
        if (n >= 1) {
            static const char *const format = "Check how many there are first, like \"if the number of words given to "
                                              "the program is at least %s:\".";
            const char *shown = es_shown(n);
            size_t size = strlen(format) + strlen(shown);
            char *text = malloc(size);
            if (!text) es_out_of_memory();
            snprintf(text, size, format, shown);
            hint = text;
        }
        if (count == 0) es_fail(site, hint, "There's no word %s: the program wasn't given any words.", es_shown(n));
        es_fail(site, hint, "There's no word %s: the program was given %zu word%s.", es_shown(n), count,
                count == 1 ? "" : "s");
    }
    const char *word = es_argv[(size_t)n];
    return es_text_copy(word, strlen(word));
}

/* --- The text sent to the program ------------------------------------------- */

/* Everything left to read (after any "ask"s), once; later uses give the same
 * text. Kept in es_sent (es_program.h), which es_finish lets go of. */
static EsText *es_sent_text(void) {
    if (!es_sent) {
        fflush(stdout); /* show any question first */
        es_terminal_normal();
        size_t len = 0, cap = 4096;
        char *buffer = malloc(cap);
        if (!buffer) es_out_of_memory();
        size_t got;
        while ((got = fread(buffer + len, 1, cap - len, stdin)) > 0) {
            len += got;
            if (len == cap) {
                cap *= 2;
                char *bigger = realloc(buffer, cap);
                if (!bigger) es_out_of_memory();
                buffer = bigger;
            }
        }
        clearerr(stdin);
        if (len > 0 && buffer[len - 1] == '\n') len--; /* like "the contents of file" */
        if (len > 0 && buffer[len - 1] == '\r') len--;
        es_sent = len > 0 ? es_text_copy(buffer, len) : &es_empty_text;
        free(buffer);
    }
    return es_retain(es_sent);
}

/* --- Settings, complaining, finishing -------------------------------------------- */

/* "the setting called NAME": its text, or nothing (NULL) if it isn't set. Consumes name. */
static EsRecord *es_setting(EsText *name) {
    const char *value = getenv(name->chars);
    es_release(name);
    return value ? es_box_t(es_text_copy(value, strlen(value))) : NULL;
}

/* "complain X": X and a new line on the error output, after what's been printed. Consumes t. */
static void es_complain(EsText *t) {
    fflush(stdout);
    fwrite(t->chars, 1, t->len, stderr);
    fputc('\n', stderr);
    es_release(t);
}

/* "stop the program with code N": what the program tells whoever started it. */
static void es_set_status(int site, double code) {
    if (!(code >= 0 && code <= 255 && code == floor(code))) {
        es_fail(site, "0 means it worked; any other code means it didn't, and the program that started it decides "
                      "what each one means.",
                "The code has to be a whole number from 0 to 255, but it's %s.", es_shown(code));
    }
    es_status = (int)code;
}

/* --- Running other programs ------------------------------------------------------- */

/* The words of a command, as the program and then its words. Consumes both. */
static EsList *es_command(EsText *program, EsList *words) {
    EsList *all = es_list_new(ES_ITEMS_TEXT, words->len + 1);
    es_push_t(all, program);
    for (size_t i = 0; i < words->len; i++) es_push_t(all, es_retain(words->at.t[i]));
    es_list_release(words);
    return all;
}

/* "with WORDS" where WORDS is a list of text: adds its items. Consumes list. */
static void es_command_add_list(EsList *words, EsList *list) {
    for (size_t i = 0; i < list->len; i++) es_push_t(words, es_retain(list->at.t[i]));
    es_list_release(list);
}

typedef struct {
    char *data;
    size_t len, cap;
} EsBytes;

static void es_bytes_read(EsBytes *b, int fd, bool *open) {
    if (b->cap - b->len < 4096) {
        b->cap = b->cap ? b->cap * 2 : 8192;
        char *bigger = realloc(b->data, b->cap);
        if (!bigger) es_out_of_memory();
        b->data = bigger;
    }
    ssize_t n = read(fd, b->data + b->len, b->cap - b->len);
    if (n > 0) {
        b->len += (size_t)n;
    } else if (n == 0 || errno != EINTR) {
        *open = false;
    }
}

/* What a program printed, as text without its last new line (like "the contents of file"). */
static EsText *es_bytes_text(EsBytes *b) {
    size_t len = b->len;
    if (len > 0 && b->data[len - 1] == '\n') len--;
    if (len > 0 && b->data[len - 1] == '\r') len--;
    EsText *t = len > 0 ? es_text_copy(b->data, len) : &es_empty_text;
    free(b->data);
    return t;
}

/* Reads both outputs until the program closes them (both at once, so a
 * program filling one while the other is read never waits forever). */
static void es_collect(int out_fd, int err_fd, EsBytes *out, EsBytes *err) {
    bool out_open = true, err_open = true;
    while (out_open || err_open) {
        struct pollfd fds[2] = {{out_open ? out_fd : -1, POLLIN, 0}, {err_open ? err_fd : -1, POLLIN, 0}};
        if (poll(fds, 2, -1) < 0) {
            if (errno == EINTR) continue;
            break;
        }
        if (fds[0].revents) es_bytes_read(out, out_fd, &out_open);
        if (fds[1].revents) es_bytes_read(err, err_fd, &err_open);
    }
}

static void es_close_on_exec(int fd) {
    fcntl(fd, F_SETFD, FD_CLOEXEC);
}

/* Starts the command (its first word the program) and waits for it, giving
 * back its code: what it exited with, or 128 plus the signal that stopped
 * it. With out and err, its output and error output are collected into
 * them; otherwise it shares the program's. Consumes command. */
static double es_spawn(int site, EsList *command, EsText **out, EsText **err) {
    const char *name = command->at.t[0]->chars;
    if (command->at.t[0]->len == 0) es_fail(site, NULL, "The name of the program to run is empty.");
    char **argv = malloc((command->len + 1) * sizeof(char *));
    if (!argv) es_out_of_memory();
    for (size_t i = 0; i < command->len; i++) argv[i] = (char *)command->at.t[i]->chars;
    argv[command->len] = NULL;
    es_terminal_normal(); /* it gets the terminal the way it was */
    fflush(stdout);
    fflush(stderr);
    int report[2], out_pipe[2] = {-1, -1}, err_pipe[2] = {-1, -1};
    if (pipe(report) != 0 || (out && (pipe(out_pipe) != 0 || pipe(err_pipe) != 0))) {
        es_fail(site, NULL, "I couldn't start \"%s\": %s.", name, es_reason(errno));
    }
    es_close_on_exec(report[0]);
    es_close_on_exec(report[1]);
    pid_t pid = fork();
    if (pid < 0) es_fail(site, NULL, "I couldn't start \"%s\": %s.", name, es_reason(errno));
    if (pid == 0) { /* the new program: its outputs into the pipes, then it replaces this copy */
        if (out) {
            dup2(out_pipe[1], STDOUT_FILENO);
            dup2(err_pipe[1], STDERR_FILENO);
            close(out_pipe[0]), close(out_pipe[1]), close(err_pipe[0]), close(err_pipe[1]);
        }
        execvp(argv[0], argv);
        int why = errno;
        ssize_t sent = write(report[1], &why, sizeof why); /* tells the program why it couldn't start */
        (void)sent;
        _exit(127);
    }
    close(report[1]);
    EsBytes out_bytes = {NULL, 0, 0}, err_bytes = {NULL, 0, 0};
    if (out) {
        close(out_pipe[1]);
        close(err_pipe[1]);
        es_collect(out_pipe[0], err_pipe[0], &out_bytes, &err_bytes);
        close(out_pipe[0]);
        close(err_pipe[0]);
    }
    int why = 0;
    ssize_t got;
    while ((got = read(report[0], &why, sizeof why)) < 0 && errno == EINTR) {
    }
    close(report[0]);
    int status = 0;
    while (waitpid(pid, &status, 0) < 0 && errno == EINTR) {
    }
    if (got == (ssize_t)sizeof why) {
        if (why == ENOENT) {
            es_fail(site, strchr(name, '/') ? NULL : "Check its name, and that it's installed. A program in this folder "
                                                     "is run with its path, like \"./tool\".",
                    "I couldn't find a program called \"%s\".", name);
        }
        es_fail(site, NULL, "I couldn't run \"%s\": %s.", name, why == EACCES ? "it isn't a program I'm allowed to run"
                                                                              : es_reason(why));
    }
    free(argv);
    double code = WIFEXITED(status) ? WEXITSTATUS(status) : WIFSIGNALED(status) ? 128 + WTERMSIG(status) : 1;
    if (out) {
        *out = es_bytes_text(&out_bytes);
        *err = es_bytes_text(&err_bytes);
    }
    es_list_release(command);
    return code;
}

/* "R worked" (yes false: "R didn't work"), from R's code. */
static bool es_worked(double code, bool yes) {
    return (code == 0) == yes;
}

/* "run P with WORDS.": its output is shown as it comes, and a code other than
 * 0 stops this program. Consumes command. */
static void es_run(int site, EsList *command) {
    EsText *name = es_retain(command->at.t[0]);
    double code = es_spawn(site, command, NULL, NULL);
    if (code != 0) {
        es_fail(site, "To carry on when it doesn't work, write \"... and call the result r\", then check \"if r "
                      "worked:\".",
                "\"%s\" didn't work: it finished with code %s.", name->chars, es_shown(code));
    }
    es_release(name);
}

#endif
