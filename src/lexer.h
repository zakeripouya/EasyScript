#ifndef LEXER_H
#define LEXER_H

typedef enum {
    TOKEN_INT,
    TOKEN_ID,
    TOKEN_MAKE,
    TOKEN_A,
    TOKEN_VARIABLE,
    TOKEN_ASSIGN,
    TOKEN_PRINT,
    TOKEN_FILE,
    TOKEN_OPEN,
    TOKEN_READ,
    TOKEN_WRITE,
    TOKEN_CLOSE,
    TOKEN_STRING,
    TOKEN_HASH,
    TOKEN_EOF
} TokenType;

typedef struct {
    TokenType type;
    char *value;
} Token;

typedef struct {
    char *source;
    int position;
    char current_char;
} Lexer;

Lexer *create_lexer(char *source);
void advance(Lexer *lexer);
void skip_whitespace(Lexer *lexer);
Token *create_token(TokenType type, char *value);
Token *get_next_token(Lexer *lexer);

#endif
