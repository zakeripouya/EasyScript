#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"

typedef struct {
    char name[256];
    int id;
    int is_string;
} VarMap;

VarMap var_map[100];
int var_map_index = 0;

int var_counter = 0;

void sanitize_filename(char *filename, char *sanitized) {
    for (int i = 0; i < strlen(filename); i++) {
        if (filename[i] == '.') {
            sanitized[i] = '_';
        } else {
            sanitized[i] = filename[i];
        }
    }
    sanitized[strlen(filename)] = '\0';
}

int get_var_id(char *name) {
    for (int i = 0; i < var_map_index; i++) {
        if (strcmp(var_map[i].name, name) == 0) {
            return var_map[i].id;
        }
    }
    return -1;
}

int is_var_string(char *name) {
    for (int i = 0; i < var_map_index; i++) {
        if (strcmp(var_map[i].name, name) == 0) {
            return var_map[i].is_string;
        }
    }
    return 0;
}

void add_var(char *name, int is_string) {
    strcpy(var_map[var_map_index].name, name);
    var_map[var_map_index].id = var_counter;
    var_map[var_map_index].is_string = is_string;
    var_map_index++;
}

void generate_code(AST *node, FILE *output_file) {
    while (node) {
        if (node->type == AST_INT) {
            fprintf(output_file, "%d", node->value);
        } else if (node->type == AST_VAR) {
            int id = get_var_id(node->token->value);
            if (id == -1) {
                fprintf(stderr, "Error: Variable not found for %s\n", node->token->value);
                exit(1);
            }
            fprintf(output_file, "%s_%d", node->token->value, id);
        } else if (node->type == AST_VAR_DECL) {
            var_counter++;
            int is_string = node->content != NULL;
            add_var(node->token->value, is_string);
            if (is_string) {
                fprintf(output_file, "char %s_%d[256] = \"%s\";\n", node->token->value, var_counter, node->content);
            } else {
                fprintf(output_file, "int %s_%d = %d;\n", node->token->value, var_counter, node->value);
            }
        } else if (node->type == AST_PRINT) {
            if (node->token->type == TOKEN_STRING) {
                fprintf(output_file, "printf(\"%s\\n\");\n", node->token->value);
            } else {
                int id = get_var_id(node->token->value);
                if (id == -1) {
                    fprintf(stderr, "Error: Variable not found for %s\n", node->token->value);
                    exit(1);
                }
                if (is_var_string(node->token->value)) {
                    fprintf(output_file, "printf(\"%%s\\n\", %s_%d);\n", node->token->value, id);
                } else {
                    fprintf(output_file, "printf(\"%%d\\n\", %s_%d);\n", node->token->value, id);
                }
            }
        } else if (node->type == AST_FILE_OPEN) {
            char sanitized[256];
            sanitize_filename(node->token->value, sanitized);
            var_counter++;
            add_var(sanitized, 0);
            fprintf(output_file, "FILE *file_%s_%d = fopen(\"%s\", \"w+\");\n", sanitized, var_counter, node->token->value);
            fprintf(output_file, "if (!file_%s_%d) { printf(\"Error opening file %s\\n\"); return 1; }\n", sanitized, var_counter, node->token->value);
        } else if (node->type == AST_FILE_WRITE) {
            char sanitized[256];
            sanitize_filename(node->token->value, sanitized);
            int id = get_var_id(sanitized);
            if (id == -1) {
                fprintf(stderr, "Error: File variable not found for %s\n", node->token->value);
                exit(1);
            }
            int var_id = get_var_id(node->content);
            if (var_id == -1) {
                fprintf(stderr, "Error: Variable not found for %s\n", node->content);
                exit(1);
            }
            if (is_var_string(node->content)) {
                fprintf(output_file, "fprintf(file_%s_%d, \"%%s\\n\", %s_%d);\n", sanitized, id, node->content, var_id);
            } else {
                fprintf(output_file, "fprintf(file_%s_%d, \"%%d\\n\", %s_%d);\n", sanitized, id, node->content, var_id);
            }
            fprintf(output_file, "fflush(file_%s_%d);\n", sanitized, id);
        } else if (node->type == AST_FILE_READ) {
            char sanitized[256];
            sanitize_filename(node->token->value, sanitized);
            int id = get_var_id(sanitized);
            if (id == -1) {
                fprintf(stderr, "Error: File variable not found for %s\n", node->token->value);
                exit(1);
            }
            fprintf(output_file, "char buffer_%s_%d[256];\n", sanitized, id);
            fprintf(output_file, "file_%s_%d = freopen(\"%s\", \"r\", file_%s_%d);\n", sanitized, id, node->token->value, sanitized, id);
            fprintf(output_file, "if (!file_%s_%d) { printf(\"Error reopening file %s for reading\\n\"); return 1; }\n", sanitized, id, node->token->value);
            fprintf(output_file, "if (fgets(buffer_%s_%d, sizeof(buffer_%s_%d), file_%s_%d)) {\n", sanitized, id, sanitized, id, sanitized, id);
            fprintf(output_file, "    printf(\"%%s\\n\", buffer_%s_%d);\n", sanitized, id);
            fprintf(output_file, "} else {\n");
            fprintf(output_file, "    printf(\"Error reading file %s\\n\");\n", node->token->value);
            fprintf(output_file, "}\n");
        } else if (node->type == AST_FILE_CLOSE) {
            char sanitized[256];
            sanitize_filename(node->token->value, sanitized);
            int id = get_var_id(sanitized);
            if (id == -1) {
                fprintf(stderr, "Error: File variable not found for %s\n", node->token->value);
                exit(1);
            }
            fprintf(output_file, "fclose(file_%s_%d);\n", sanitized, id);
        }
        node = node->right;
    }
}
