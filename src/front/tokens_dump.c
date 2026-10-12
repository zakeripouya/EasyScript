// Printing tokens for `easyscript tokens`: one per line, as
// "KIND line:col text [after "the"]".

#include <stdio.h>
#include <string.h>
#include "front/lexer.h"

const char *token_kind_name(TokenKind kind) {
    static const char *const names[] = {
        "EOF", "NEWLINE", "INDENT", "DEDENT", "WORD", "NUMBER", "STRING", "ORDINAL",
        "PERIOD", "COMMA", "COLON", "LPAREN", "RPAREN", "PLUS", "MINUS",
        "STAR", "SLASH", "PERCENT", "EQUAL", "LESS", "GREATER",
        "LESS_EQUAL", "GREATER_EQUAL", "NOT_EQUAL", "EQUAL_EQUAL", "LBRACKET", "RBRACKET", "POSSESSIVE",
    };
    return names[kind];
}

#define KIND_WIDTH 13      // strlen("GREATER_EQUAL")
#define POSITION_WIDTH 8

static void dump_token(const Token *token, const char *source, StrBuf *out) {
    const char *name = token_kind_name(token->kind);
    sb_append(out, name);
    sb_append_repeat(out, ' ', KIND_WIDTH + 1 - strlen(name));
    int pos_len = snprintf(NULL, 0, "%zu:%zu", token->line, token->column);
    sb_appendf(out, "%zu:%zu", token->line, token->column);
    if (token->kind == TOK_STRING || token->text_len > 0 || token->filler.length > 0) {
        sb_append_repeat(out, ' ', pos_len < POSITION_WIDTH ? (size_t)(POSITION_WIDTH - pos_len) : 1);
        if (token->kind == TOK_STRING) {
            sb_append_quoted(out, token->text, token->text_len);
        } else {
            sb_append_n(out, token->text, token->text_len);
        }
    }
    if (token->filler.length > 0) {
        if (token->kind == TOK_STRING || token->text_len > 0) sb_append(out, "  ");
        sb_appendf(out, "[after \"%.*s\"]", (int)token->filler.length, source + token->filler.offset);
    }
    sb_append_char(out, '\n');
}

void tokens_dump(const TokenList *tokens, const char *source, StrBuf *out) {
    for (size_t i = 0; i < tokens->len; i++) {
        dump_token(&tokens->items[i], source, out);
    }
}
