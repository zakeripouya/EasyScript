#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "parser.h"

Parser *create_parser(Lexer *lexer) {
    Parser *parser = (Parser *)malloc(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = get_next_token(lexer);
    return parser;
}

void eat(Parser *parser, TokenType type) {
    if (parser->current_token->type == type) {
        parser->current_token = get_next_token(parser->lexer);
    } else {
        fprintf(stderr, "Error: Unexpected token %s\n", parser->current_token->value);
        exit(1);
    }
}

AST *factor(Parser *parser) {
    Token *token = parser->current_token;
    if (token->type == TOKEN_INT) {
        eat(parser, TOKEN_INT);
        return create_ast(AST_INT, token);
    } else if (token->type == TOKEN_ID) {
        eat(parser, TOKEN_ID);
        return create_ast(AST_VAR, token);
    } else {
        fprintf(stderr, "Error: Unexpected token %s\n", token->value);
        exit(1);
    }
    return NULL;
}

AST *term(Parser *parser) {
    AST *node = factor(parser);
    return node;
}

AST *statement(Parser *parser) {
    Token *token = parser->current_token;
    AST *node;
    if (token->type == TOKEN_MAKE) {
        eat(parser, TOKEN_MAKE);
        eat(parser, TOKEN_A);
        eat(parser, TOKEN_VARIABLE);
        Token *var_name = parser->current_token;
        eat(parser, TOKEN_ID);
        eat(parser, TOKEN_ASSIGN);
        if (parser->current_token->type == TOKEN_STRING) {
            Token *str_token = parser->current_token;
            eat(parser, TOKEN_STRING);
            node = create_ast(AST_VAR_DECL, var_name);
            node->content = strdup(str_token->value);
        } else {
            AST *var_value = term(parser);
            node = create_ast(AST_VAR_DECL, var_name);
            node->value = atoi(var_value->token->value); // Correctly assign the value
        }
    } else if (token->type == TOKEN_PRINT) {
        eat(parser, TOKEN_PRINT);
        eat(parser, TOKEN_HASH);
        node = create_ast(AST_PRINT, parser->current_token);
        if (parser->current_token->type == TOKEN_STRING) {
            node->token = parser->current_token;
            eat(parser, TOKEN_STRING);
        } else {
            node->value = atoi(parser->current_token->value);
            eat(parser, TOKEN_ID);
        }
    } else if (token->type == TOKEN_FILE) {
        eat(parser, TOKEN_FILE);
        Token *action = parser->current_token;
        if (strcmp(action->value, "OPEN") == 0) {
            eat(parser, TOKEN_OPEN);
            node = create_ast(AST_FILE_OPEN, parser->current_token);
            eat(parser, TOKEN_ID);
        } else if (strcmp(action->value, "READ") == 0) {
            eat(parser, TOKEN_READ);
            node = create_ast(AST_FILE_READ, parser->current_token);
            eat(parser, TOKEN_ID);
        } else if (strcmp(action->value, "WRITE") == 0) {
            eat(parser, TOKEN_WRITE);
            Token *filename = parser->current_token;
            eat(parser, TOKEN_ID);
            Token *content = parser->current_token;
            if (content->type == TOKEN_STRING) {
                eat(parser, TOKEN_STRING);
            } else {
                eat(parser, TOKEN_ID);
            }
            node = create_ast(AST_FILE_WRITE, filename);
            node->content = strdup(content->value);
        } else if (strcmp(action->value, "CLOSE") == 0) {
            eat(parser, TOKEN_CLOSE);
            node = create_ast(AST_FILE_CLOSE, parser->current_token);
            eat(parser, TOKEN_ID);
        } else {
            fprintf(stderr, "Error: Unknown FILE action %s\n", action->value);
            exit(1);
        }
    } else {
        node = term(parser);
    }
    return node;
}

AST *program(Parser *parser) {
    AST *node = statement(parser);
    AST *root = node;
    while (parser->current_token->type != TOKEN_EOF) {
        node->right = statement(parser);
        node = node->right;
    }
    return root;
}

AST *create_ast(ASTType type, Token *token) {
    AST *node = (AST *)malloc(sizeof(AST));
    node->type = type;
    node->token = token;
    node->left = NULL;
    node->right = NULL;
    node->value = 0;
    node->content = NULL;
    return node;
}
