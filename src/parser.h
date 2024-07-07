#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"

typedef enum {
    AST_INT,
    AST_VAR,
    AST_VAR_DECL,
    AST_PRINT,
    AST_IF,
    AST_FOR_LOOP,
    AST_FUNCTION,
    AST_CALL,
    AST_FILE_OPEN,
    AST_FILE_READ,
    AST_FILE_WRITE,
    AST_FILE_CLOSE
} ASTType;

typedef struct AST {
    ASTType type;
    Token *token;
    struct AST *left;
    struct AST *right;
    int value;
    char *content;
    struct AST *condition;
    struct AST *true_branch;
    struct AST *false_branch;
    int start;
    int end;
    struct AST *body;
} AST;

typedef struct {
    Lexer *lexer;
    Token *current_token;
} Parser;

Parser *create_parser(Lexer *lexer);
void eat(Parser *parser, TokenType type);
AST *factor(Parser *parser);
AST *term(Parser *parser);
AST *statement(Parser *parser);
AST *program(Parser *parser);
AST *create_ast(ASTType type, Token *token);

#endif
