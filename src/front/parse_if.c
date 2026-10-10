// if / otherwise if / otherwise ("else" means "otherwise").
//
//   if C:                     one-line forms (no "otherwise"):
//       statements                if C, S.
//   otherwise if C:               if C then S.
//       statements
//   otherwise:
//       statements
//
// An "otherwise" continues the if that starts in the same column, right
// above it. The lexer's INDENT/DEDENT tokens make that exact: after the
// if's block ends (DEDENT), an "otherwise" at the start of the next line is
// at the if's indentation.

#include <string.h>
#include "front/parse_internal.h"

bool parser_at_otherwise(const Parser *p) {
    return parser_at_word(p, 0, "otherwise") || parser_at_word(p, 0, "else");
}

static bool at_if_start_of_otherwise(const Parser *p) {
    return parser_at_otherwise(p) && parser_at_word(p, 1, "if");
}

// --- Branches ----------------------------------------------------------------

static Span header_span(const SourcePos *start, Span end) {
    Span span = {start->span.offset, end.offset + end.length - start->span.offset};
    return span;
}

// ":" NEWLINE INDENT statements DEDENT, after a condition or "otherwise".
// `if_pos` is the if this branch belongs to, for misplaced "otherwise"s inside.
static bool parse_branch_block(Parser *p, IfBranch *branch, const SourcePos *if_pos, const char *what) {
    if (!parser_at(p, 0, TOK_COLON)) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected a colon after %s, but found %s.", what, parser_describe(p, token))) {
            diag_note(p->diag, "Put a colon at the end of the line and the sentences that belong to it on the "
                               "indented lines below.");
        }
        parser_skip_line_and_block(p);
        return false;
    }
    Span colon = parser_advance(p)->span;
    branch->pos.span = header_span(&branch->pos, colon);
    if (!parser_at(p, 0, TOK_NEWLINE) && !parser_at(p, 0, TOK_EOF)) {
        if (parser_error(p, parser_peek(p, 0)->span, "After the colon, start a new line.")) {
            diag_note(p->diag, "Put the sentences that belong to it on the next lines, indented. "
                               "For one sentence you can also write \"if x is 5, say \"five\"\".");
        }
        parser_skip_line_and_block(p);
        return false;
    }
    if (parser_at(p, 0, TOK_NEWLINE)) parser_advance(p);
    if (!parser_at(p, 0, TOK_INDENT)) {
        if (parser_error(p, branch->pos.span, "Nothing is indented under %s.", what)) {
            diag_note(p->diag, "Put the sentences that belong to it on the next lines, indented "
                               "(4 spaces is usual).");
        }
        p->sentence_failed = false;
        return false;
    }
    parser_advance(p);
    vec_push(p->arena, &p->open_ifs, *if_pos);
    parse_statements(p, &branch->body, true);
    p->open_ifs.len--;
    return true;
}

static bool missing_condition(Parser *p, const char *after) {
    const Token *next = parser_peek(p, 0);
    bool missing = parser_at_sentence_end(p) || next->kind == TOK_COLON || next->kind == TOK_COMMA ||
                   parser_at_word(p, 0, "then");
    if (missing && parser_error(p, p->prev_span, "Something is missing after \"%s\".", after)) {
        diag_note(p->diag, "Add a condition that's yes or no, like \"if x is 5:\".");
    }
    return missing;
}

// "otherwise if C:" or "otherwise:" branches after a block if. Returns false
// if any had an error (they're still all consumed).
static bool parse_otherwise_chain(Parser *p, Stmt *stmt) {
    bool ok = true;
    size_t final_line = 0;  // line of a plain "otherwise", once seen
    while (parser_at_otherwise(p)) {
        const Token *keyword = parser_peek(p, 0);
        bool has_condition = at_if_start_of_otherwise(p);
        if (final_line) {
            if (parser_error(p, keyword->span, "This \"if\" already has an \"otherwise\", on line %zu.", final_line)) {
                diag_note(p->diag, "The plain \"otherwise\" has to come last. "
                                   "For more choices, use \"otherwise if\" before it.");
            }
            parser_skip_line_and_block(p);
            ok = false;
            continue;
        }
        parser_consume(p, has_condition ? 2 : 1);
        IfBranch branch = {0};
        branch.pos = parser_token_pos(keyword);
        if (has_condition) {
            if (missing_condition(p, "otherwise if")) {
                parser_skip_line_and_block(p);
                ok = false;
                continue;
            }
            branch.condition = parse_expression(p);
            if (p->sentence_failed) {
                parser_skip_line_and_block(p);
                ok = false;
                continue;
            }
        } else {
            final_line = keyword->line;
        }
        const SourcePos *if_pos = &stmt->pos;
        if (parse_branch_block(p, &branch, if_pos, has_condition ? "this \"otherwise if\"" : "this \"otherwise\"")) {
            vec_push(p->arena, &stmt->as.if_stmt.branches, branch);
        } else {
            ok = false;
        }
    }
    return ok;
}

// --- if ------------------------------------------------------------------------

// "if C, S" / "if C then S": one simple statement on the same line.
static bool parse_one_line_body(Parser *p, IfBranch *branch) {
    Span separator = parser_advance(p)->span;  // "," or "then"
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, separator, "Something is missing after %s.", parser_quoted(p, separator))) {
            diag_note(p->diag, "Add the sentence to do, like \"if x is 5, say \"five\"\".");
        }
        return false;
    }
    if (parser_at_word(p, 0, "if") || parser_at_otherwise(p)) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "A one-line \"if\" can only hold one simple sentence.")) {
            diag_note(p->diag, "Write the outer \"if\" with a colon and put the inner one on an indented line below it.");
        }
        return false;
    }
    Stmt *body = parse_statement(p);
    if (!body || p->sentence_failed) return false;
    vec_push(p->arena, &branch->body, body);
    return true;
}

Stmt *parse_if(Parser *p, const Token *verb) {
    Stmt *stmt = ast_new_stmt(p->arena, STMT_IF, parser_token_pos(verb));
    stmt->verb = verb->span;
    IfBranch first = {0};
    first.pos = stmt->pos;

    if (missing_condition(p, "if")) {
        parser_skip_line_and_block(p);
        parse_otherwise_chain(p, stmt);
        p->ended_with_block = true;  // set last: statements inside the blocks reset it
        return NULL;
    }
    first.condition = parse_expression(p);
    if (!p->sentence_failed && (parser_at(p, 0, TOK_COMMA) || parser_at_word(p, 0, "then"))) {
        stmt->as.if_stmt.one_line = true;
        if (!parse_one_line_body(p, &first)) return NULL;
        vec_push(p->arena, &stmt->as.if_stmt.branches, first);
        return stmt;
    }

    bool ok = !p->sentence_failed;
    if (!ok) {
        parser_skip_line_and_block(p);
    } else if (!parser_at(p, 0, TOK_COLON)) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected a colon, a comma, or \"then\" after the condition, but found %s.",
                         parser_describe(p, token))) {
            diag_note(p->diag, "Write \"if x is 5:\" with the sentences on indented lines below, "
                               "or \"if x is 5, say \"five\"\" on one line.");
        }
        parser_skip_line_and_block(p);
        ok = false;
    } else {
        ok = parse_branch_block(p, &first, &stmt->pos, "this \"if\"");
    }
    if (ok) vec_push(p->arena, &stmt->as.if_stmt.branches, first);
    ok = parse_otherwise_chain(p, stmt) && ok;
    p->ended_with_block = true;  // set last: statements inside the blocks reset it
    return ok ? stmt : NULL;
}

// --- A stray "otherwise" -------------------------------------------------------

void parse_orphan_otherwise(Parser *p, const Stmt *previous) {
    const Token *keyword = parser_peek(p, 0);
    const char *word = keyword->text;
    if (previous && previous->kind == STMT_IF && previous->as.if_stmt.one_line) {
        if (parser_error(p, keyword->span, "A one-line \"if\" can't have an \"%s\".", word)) {
            diag_note(p->diag, "Write the \"if\" with a colon and its sentences on indented lines below it; "
                               "then the \"%s\" can follow.", word);
        }
    } else if (p->open_ifs.len > 0) {
        SourcePos if_pos = vec_last(&p->open_ifs);
        if (parser_error(p, keyword->span, "This \"%s\" doesn't line up with its \"if\".", word)) {
            diag_note(p->diag, "An \"%s\" has to start in the same column as its \"if\". "
                               "The \"if\" on line %zu starts in column %zu.",
                      word, if_pos.line, if_pos.column);
        }
    } else if (parser_error(p, keyword->span, "There's no \"if\" before this \"%s\".", word)) {
        diag_note(p->diag, "An \"%s\" continues the \"if\" that starts in the same column, just above it.", word);
    }
    parser_skip_line_and_block(p);
}
