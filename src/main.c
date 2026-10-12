#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE  // macOS hides mkdtemp under strict feature macros

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common/arena.h"
#include "common/diag.h"
#include "common/util.h"
#include "front/lexer.h"
#include "front/parse.h"
#include "front/check.h"
#include "back/codegen_c.h"
#include "back/source_table.h"
#include "modules.h"
#include "shell.h"

static Arena *arena;

// Every generated file lives in one temp directory, removed at exit.
static char *temp_dir;
static char *temp_c_path;
static char *temp_bin_path;
static char *temp_session_path;
static char *temp_answers_path;

static void print_usage(FILE *out) {
    fprintf(out, "Usage:\n");
    fprintf(out, "  easyscript run FILE [WORDS]   Compile FILE and run it, giving it the words after FILE\n");
    fprintf(out, "  easyscript test FILE [WORDS]  Run FILE and say whether its examples (\"for example, ...\") are right\n");
    fprintf(out, "  easyscript build FILE -o OUT  Compile FILE to the executable OUT\n");
    fprintf(out, "  easyscript emit FILE          Print the C code generated for FILE\n");
    fprintf(out, "  easyscript tokens FILE        Print the tokens of FILE, one per line\n");
    fprintf(out, "  easyscript ast FILE           Print the syntax tree of FILE as an outline\n");
    fprintf(out, "  easyscript errors FILE        Check FILE and print its errors as data, one per line\n");
    fprintf(out, "  easyscript plan FILE          Print what each list question and filter in FILE becomes\n");
    fprintf(out, "                                (also: easyscript build --plan FILE)\n");
    fprintf(out, "  easyscript                    Start the interactive shell\n");
}

static int usage_error(const char *message) {
    fprintf(stderr, "%s\n", message);
    print_usage(stderr);
    return 2;
}

static void free_arena(void) {
    arena_free(arena);
}

static void remove_temp_dir(void) {
    remove(temp_c_path);
    remove(temp_bin_path);
    remove(temp_session_path);
    remove(temp_answers_path);
    rmdir(temp_dir);
}

static void make_temp_dir(void) {
    const char *base = getenv("TMPDIR");
    if (!base || !*base) {
        base = "/tmp";
    }
    char *dir = arena_sprintf(arena, "%s/easyscript-XXXXXX", base);
    if (!mkdtemp(dir)) {
        fprintf(stderr, "I couldn't make a temporary folder in %s: %s.\n", base, strerror(errno));
        exit(1);
    }
    temp_dir = dir;
    temp_c_path = arena_sprintf(arena, "%s/program.c", temp_dir);
    temp_bin_path = arena_sprintf(arena, "%s/program", temp_dir);
    temp_session_path = arena_sprintf(arena, "%s/session.es", temp_dir);
    temp_answers_path = arena_sprintf(arena, "%s/answers.txt", temp_dir);
    atexit(remove_temp_dir);
}

// Why a file couldn't be opened, in plain words.
static const char *file_reason(int err) {
    switch (err) {
    case ENOENT: return "it doesn't exist";
    case EACCES: return "permission was denied";
    case EISDIR: return "it's a folder, not a file";
    default: return strerror(err);
    }
}

static _Noreturn void cannot_read(const char *path, int err) {
    fprintf(stderr, "I couldn't read the file \"%s\": %s.\n", path, file_reason(err));
    exit(1);
}

static char *read_file(const char *path, size_t *length_out) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "I couldn't open the file \"%s\": %s.\n", path, file_reason(errno));
        exit(1);
    }
    struct stat st;
    if (fstat(fileno(file), &st) == 0 && S_ISDIR(st.st_mode)) cannot_read(path, EISDIR);
    if (fseek(file, 0, SEEK_END) != 0) cannot_read(path, errno);
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) cannot_read(path, errno);
    char *data = arena_alloc(arena, (size_t)length + 1);
    if (fread(data, 1, (size_t)length, file) != (size_t)length) cannot_read(path, errno);
    fclose(file);
    data[length] = '\0';
    if (length_out) {
        *length_out = (size_t)length;
    }
    return data;
}

// Compiles an EasyScript source file to C at temp_c_path. On errors, prints
// them all and exits with status 1.
// Reads, lexes, parses and (if that went well) checks the program.
static Program *front_end(const char *source_path, Sources **sources) {
    *sources = sources_load(arena, source_path, getenv("ES_MODULE_DIR"));  // the shell's folder
    Diag *diag = (*sources)->diag;
    TokenList tokens = lex_files(arena, diag, (*sources)->source, (*sources)->len, (*sources)->starts, (*sources)->count);
    Program *program = parse_program(arena, diag, (*sources)->source, &tokens);
    program->files = (*sources)->files;
    if (diag_count(diag) == 0) {
        check_program(arena, diag, program);  // only on a cleanly parsed program
    }
    return program;
}

// The program's C code (and its plan, when `plan` isn't NULL). On errors,
// prints them all and exits with status 1.
static StrBuf generate(const char *source_path, StrBuf *plan) {
    Sources *sources;
    Program *program = front_end(source_path, &sources);
    Diag *diag = sources->diag;
    if (diag_count(diag) > 0) {
        diag_print(diag, stderr);
        exit(1);
    }
    if (diag_warning_count(diag) > 0) {  // the shell shows each line's warnings once
        const char *from = getenv("ES_WARN_FROM_LINE");
        diag_print_warnings(diag, from ? strtoul(from, NULL, 10) : 1, stderr);
    }

    StrBuf lines;  // the source lines, for runtime errors to show
    sb_init(&lines, arena);
    source_table_c(arena, sources->source, sources->len, sources->shown, sources->starts, sources->count, &lines);
    StrBuf code;
    sb_init(&code, arena);
    codegen_c(arena, program, lines.data, &code, plan);
    return code;
}

static void generate_c(const char *source_path) {
    StrBuf code = generate(source_path, NULL);
    FILE *output_file = fopen(temp_c_path, "w");
    if (!output_file) {
        fprintf(stderr, "I couldn't write the C file: %s.\n", strerror(errno));
        exit(1);
    }
    fwrite(code.data, 1, code.len, output_file);
    if (fclose(output_file) != 0) {
        fprintf(stderr, "I couldn't write the C file: %s.\n", strerror(errno));
        exit(1);
    }
}

// Runs a command and waits for it. Returns its exit status, 128 + signal
// number if it was killed, or -1 if it could not be waited on.
static int run_process(char *const argv[]) {
    fflush(stdout);
    fflush(stderr);
    pid_t pid = fork();
    if (pid < 0) {
        fprintf(stderr, "I couldn't start %s: %s.\n", argv[0], strerror(errno));
        return -1;
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "I couldn't run %s: %s.\n", argv[0], strerror(errno));
        _exit(127);  // _exit so the child doesn't run the parent's atexit cleanup
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            fprintf(stderr, "I couldn't wait for %s: %s.\n", argv[0], strerror(errno));
            return -1;
        }
    }
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    if (WIFSIGNALED(status)) {
        return 128 + WTERMSIG(status);
    }
    return -1;
}

static bool env_is_1(const char *name) {
    const char *value = getenv(name);
    return value && strcmp(value, "1") == 0;
}

// With ES_SANITIZE=1 (used by make test-debug), the program is built with
// AddressSanitizer and UBSan, so a reference-counting mistake in the runtime
// or the generated code (a use after free, a double free, or, on Linux, a
// leak) stops it with a report. `hooks` (only for "run" with ES_HOOKS=1, set
// by the tests and the shell) compiles in the runtime's environment hooks
// (ES_SKIP_OUTPUT, ES_ANSWERS, ES_DEBUG_MEMORY, ES_MEMORY_LIMIT); "build"
// never does, so programs given to others don't read them.
static int compile_c(const char *output_path, bool hooks, bool testing) {
    char *flags[3] = {NULL, NULL, NULL};  // last, so the list ends after the ones given
    size_t n = 0;
    if (hooks) flags[n++] = "-DES_HOOKS";
    if (testing) flags[n++] = "-DES_TESTING";  // "easyscript test": say how many examples were right
    char *plain[] = {"cc", "-O2", temp_c_path, "-o", (char *)output_path, "-lm", flags[0], flags[1], NULL};
    char *checked[] = {"cc", "-O1", "-fno-omit-frame-pointer", "-fsanitize=address,undefined",
                       "-fno-sanitize-recover=all", temp_c_path, "-o", (char *)output_path, "-lm", flags[0], flags[1], NULL};
    if (run_process(env_is_1("ES_SANITIZE") ? checked : plain) != 0) {
        fprintf(stderr, "I couldn't build the program from the C I made. This is a bug in EasyScript; please report it.\n");
        return 1;
    }
    return 0;
}

static int cmd_emit(const char *source_path) {
    make_temp_dir();
    generate_c(source_path);
    size_t length;
    char *code = read_file(temp_c_path, &length);
    fwrite(code, 1, length, stdout);
    return 0;
}

// Lexes FILE with the new lexer and prints its tokens. Lexer errors are
// printed after the tokens, to stderr.
static int cmd_tokens(const char *source_path) {
    size_t length;
    char *source = read_file(source_path, &length);
    Diag *diag = diag_new(arena, source, length);
    TokenList tokens = lex(arena, diag, source, length);

    StrBuf out;
    sb_init(&out, arena);
    tokens_dump(&tokens, source, &out);
    fwrite(out.data, 1, out.len, stdout);
    if (diag_count(diag) > 0) {
        diag_print(diag, stderr);
        return 1;
    }
    return 0;
}

// Parses FILE with the new front end and prints its syntax tree. Errors are
// printed after the tree, to stderr.
static int cmd_ast(const char *source_path) {
    size_t length;
    char *source = read_file(source_path, &length);
    Diag *diag = diag_new(arena, source, length);
    TokenList tokens = lex(arena, diag, source, length);
    Program *program = parse_program(arena, diag, source, &tokens);

    StrBuf out;
    sb_init(&out, arena);
    ast_dump_program(program, &out);
    fwrite(out.data, 1, out.len, stdout);
    if (diag_count(diag) > 0) {
        diag_print(diag, stderr);
        return 1;
    }
    return 0;
}

// Checks FILE and prints its errors and warnings as data, one JSON object per
// line (for tools, and a later notebook). Exits 1 if there are errors.
static int cmd_errors(const char *source_path) {
    Sources *sources;
    front_end(source_path, &sources);
    StrBuf out;
    sb_init(&out, arena);
    diag_render_data(sources->diag, &out);
    fwrite(out.data, 1, out.len, stdout);
    return diag_count(sources->diag) > 0 ? 1 : 0;
}

// Prints the plan: what each high-level form in the program became.
static int cmd_plan(const char *source_path) {
    StrBuf plan;
    sb_init(&plan, arena);
    generate(source_path, &plan);
    fwrite(plan.data, 1, plan.len, stdout);
    return 0;
}

static int cmd_build(const char *source_path, const char *output_path) {
    make_temp_dir();
    generate_c(source_path);
    return compile_c(output_path, false, false);
}

// Runs the program with `words` (the command line after the file) as the
// words given to it.
static int cmd_run(const char *source_path, char **words, int count, bool testing) {
    make_temp_dir();
    generate_c(source_path);
    if (compile_c(temp_bin_path, env_is_1("ES_HOOKS"), testing) != 0) {
        return 1;
    }
    char **argv = arena_alloc(arena, ((size_t)count + 2) * sizeof(char *));
    argv[0] = temp_bin_path;
    for (int i = 0; i < count; i++) argv[i + 1] = words[i];
    argv[count + 1] = NULL;
    int status = run_process(argv);
    return status < 0 ? 1 : status;
}

int main(int argc, char *argv[]) {
    arena = arena_new();
    atexit(free_arena);

    if (argc == 1) {
        make_temp_dir();
        shell_run(arena, argv[0], temp_session_path, temp_answers_path);
        return 0;
    }

    const char *command = argv[1];
    if (strcmp(command, "help") == 0 || strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        print_usage(stdout);
        return 0;
    }

    const char *source_path = NULL;
    const char *output_path = NULL;
    bool plan = false;  // "build --plan FILE" is "plan FILE"
    int words_at = argc;  // "run FILE WORDS...": the words given to the program
    for (int i = 2; i < argc; i++) {
        if (source_path && (strcmp(command, "run") == 0 || strcmp(command, "test") == 0)) {
            words_at = i;
            break;
        }
        if (strcmp(argv[i], "--plan") == 0 && strcmp(command, "build") == 0 && !plan) {
            plan = true;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                return usage_error("After -o, give the name of the program to make.");
            }
            if (output_path) {
                return usage_error("Give -o only once.");
            }
            output_path = argv[++i];
        } else if (!source_path) {
            source_path = argv[i];
        } else {
            return usage_error("Give one .es file at a time.");
        }
    }

    if (strcmp(command, "run") == 0 || strcmp(command, "test") == 0 || strcmp(command, "emit") == 0 || strcmp(command, "build") == 0 ||
        strcmp(command, "tokens") == 0 || strcmp(command, "ast") == 0 || strcmp(command, "errors") == 0 ||
        strcmp(command, "plan") == 0) {
        if (!source_path) {
            return usage_error("Tell me which .es file to use.");
        }
    } else {
        fprintf(stderr, "I don't know the command \"%s\".\n", command);
        print_usage(stderr);
        return 2;
    }

    if (plan) {
        if (output_path) return usage_error("--plan only prints the plan, so it doesn't take -o.");
        return cmd_plan(source_path);
    }
    if (strcmp(command, "build") == 0) {
        if (!output_path) {
            return usage_error("Tell me what to call the program with -o, like \"easyscript build hello.es -o hello\".");
        }
        return cmd_build(source_path, output_path);
    }
    if (output_path) {
        return usage_error("-o only works with build.");
    }
    if (strcmp(command, "run") == 0 || strcmp(command, "test") == 0) {  // "test": run it and check its examples
        return cmd_run(source_path, argv + words_at, argc - words_at, strcmp(command, "test") == 0);
    }
    if (strcmp(command, "tokens") == 0) {
        return cmd_tokens(source_path);
    }
    if (strcmp(command, "ast") == 0) {
        return cmd_ast(source_path);
    }
    if (strcmp(command, "errors") == 0) {
        return cmd_errors(source_path);
    }
    if (strcmp(command, "plan") == 0) {
        return cmd_plan(source_path);
    }
    return cmd_emit(source_path);
}
