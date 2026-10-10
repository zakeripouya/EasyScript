#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE  // macOS hides mkdtemp under strict feature macros

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include "common/arena.h"
#include "common/diag.h"
#include "common/util.h"
#include "front/lexer.h"
#include "front/parse.h"
#include "legacy.h"

static Arena *arena;

// Every generated file lives in one temp directory, removed at exit.
static char *temp_dir;
static char *temp_c_path;
static char *temp_bin_path;
static char *temp_session_path;

static void print_banner(void) {
    printf("========================================\n");
    printf("           EasyScript Language          \n");
    printf("========================================\n");
    printf("Available Commands:\n");
    printf("  PRINT ## - Print a value\n");
    printf("  MAKE A FOR LOOP - Create a for loop\n");
    printf("  MAKE A FUNCTION - Define a function\n");
    printf("  IF - Conditional statement\n");
    printf("  ELSE - Else statement\n");
    printf("  VARIABLE DECLARATION - Declare a variable\n");
    printf("  ASSIGNMENT - Assign a value to a variable\n");
    printf("  FILE OPEN filename - Open a file\n");
    printf("  FILE READ filename - Read from a file\n");
    printf("  FILE WRITE filename content - Write to a file\n");
    printf("  FILE CLOSE filename - Close a file\n");
    printf("  CALL function_name - Call a function\n");
    printf("  EXIT - Exit the interpreter\n");
    printf("========================================\n");
}

static void print_usage(FILE *out) {
    fprintf(out, "Usage:\n");
    fprintf(out, "  easyscript run FILE           Compile FILE and run it\n");
    fprintf(out, "  easyscript build FILE -o OUT  Compile FILE to the executable OUT\n");
    fprintf(out, "  easyscript emit FILE          Print the C code generated for FILE\n");
    fprintf(out, "  easyscript tokens FILE        Print the tokens of FILE, one per line\n");
    fprintf(out, "  easyscript ast FILE           Print the syntax tree of FILE as an outline\n");
    fprintf(out, "  easyscript                    Start the interactive shell\n");
}

static int usage_error(const char *message) {
    fprintf(stderr, "Error: %s\n", message);
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
    rmdir(temp_dir);
}

static void make_temp_dir(void) {
    const char *base = getenv("TMPDIR");
    if (!base || !*base) {
        base = "/tmp";
    }
    char *dir = arena_sprintf(arena, "%s/easyscript-XXXXXX", base);
    if (!mkdtemp(dir)) {
        fprintf(stderr, "Error: Unable to create temporary directory in %s: %s\n", base, strerror(errno));
        exit(1);
    }
    temp_dir = dir;
    temp_c_path = arena_sprintf(arena, "%s/program.c", temp_dir);
    temp_bin_path = arena_sprintf(arena, "%s/program", temp_dir);
    temp_session_path = arena_sprintf(arena, "%s/session.es", temp_dir);
    atexit(remove_temp_dir);
}

static char *read_file(const char *path, size_t *length_out) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Error: Unable to open source file %s.\n", path);
        exit(1);
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: Unable to read source file %s.\n", path);
        exit(1);
    }
    long length = ftell(file);
    if (length < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fprintf(stderr, "Error: Unable to read source file %s.\n", path);
        exit(1);
    }
    char *data = arena_alloc(arena, (size_t)length + 1);
    if (fread(data, 1, (size_t)length, file) != (size_t)length) {
        fprintf(stderr, "Error: Unable to read source file %s.\n", path);
        exit(1);
    }
    fclose(file);
    data[length] = '\0';
    if (length_out) {
        *length_out = (size_t)length;
    }
    return data;
}

// Compiles an EasyScript source file to C at temp_c_path.
static void generate_c(const char *source_path) {
    char *source = read_file(source_path, NULL);

    FILE *output_file = fopen(temp_c_path, "w");
    if (!output_file) {
        fprintf(stderr, "Error: Unable to create output file.\n");
        exit(1);
    }

    fprintf(output_file, "#include <stdio.h>\n");
    fprintf(output_file, "#include <stdlib.h>\n");
    fprintf(output_file, "int main() {\n");
    legacy_compile(source, output_file);
    fprintf(output_file, "return 0;\n");
    fprintf(output_file, "}\n");

    if (fclose(output_file) != 0) {
        fprintf(stderr, "Error: Unable to write output file.\n");
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
        fprintf(stderr, "Error: Unable to start %s: %s\n", argv[0], strerror(errno));
        return -1;
    }
    if (pid == 0) {
        execvp(argv[0], argv);
        fprintf(stderr, "Error: Unable to run %s: %s\n", argv[0], strerror(errno));
        _exit(127);  // _exit so the child doesn't run the parent's atexit cleanup
    }
    int status;
    while (waitpid(pid, &status, 0) < 0) {
        if (errno != EINTR) {
            fprintf(stderr, "Error: Unable to wait for %s: %s\n", argv[0], strerror(errno));
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

static int compile_c(const char *output_path) {
    char *argv[] = {"cc", "-O2", temp_c_path, "-o", (char *)output_path, NULL};
    if (run_process(argv) != 0) {
        fprintf(stderr, "Error: Failed to compile the generated C code.\n");
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
    tokens_dump(&tokens, &out);
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
    Block *program = parse_program(arena, diag, source, &tokens);

    StrBuf out;
    sb_init(&out, arena);
    ast_dump_block(program, &out);
    fwrite(out.data, 1, out.len, stdout);
    if (diag_count(diag) > 0) {
        diag_print(diag, stderr);
        return 1;
    }
    return 0;
}

static int cmd_build(const char *source_path, const char *output_path) {
    make_temp_dir();
    generate_c(source_path);
    return compile_c(output_path);
}

static int cmd_run(const char *source_path) {
    make_temp_dir();
    generate_c(source_path);
    if (compile_c(temp_bin_path) != 0) {
        return 1;
    }
    char *argv[] = {temp_bin_path, NULL};
    int status = run_process(argv);
    return status < 0 ? 1 : status;
}

static void interactive_shell(char *self) {
    print_banner();
    make_temp_dir();
    FILE *session_file = fopen(temp_session_path, "w");
    if (!session_file) {
        printf("Error: Unable to create session file.\n");
        return;
    }

    char *line = NULL;
    size_t cap = 0;
    while (1) {
        printf(">> ");
        fflush(stdout);
        ssize_t len = getline(&line, &cap, stdin);
        if (len < 0) {
            break;
        }

        // Remove newline character from the end of the input
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strcmp(line, "EXIT") == 0) {
            break;
        }

        // Append the line to the session file and rerun the whole session
        fprintf(session_file, "%s\n", line);
        fflush(session_file);

        char *argv[] = {self, "run", temp_session_path, NULL};
        if (run_process(argv) != 0) {
            printf("Error: Command execution failed.\n");
        }
    }

    free(line);
    fclose(session_file);
}

int main(int argc, char *argv[]) {
    arena = arena_new();
    atexit(free_arena);

    if (argc == 1) {
        interactive_shell(argv[0]);
        return 0;
    }

    const char *command = argv[1];
    if (strcmp(command, "help") == 0 || strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        print_usage(stdout);
        return 0;
    }

    const char *source_path = NULL;
    const char *output_path = NULL;
    for (int i = 2; i < argc; i++) {
        if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) {
                return usage_error("-o needs an output path.");
            }
            if (output_path) {
                return usage_error("-o given more than once.");
            }
            output_path = argv[++i];
        } else if (!source_path) {
            source_path = argv[i];
        } else {
            return usage_error("Only one source file can be given.");
        }
    }

    if (strcmp(command, "run") == 0 || strcmp(command, "emit") == 0 || strcmp(command, "build") == 0 ||
        strcmp(command, "tokens") == 0 || strcmp(command, "ast") == 0) {
        if (!source_path) {
            return usage_error("No source file given.");
        }
    } else {
        fprintf(stderr, "Error: Unknown command '%s'.\n", command);
        print_usage(stderr);
        return 2;
    }

    if (strcmp(command, "build") == 0) {
        if (!output_path) {
            return usage_error("build needs -o OUT.");
        }
        return cmd_build(source_path, output_path);
    }
    if (output_path) {
        return usage_error("-o is only valid with build.");
    }
    if (strcmp(command, "run") == 0) {
        return cmd_run(source_path);
    }
    if (strcmp(command, "tokens") == 0) {
        return cmd_tokens(source_path);
    }
    if (strcmp(command, "ast") == 0) {
        return cmd_ast(source_path);
    }
    return cmd_emit(source_path);
}
