#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "lexer.h"

Lexer *create_lexer(char *source) {
    Lexer *lexer = (Lexer *)malloc(sizeof(Lexer));
    lexer->source = source;
    lexer->position = 0;
    lexer->current_char = source[lexer->position];
    return lexer;
}

void advance(Lexer *lexer) {
    lexer->position++;
    if (lexer->position > strlen(lexer->source) - 1) {
        lexer->current_char = '\0';
    } else {
        lexer->current_char = lexer->source[lexer->position];
    }
}

void skip_whitespace(Lexer *lexer) {
    while (lexer->current_char != '\0' && isspace(lexer->current_char)) {
        advance(lexer);
    }
}

Token *create_token(TokenType type, char *value) {
    Token *token = (Token *)malloc(sizeof(Token));
    token->type = type;
    token->value = strdup(value);
    return token;
}

char *get_id(Lexer *lexer) {
    char *result = (char *)malloc(256);
    int i = 0;
    while (lexer->current_char != '\0' && (isalnum(lexer->current_char) || lexer->current_char == '.')) {
        result[i++] = lexer->current_char;
        advance(lexer);
    }
    result[i] = '\0';
    return result;
}

Token *get_next_token(Lexer *lexer) {
    while (lexer->current_char != '\0') {
        if (isspace(lexer->current_char)) {
            skip_whitespace(lexer);
            continue;
        }

        if (isalpha(lexer->current_char)) {
            char *id = get_id(lexer);
            if (strcmp(id, "MAKE") == 0) return create_token(TOKEN_MAKE, id);
            if (strcmp(id, "A") == 0) return create_token(TOKEN_A, id);
            if (strcmp(id, "VARIABLE") == 0) return create_token(TOKEN_VARIABLE, id);
            if (strcmp(id, "ASSIGN") == 0) return create_token(TOKEN_ASSIGN, id);
            if (strcmp(id, "PRINT") == 0) return create_token(TOKEN_PRINT, id);
            if (strcmp(id, "FILE") == 0) return create_token(TOKEN_FILE, id);
            if (strcmp(id, "OPEN") == 0) return create_token(TOKEN_OPEN, id);
            if (strcmp(id, "READ") == 0) return create_token(TOKEN_READ, id);
            if (strcmp(id, "WRITE") == 0) return create_token(TOKEN_WRITE, id);
            if (strcmp(id, "CLOSE") == 0) return create_token(TOKEN_CLOSE, id);
            return create_token(TOKEN_ID, id);
        }

        if (isdigit(lexer->current_char)) {
            char *result = (char *)malloc(256);
            int i = 0;
            while (lexer->current_char != '\0' && isdigit(lexer->current_char)) {
                result[i++] = lexer->current_char;
                advance(lexer);
            }
            result[i] = '\0';
            return create_token(TOKEN_INT, result);
        }

        if (lexer->current_char == '"') {
            advance(lexer);
            char *result = (char *)malloc(256);
            int i = 0;
            while (lexer->current_char != '\0' && lexer->current_char != '"') {
                result[i++] = lexer->current_char;
                advance(lexer);
            }
            result[i] = '\0';
            advance(lexer);  // Skip closing quote
            return create_token(TOKEN_STRING, result);
        }

        if (lexer->current_char == '#') {
            advance(lexer);
            return create_token(TOKEN_HASH, "#");
        }

        fprintf(stderr, "Error: Unknown character '%c' at position %d\n", lexer->current_char, lexer->position);
        exit(1);
    }

    return create_token(TOKEN_EOF, "");
}
