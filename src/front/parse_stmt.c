// Program structure. Until statements exist, a program is a list of
// expression sentences, each ended by a period or the end of its line.

#include "front/parse.h"
#include "front/parse_internal.h"

// Skips the rest of a sentence that had an error.
static void skip_sentence(Parser *p) {
    while (!parser_at_sentence_end(p)) parser_advance(p);
}

// After an expression: a period, the end of the line, or the end of the file.
// A period may be followed by another sentence on the same line.
static void end_sentence(Parser *p) {
    if (!p->sentence_failed && !parser_at_sentence_end(p)) parser_report_leftover(p);
    skip_sentence(p);
    if (parser_at(p, 0, TOK_PERIOD)) parser_advance(p);
    if (parser_at(p, 0, TOK_NEWLINE)) parser_advance(p);
    p->sentence_failed = false;
}

static void reject_indent(Parser *p) {
    const Token *indent = parser_advance(p);
    if (parser_error(p, indent->span, "This line is indented, but nothing above it starts a block.")) {
        diag_note(p->diag, "Remove the spaces at the start of the line.");
    }
    p->sentence_failed = false;
}

// A period where a sentence should start: "say 1.." or ".5".
static void reject_stray_period(Parser *p) {
    const Token *period = parser_advance(p);
    const Token *next = parser_peek(p, 0);
    bool decimal = next->kind == TOK_NUMBER && next->span.offset == period->span.offset + 1;
    if (parser_error(p, period->span, "There's a period here with no sentence before it.")) {
        if (decimal) {
            diag_note(p->diag, "Write decimals with a digit before the point, like 0.%s.", next->text);
        } else {
            diag_note(p->diag, "Remove the extra period.");
        }
    }
    if (decimal) skip_sentence(p);
    p->sentence_failed = false;
}

static Stmt *parse_expression_statement(Parser *p) {
    Expr *expr = parse_expression(p);
    Stmt *stmt = ast_new_stmt(p->arena, STMT_EXPR, expr->pos);
    stmt->as.expr = expr;
    return stmt;
}

Block *parse_program(Arena *arena, Diag *diag, const char *source, const TokenList *tokens) {
    Parser p = {0};
    p.arena = arena;
    p.diag = diag;
    p.source = source;
    p.tokens = tokens->items;
    p.count = tokens->len;

    Block *program = arena_alloc(arena, sizeof(Block));
    while (!parser_at(&p, 0, TOK_EOF)) {
        TokenKind kind = parser_peek(&p, 0)->kind;
        if (kind == TOK_INDENT) {
            reject_indent(&p);
        } else if (kind == TOK_PERIOD) {
            reject_stray_period(&p);
        } else if (kind == TOK_DEDENT || kind == TOK_NEWLINE) {
            parser_advance(&p);
        } else {
            vec_push(arena, program, parse_expression_statement(&p));
            end_sentence(&p);
        }
    }
    return program;
}
