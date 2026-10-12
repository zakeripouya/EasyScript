// Acting on a whole group at once:
//
//   remove the tasks that are done.          (also "whose ...", "where ...", "the done tasks")
//   set every monster's color to blue.       (and the other sentences that change a field)
//
// "remove the Xs that ..." keeps the other items: it's "keep only the Xs in L
// where not ..." (LIST_KEEP). A sentence about "every ITEM's FIELD" is that
// sentence about "it's FIELD", done for each item: a for-each loop over the
// list named like the item's plural (STMT_LOOP), the way it would be written by hand.

#include <string.h>
#include "front/parse_internal.h"

// --- remove the tasks that are done ----------------------------------------------------

// After "remove": no "from" before the sentence ends, and a filter ("that",
// "whose", "where") or an adjective first ("the done tasks").
bool parser_at_remove_filter(const Parser *p) {
    if (parser_is_describing(p, parser_peek(p, 0)->text) && parser_is_name_word(parser_peek(p, 1))) return true;
    bool filter = false;
    int depth = 0;
    for (size_t i = 0;; i++) {
        const Token *token = parser_peek(p, i);
        if (token->kind == TOK_PERIOD || token->kind == TOK_NEWLINE || token->kind == TOK_EOF ||
            token->kind == TOK_DEDENT || token->kind == TOK_INDENT) {
            return filter;
        }
        if (token->kind == TOK_LPAREN) depth++;
        if (token->kind == TOK_RPAREN) depth--;
        if (depth == 0 && parser_at_word(p, i, "from")) return false;
        if (depth == 0 && (parser_at_word(p, i, "that") || parser_at_word(p, i, "whose") || parser_at_word(p, i, "where"))) {
            filter = true;
        }
    }
}

Stmt *parser_parse_remove_filter(Parser *p, Stmt *stmt) {
    Expr *value = parser_parse_filters(p, parse_primary(p));
    if (p->sentence_failed) return NULL;
    if (value->kind != EXPR_ONLY) {
        if (parser_error(p, value->pos.span, "I expected the items to remove here, like \"the tasks that are done\".")) {
            diag_note(p->diag, "To remove one item, write \"remove item 2 from tasks\".");
            diag_info(p->diag, DIAG_MISSING, "which items to remove", NULL);
        }
        return NULL;
    }
    Expr *condition = value->as.only.condition, *list = value->as.only.list;
    while (list->kind == EXPR_ONLY) {  // "the done tasks that are old": both
        Expr *both = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(list->as.only.condition->pos, condition->pos));
        both->as.binary.op = BINARY_AND;
        both->as.binary.op_span = condition->pos.span;
        both->as.binary.left = list->as.only.condition;
        both->as.binary.right = condition;
        condition = both;
        list = list->as.only.list;
    }
    stmt->as.list.op = LIST_KEEP;
    stmt->as.list.list = list;
    Expr *hidden = ast_new_expr(p->arena, EXPR_NAME, list->pos);  // the list as it was, while the kept items are chosen
    hidden->as.name = arena_sprintf(p->arena, "0list%zu", ++p->hidden_lists);
    hidden->display = parser_text(p, list->pos.span);
    stmt->as.list.hidden = hidden;
    Expr *not = ast_new_expr(p->arena, EXPR_UNARY, condition->pos);
    not->as.unary.op = UNARY_NOT;
    not->as.unary.op_span = condition->pos.span;
    not->as.unary.operand = condition;
    Expr *kept = ast_new_expr(p->arena, EXPR_ONLY, value->pos);
    kept->as.only.list = hidden;
    kept->as.only.condition = not;
    stmt->as.list.value = kept;
    return stmt;
}

// --- set every monster's color to blue -------------------------------------------------

// "every ITEM's": how many words name the item, or 0.
static size_t every_words(const Parser *p) {
    if (!parser_at_word(p, 0, "every")) return 0;
    size_t n = 0;
    while (parser_is_name_word(parser_peek(p, 1 + n))) n++;
    return n > 0 && parser_at(p, 1 + n, TOK_POSSESSIVE) ? n : 0;
}

bool parser_at_every_target(const Parser *p) {
    return every_words(p) > 0;
}

// "every monster's color": the field of "it", for each item of the list named
// like the item's plural, which the sentence is then done for (p->group).
Expr *parser_parse_every_target(Parser *p) {
    size_t n = every_words(p);
    const Token *every = parser_advance(p);
    Name item;
    parser_take_name(p, n, &item);
    p->group_list = parser_implied_list(p, &item);
    p->group_item = item;
    p->group_every = every->span;
    Expr *base = ast_new_expr(p->arena, EXPR_IT, item.pos);
    while (parser_at(p, 0, TOK_POSSESSIVE) && !p->sentence_failed) base = parser_parse_field(p, base);
    return base;
}

// The sentence about "it's FIELD", done for each item of the group.
Stmt *parser_finish_group(Parser *p, Stmt *stmt) {
    Expr *list = p->group_list;
    p->group_list = NULL;
    if (!stmt) return NULL;
    Stmt *loop = ast_new_stmt(p->arena, STMT_LOOP, stmt->pos);
    loop->verb = stmt->verb;
    loop->as.loop.kind = LOOP_EACH;
    loop->as.loop.list = list;
    loop->as.loop.implied = list->kind == EXPR_NAME;
    loop->as.loop.var = p->group_item;
    loop->as.loop.var.text = arena_sprintf(p->arena, "0every%zu", ++p->hidden_lists);  // a name nothing else uses
    loop->as.loop.var_named = true;
    vec_push(p->arena, &loop->as.loop.body, stmt);
    return loop;
}
