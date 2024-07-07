#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

void print_banner() {
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

int compile_and_run(const char *source_file) {
    FILE *file = fopen(source_file, "r");
    if (!file) {
        printf("Error: Unable to open source file %s.\n", source_file);
        return 1;
    }

    fseek(file, 0, SEEK_END);
    long length = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *source = malloc(length + 1);
    fread(source, 1, length, file);
    fclose(file);
    source[length] = '\0';

    Lexer *lexer = create_lexer(source);
    Parser *parser = create_parser(lexer);
    AST *root = program(parser);

    FILE *output_file = fopen("output.c", "w");
    if (!output_file) {
        printf("Error: Unable to create output file.\n");
        free(source);
        return 1;
    }

    fprintf(output_file, "#include <stdio.h>\n");
    fprintf(output_file, "#include <stdlib.h>\n");
    fprintf(output_file, "int main() {\n");
    generate_code(root, output_file);
    fprintf(output_file, "return 0;\n");
    fprintf(output_file, "}\n");

    fclose(output_file);
    free(source);

    int result = system("gcc output.c -o output_program");
    if (result != 0) {
        printf("Error: Failed to compile the generated C code.\n");
        return 1;
    }

    result = system("./output_program");
    if (result != 0) {
        printf("Error: Failed to run the generated executable.\n");
        return 1;
    }

    return 0;
}

void interactive_shell() {
    print_banner();
    FILE *persistent_file = fopen("persistent_code.code", "w+");
    if (!persistent_file) {
        printf("Error: Unable to create persistent_code.code file.\n");
        return;
    }

    char line[1024];
    while (1) {
        printf(">> ");
        if (!fgets(line, sizeof(line), stdin)) {
            break;
        }

        // Remove newline character from the end of the input
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        if (strcmp(line, "EXIT") == 0) {
            break;
        }

        // Append the line to the persistent file
        fprintf(persistent_file, "%s\n", line);
        fflush(persistent_file);

        // Run the persistent file using easyscript
        int result = system("./easyscript persistent_code.code");
        if (result != 0) {
            printf("Error: Command execution failed.\n");
        }
    }

    fclose(persistent_file);
    // Clear the persistent file
    persistent_file = fopen("persistent_code.code", "w");
    if (persistent_file) {
        fclose(persistent_file);
    }
}

int main(int argc, char *argv[]) {
    if (argc > 1) {
        return compile_and_run(argv[1]);
    } else {
        interactive_shell();
    }

    return 0;
}
