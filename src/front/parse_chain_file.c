// The file a step names, in steps in a row:
//
//   make a new file called "groceries.txt", then write "milk" in it,
//   then add "eggs" and "bread" to it, one per line, and then show me what's in the file.
//
// Right after a step that made or wrote to a file (Parser.chain_file), "it"
// and "the file" with no name after it mean that file: the step before's
// result, like any "it" in steps in a row (parser_chain_it). Elsewhere file
// sentences still say "file F" (a known exception), and "the file" alone is
// an error.

#include <string.h>
#include "front/parse_internal.h"

static bool ends_place(const Parser *p, size_t ahead) {  // nothing more about the file follows
    const Token *token = parser_peek(p, ahead);
    TokenKind kind = token->kind;
    return kind == TOK_PERIOD || kind == TOK_NEWLINE || kind == TOK_EOF || kind == TOK_DEDENT || kind == TOK_INDENT ||
           kind == TOK_COMMA || parser_at_word(p, ahead, "and") || parser_at_word(p, ahead, "then");
}

bool parser_at_chain_it_file(const Parser *p, size_t ahead) {
    return p->chain_file && parser_at_word(p, ahead, "it") && ends_place(p, ahead + 1);
}

bool parser_at_bare_file(const Parser *p, size_t ahead) {
    return parser_at_word(p, ahead, "file") && ends_place(p, ahead + 1);
}

// At "it" or "file": the file the step before made or wrote to.
Expr *parser_chain_file(Parser *p) {
    const Token *token = parser_advance(p);
    if (!p->chain_file || !p->chain_it || !*p->chain_it) {
        if (parser_error(p, token->span, "I don't know which file \"%s\" means here.",
                         strcmp(token->text, "it") == 0 ? "it" : "the file")) {
            diag_note(p->diag, "Say its name, like \"in file \"notes.txt\"\". The file alone works only right after a "
                               "step that makes a file or writes to one.");
            diag_info(p->diag, DIAG_MISSING, "the file's name", "text");
        }
        return parser_error_expr(p);
    }
    return parser_chain_it(p, ast_new_expr(p->arena, EXPR_IT, parser_token_pos(token)));
}

// After "in" or "to" in a file sentence: "it", "file" alone, or NULL (read nothing) for "file F".
Expr *parser_parse_chain_place(Parser *p) {
    if (parser_at_chain_it_file(p, 0) || parser_at_bare_file(p, 0)) return parser_chain_file(p);
    return NULL;
}
