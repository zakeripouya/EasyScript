// Steps in a row: sentences joined by ", then", ", and then", "and then" or
// "and" (before another sentence), as in "read the table in "sales.csv", then
// total the amount per month, then sort it biggest first".
//
// Each step that gives something (the name a "let" makes, the list a "sort"
// changes, the file a "write" writes to, the value of "total the amount per
// month") keeps it in a hidden name, and "it" in the next step means that. A
// step's "it" inside a filter or loop could mean either, so that's an error
// (checker). Steps also start with "first", "next", "after that", "finally"
// and "then", which only say the order.
//
// Action forms: "total the amount [of L] [per G]", "average the score ...",
// "find the median ...", "count the big orders": the question about L (or, in
// a chain, about "it"), kept in a name: "... and call it totals", "..., called
// totals", or a hidden one for the next step.

#include <string.h>
#include "front/parse_internal.h"

// --- Joining steps --------------------------------------------------------------------

static const char *const links[] = {"then", "and then", "after that", "next", "finally"};

// A comma and a word that starts the next step (", then"): it ends lists and the pieces of "say".
bool parser_at_step_link(const Parser *p) {
    if (!parser_at(p, 0, TOK_COMMA)) return false;
    for (size_t i = 0; i < sizeof links / sizeof links[0]; i++) {
        if (parser_match_words_at(p, 1, links[i])) return true;
    }
    return false;
}

// The words that join this step to the next, and how many tokens they are (0: none).
static size_t link_words(const Parser *p) {
    if (parser_at_next_action(p)) return parser_at_word(p, 1, "then") ? 2 : 1;
    if (!parser_at(p, 0, TOK_COMMA)) return 0;
    for (size_t i = 0; i < sizeof links / sizeof links[0]; i++) {
        size_t n = parser_match_words_at(p, 1, links[i]);
        if (n) return 1 + n + (parser_at(p, 1 + n, TOK_COMMA) ? 1 : 0);
    }
    return 0;
}

// A copy of a simple value, to keep a step's result in a name.
static Expr *copy_simple(Parser *p, const Expr *expr) {
    bool step_it = expr && expr->kind == EXPR_IT && expr->display && *expr->display;  // "sort it": still it
    if (!expr || !(expr->kind == EXPR_NAME || expr->kind == EXPR_TEXT || expr->kind == EXPR_NUMBER || step_it)) return NULL;
    Expr *copy = ast_new_expr(p->arena, expr->kind, expr->pos);
    copy->as = expr->as;
    copy->display = expr->display;
    copy->module = expr->module;
    return copy;
}

static Expr *name_expr(Parser *p, const Name *name) {
    Expr *expr = ast_new_expr(p->arena, EXPR_NAME, name->pos);
    expr->as.name = name->text;
    expr->display = name->display;
    return expr;
}

// What a step gives the next one as "it", or NULL.
static Expr *step_result(Parser *p, const Stmt *stmt) {
    switch (stmt->kind) {
    case STMT_LET:
    case STMT_SET: return stmt->as.assign.field ? NULL : name_expr(p, &stmt->as.assign.name);
    case STMT_CHANGE: return stmt->as.change.field ? NULL : name_expr(p, &stmt->as.change.target);
    case STMT_ASK: return name_expr(p, &stmt->as.ask.answer);
    case STMT_READ_FILE: return name_expr(p, &stmt->as.read_file.name);
    case STMT_LIST: return copy_simple(p, stmt->as.list.list);
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE: return copy_simple(p, stmt->as.file_write.path);
    case STMT_LIBRARY:
        switch (stmt->as.library.op) {
        case DO_READ_CSV:
        case DO_READ_TABLE: return stmt->as.library.name.text ? name_expr(p, &stmt->as.library.name) : NULL;
        case DO_SAVE_CSV:
        case DO_SHUFFLE:
        case DO_MAKE_FILE:
        case DO_MAKE_FOLDER: return copy_simple(p, stmt->as.library.a);
        case DO_SAVE_CHART: return copy_simple(p, stmt->as.library.b);  // the chart's file
        default: return NULL;
        }
    default: return NULL;
    }
}

Stmt *parser_parse_actions(Parser *p, Block *block) {
    const char *outer = p->chain_it;
    p->chain_it = NULL;  // the first step's "it" is the usual one
    Stmt *stmt = parse_statement(p);
    if (stmt && !p->sentence_failed) vec_push(p->arena, block, stmt);
    size_t n;
    while (stmt && !p->sentence_failed && !p->ended_with_block && (n = link_words(p)) > 0) {
        parser_consume(p, n);
        Expr *result = step_result(p, stmt);
        p->chain_file = stmt->kind == STMT_WRITE_FILE || stmt->kind == STMT_APPEND_FILE ||
                        (stmt->kind == STMT_LIBRARY &&
                         (stmt->as.library.op == DO_MAKE_FILE || stmt->as.library.op == DO_SAVE_CHART));
        Stmt *keep = NULL;
        if (result && result->kind == EXPR_NAME && strncmp(result->as.name, "0step", 5) == 0) {
            p->chain_it = result->as.name;  // already kept in a hidden name
            p->chain_used = false;
            stmt = parse_statement(p);
            if (stmt && !p->sentence_failed) vec_push(p->arena, block, stmt);
            continue;
        }
        if (result) {  // kept for the next step's "it", if it says "it"
            keep = ast_new_stmt(p->arena, STMT_LET, stmt->pos);
            keep->verb = stmt->verb;
            keep->as.assign.name.text = arena_sprintf(p->arena, "0step%zu", ++p->hidden_lists);
            keep->as.assign.name.display = "it";
            keep->as.assign.name.pos = stmt->pos;
            keep->as.assign.value = result;
        }
        p->chain_it = keep ? keep->as.assign.name.text : "";  // nothing: "it" there is an error
        p->chain_used = false;
        size_t at = block->len;
        stmt = parse_statement(p);
        if (stmt && !p->sentence_failed) vec_push(p->arena, block, stmt);
        if (keep && p->chain_used && !p->sentence_failed) {  // the step said "it": keep the result just before it
            vec_push(p->arena, block, keep);
            for (size_t i = block->len - 1; i > at; i--) block->items[i] = block->items[i - 1];
            block->items[at] = keep;
        }
    }
    p->chain_it = outer;
    p->chain_file = false;
    return stmt;
}

// "first", "next", "after that", "finally", "then" before a sentence: they
// only say the order. Not before a name the program knows ("next word in r").
void parser_skip_step_words(Parser *p) {
    static const char *const words[] = {"first", "next", "after that", "finally", "then"};
    const KnownName *known = parser_match_name(p, 0, false);
    if (known && known->words > 1) return;
    for (size_t i = 0; i < sizeof words / sizeof words[0]; i++) {
        size_t n = parser_match_words(p, words[i]);
        if (!n || parser_is_function(p, words[i])) continue;
        const Token *after = parser_peek(p, n + (parser_at(p, n, TOK_COMMA) ? 1 : 0));
        static const char *const polite[] = {"can", "could", "would", "i", "i'd"};  // "then can you ..."
        bool asking = false;
        for (size_t k = 0; after->kind == TOK_WORD && k < sizeof polite / sizeof polite[0]; k++) {
            asking = asking || strcmp(after->text, polite[k]) == 0;
        }
        if (after->kind != TOK_WORD || (!parser_is_statement_word(after->text) && !asking)) continue;
        parser_consume(p, n + (parser_at(p, n, TOK_COMMA) ? 1 : 0));
        return;
    }
}

// "it" in a step: the step before's result (a hidden name), kept in the
// node's display for the checker to decide (EXPR_IT).
Expr *parser_chain_it(Parser *p, Expr *it) {
    if (p->chain_it) {
        it->display = p->chain_it;
        p->chain_used = true;
    }
    return it;
}

// --- Action forms ---------------------------------------------------------------------

static const struct {
    const char *word;
    AggregateKind kind;
} actions[] = {{"total", AGG_TOTAL}, {"average", AGG_AVERAGE}, {"median", AGG_MEDIAN}};

static int action_at(const Parser *p) {
    const Token *word = parser_peek(p, 0);
    const KnownName *known = parser_match_name(p, 0, false);
    if (word->kind != TOK_WORD || parser_is_function(p, word->text) || (known && known->words > 1)) return -1;
    for (size_t i = 0; i < sizeof actions / sizeof actions[0]; i++) {
        if (strcmp(word->text, actions[i].word) == 0 && parser_field_words(p, 1) > 0) return (int)i;
    }
    static const char *const loop_words[] = {"from", "down", "to", "by", "in", "with", "of", "at", "up"};
    bool loop = false;
    for (size_t i = 0; i < sizeof loop_words / sizeof loop_words[0]; i++) loop |= parser_at_word(p, 1, loop_words[i]);
    bool count = strcmp(word->text, "count") == 0 && !loop &&
                 (parser_is_name_word(parser_peek(p, 1)) || parser_at(p, 1, TOK_LPAREN) || parser_at_word(p, 1, "it"));
    return count ? (int)(sizeof actions / sizeof actions[0]) : -1;
}

bool parser_at_action(const Parser *p) {
    return action_at(p) >= 0;
}

// "and call it NAME", "and call the result NAME", ", called NAME", "called NAME", "named NAME".
static bool parse_called(Parser *p, Name *name) {
    size_t n = parser_match_words(p, "and call it");
    if (!n) n = parser_match_words(p, "and call result");
    if (!n) n = parser_match_words(p, "called") ? 1 : parser_match_words(p, "named") ? 1 : 0;
    if (!n && parser_at(p, 0, TOK_COMMA) && (parser_at_word(p, 1, "called") || parser_at_word(p, 1, "named"))) n = 2;
    if (!n) return false;
    parser_consume(p, n);
    return parser_parse_name(p, name, NULL);
}

Stmt *parser_parse_action(Parser *p) {
    int which = action_at(p);
    const Token *verb = parser_advance(p);
    Stmt *stmt = parser_new_stmt(p, STMT_LET, verb);
    Expr *value;
    if (which == (int)(sizeof actions / sizeof actions[0])) {  // count the big orders
        Expr *list = parser_parse_filters(p, parse_primary(p));
        value = ast_new_expr(p->arena, EXPR_AGGREGATE, ast_pos_join(parser_token_pos(verb), list->pos));
        value->as.aggregate.kind = AGG_COUNT;
        value->as.aggregate.op_span = verb->span;
        value->as.aggregate.list = list;
    } else {  // total the amount [of L] [per G]
        value = ast_new_expr(p->arena, EXPR_AGGREGATE, parser_token_pos(verb));
        value->as.aggregate.kind = actions[which].kind;
        value->as.aggregate.op_span = verb->span;
        Expr *it = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(parser_peek(p, 0)));
        Expr *field = ast_new_expr(p->arena, EXPR_FIELD, it->pos);
        field->as.field.record = it;
        parser_take_name(p, parser_field_words(p, 0), &field->as.field.name);
        value->as.aggregate.key = field;
        if (parser_at_word(p, 0, "of")) {
            parser_advance(p);
            value->as.aggregate.list = parser_parse_filters(p, parse_primary(p));
        } else {  // in a chain, the step before's result
            value->as.aggregate.list = parser_chain_it(p, ast_new_expr(p->arena, EXPR_IT, field->pos));
        }
        if (!p->sentence_failed && parser_at_word(p, 0, "per")) parser_parse_group(p, value);
        value->pos = ast_pos_join(value->pos, (SourcePos){p->prev_span, 0, 0, 0});
    }
    if (p->sentence_failed) return NULL;
    stmt->as.assign.value = value;
    if (!parse_called(p, &stmt->as.assign.name)) {
        stmt->as.assign.name.text = arena_sprintf(p->arena, "0step%zu", ++p->hidden_lists);
        stmt->as.assign.name.display = "it";
        stmt->as.assign.name.pos = value->pos;
    }
    return p->sentence_failed ? NULL : stmt;
}
