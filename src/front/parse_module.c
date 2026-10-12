// Sentences about files that use other files (modules):
//
//   use helpers.                    (also "use the helpers file.")
//   privately to tidy with text:    (only this file can use it)
//   privately keep rate at 0.2.
//   helpers's double with 3         (a definition from one file, by name)
//
// The files themselves are found and read before parsing (src/modules.c);
// here "use" only becomes a statement, and the names it gives are known so
// "helpers's double" isn't read as a field of something called helpers.

#include <string.h>
#include "front/parse_internal.h"

static bool word_is(const Parser *p, size_t i, const char *word) {
    return i < p->count && p->tokens[i].kind == TOK_WORD && strcmp(p->tokens[i].text, word) == 0;
}

static bool sentence_start(const Parser *p, size_t i) {
    if (i == 0) return true;
    TokenKind before = p->tokens[i - 1].kind;
    return before == TOK_NEWLINE || before == TOK_INDENT || before == TOK_DEDENT || before == TOK_PERIOD;
}

// The words of "use NAME [file]" from tokens[i + 1]: how many name the file.
static size_t use_words(const Parser *p, size_t i) {
    size_t n = 0;
    while (i + 1 + n < p->count && p->tokens[i + 1 + n].kind == TOK_WORD) n++;
    if (i + 1 + n < p->count) {
        TokenKind end = p->tokens[i + 1 + n].kind;
        if (end != TOK_PERIOD && end != TOK_NEWLINE && end != TOK_EOF) return 0;
    }
    if (n > 1 && word_is(p, i + n, "file")) n--;  // "use the helpers file"
    return n;
}

void parser_find_modules(Parser *p) {
    for (size_t i = 0; i < p->count; i++) {
        if (!word_is(p, i, "use") || !sentence_start(p, i) || word_is(p, i + 1, "same")) continue;
        size_t n = use_words(p, i);
        if (n == 1) vec_push(p->arena, &p->modules, p->tokens[i + 1].text);
    }
}

bool parser_after_privately(const Parser *p, size_t i) {
    if (i > 0 && p->tokens[i - 1].kind == TOK_COMMA) i--;
    return i > 0 && word_is(p, i - 1, "privately") && sentence_start(p, i - 1);
}

bool parser_at_module(const Parser *p, size_t ahead) {
    const Token *word = parser_peek(p, ahead);
    if (word->kind != TOK_WORD || !parser_at(p, ahead + 1, TOK_POSSESSIVE)) return false;
    for (size_t i = 0; i < p->modules.len; i++) {
        if (strcmp(p->modules.items[i], word->text) == 0) return true;
    }
    return false;
}

// After "helpers's": the name of something it defines has to follow.
static bool expect_defined_name(Parser *p, const char *module) {
    if (parser_is_name_word(parser_peek(p, 0))) return true;
    if (parser_error(p, p->prev_span, "Something is missing after \"%s's\".", module)) {
        diag_note(p->diag, "Write the name of something %s.es defines, like \"%s's double with 3\".", module, module);
    }
    return false;
}

Expr *parser_parse_module_value(Parser *p, Expr *(*parse_name)(Parser *p)) {
    const Token *module = parser_peek(p, 0);
    parser_consume(p, 2);
    if (!expect_defined_name(p, module->text)) return parser_error_expr(p);
    Expr *expr = parse_name(p);
    if (expr->kind == EXPR_NAME || expr->kind == EXPR_CALL) {
        expr->module = module->text;
        expr->pos = ast_pos_join(parser_token_pos(module), expr->pos);
    }
    return expr;
}

Stmt *parser_parse_module_call(Parser *p) {
    const Token *module = parser_peek(p, 0);
    parser_consume(p, 2);
    if (!expect_defined_name(p, module->text)) return NULL;
    const KnownName *function = parser_match_name(p, 0, true);
    if (!function && (parser_at_word(p, 1, "with") || parser_at_word(p, 1, "of") || parser_at_word(p, 1, "using") ||
                      parser_at(p, 1, TOK_PERIOD) || parser_at(p, 1, TOK_NEWLINE) || parser_at(p, 1, TOK_EOF))) {
        // a one-word function of a file this one doesn't see (only "easyscript ast" reads one file)
        KnownName *one = arena_alloc(p->arena, sizeof(KnownName));
        *one = (KnownName){parser_peek(p, 0)->text, 1, true, 0, false, NULL, 0};
        function = one;
    }
    if (!function) {
        const Token *word = parser_peek(p, 0);
        if (parser_error(p, word->span, "I don't know a sentence that starts with \"%s's %s\".", module->text,
                         word->text)) {
            diag_note(p->diag, "A sentence can use a name from another file, like \"%s's greet with \"Ann\"\".",
                      module->text);
        }
        return NULL;
    }
    Stmt *stmt = parse_call_statement(p, module, function);
    if (stmt && stmt->as.call->kind == EXPR_CALL) {
        stmt->as.call->module = module->text;
        stmt->as.call->pos = ast_pos_join(parser_token_pos(module), stmt->as.call->pos);
    }
    return stmt;
}

Stmt *parser_parse_use(Parser *p, const Token *verb) {
    size_t at = p->pos - 1;
    size_t n = use_words(p, at);
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, verb->span, "Something is missing after \"use\".")) {
            diag_note(p->diag, "Write the name of the file to use, without \".es\", like \"use helpers.\".");
        }
        return NULL;
    }
    if (n != 1) {
        bool word = parser_peek(p, 0)->kind == TOK_WORD;
        size_t words = 0;
        while (parser_peek(p, words)->kind == TOK_WORD) words++;
        const Token *token = parser_peek(p, word ? 1 : 0);
        if (!word || words == 1) token = parser_peek(p, words);
        bool reported = words > 1 ? parser_error(p, token->span, "The name of a file to use is one word.")
                                  : parser_error(p, token->span, "I expected the sentence to end after the name of "
                                                                 "the file, but found %s.", parser_describe(p, token));
        if (reported) diag_note(p->diag, "Use one file per sentence, named with one word, like \"use shapes.\".");
        return NULL;
    }
    Stmt *stmt = parser_new_stmt(p, STMT_USE, verb);
    parser_take_name(p, 1, &stmt->as.module);
    if (parser_at_word(p, 0, "file")) parser_advance(p);
    return stmt;
}

Stmt *parser_parse_privately(Parser *p, const Token *verb) {
    if (parser_at(p, 0, TOK_COMMA)) parser_advance(p);
    const Token *next = parser_peek(p, 0);
    Stmt *stmt = NULL;
    if (parser_at_word(p, 0, "to")) {
        parser_advance(p);
        stmt = parse_function(p, next);
        if (stmt) stmt->as.function.private = true;
    } else if (parser_at_word(p, 0, "keep") && !parser_at_word(p, 1, "doing") && !parser_at_keep_only(p)) {
        parser_advance(p);
        stmt = parse_constant(p, next);
        if (stmt) stmt->as.constant.private = true;
    } else {
        bool reported = parser_at_sentence_end(p)
                            ? parser_error(p, verb->span, "Something is missing after \"privately\".")
                            : parser_error(p, next->span, "Only definitions can be kept private, but this is %s.",
                                           parser_describe(p, next));
        if (reported) {
            diag_note(p->diag, "Write \"privately to tidy with text:\" or \"privately keep rate at 0.2.\".");
        }
        return NULL;
    }
    if (stmt) stmt->pos = ast_pos_join(parser_token_pos(verb), stmt->pos);
    return stmt;
}
