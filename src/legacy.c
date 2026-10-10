#include "legacy.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"

void legacy_compile(char *source, FILE *out) {
    Lexer *lexer = create_lexer(source);
    Parser *parser = create_parser(lexer);
    AST *root = program(parser);
    generate_code(root, out);
}
