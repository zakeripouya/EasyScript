// Yes/no fields read like describing words, and choices written as values:
//
//   a parcel has a weight and a country, and can be fragile.   (or: a parcel can be fragile.)
//   let box be a new fragile parcel with weight 12.            mark box as not fragile.
//   if box is fragile, ...      the fragile parcels      how many parcels are fragile
//   a parcel's care fee is 4 if it's fragile, otherwise 0.
//   a parcel's price is its base price, plus 12 if it's abroad, plus 4 if it's fragile.
//
// A field written as yes or no (`can be WORD`, or `(yes or no)`) is a
// describing word wherever one a kind defines is (`X is WORD`, `the WORD Ls`,
// `are WORD`, `every WORD ITEM`): it reads the field. A choice is a table
// with no subject (EXPR_TABLE): the value when the condition is yes, the
// other one otherwise.

#include <string.h>
#include "front/parse_internal.h"

bool parser_is_describing(const Parser *p, const char *word) {
    return parser_is_adjective(p, word) || parser_is_yes_field(p, word);
}

// --- a parcel can be fragile. ----------------------------------------------------------

bool parser_at_can_be(const Parser *p) {
    size_t words;
    bool line_start = p->pos == 0 || p->tokens[p->pos - 1].kind == TOK_NEWLINE ||
                      p->tokens[p->pos - 1].kind == TOK_INDENT || p->tokens[p->pos - 1].kind == TOK_DEDENT;
    return line_start && parser_match_record(p, 0, false, &words) >= 0 && parser_at_word(p, words, "can") &&
           parser_at_word(p, words + 1, "be");
}

// "can be WORD": a yes/no field starting as no.
bool parser_parse_can_be_field(Parser *p, Field *field) {
    static const char *const after_field[] = {"and", NULL};
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    parser_consume(p, 2);  // can be
    if (!parser_parse_name(p, &field->name, after_field)) return false;
    field->type = TYPE_YESNO;
    Expr *no = ast_new_expr(p->arena, EXPR_BOOLEAN, field->name.pos);
    no->as.boolean = false;
    field->initial = no;
    field->pos = ast_pos_join(start, field->name.pos);
    return true;
}

// The fields are added to the kind's definition, which has to come first.
Stmt *parser_parse_can_be(Parser *p) {
    size_t words;
    const Token *first = parser_peek(p, 0);
    const RecordKind *kind = &p->records->items[parser_match_record(p, 0, false, &words)];
    parser_consume(p, words);
    Stmt *record = kind->definition;
    if (!record || record->kind != STMT_RECORD) {
        const char *article = strchr("aeiou", kind->display[0]) ? "an" : "a";
        if (parser_error(p, first->span, "Say what %s %s has before what it can be.", article, kind->display)) {
            diag_note(p->diag, "Write it like \"a %s has a name.\" on a line above this one.", kind->display);
            diag_info(p->diag, DIAG_MISSING, "what it has", NULL);
        }
        return NULL;
    }
    for (;;) {
        Field field = {0};
        if (!parser_parse_can_be_field(p, &field)) return NULL;
        vec_push(p->arena, &record->as.record.fields, field);
        if (!parser_at_word(p, 0, "and") || !parser_at_word(p, 1, "can")) break;
        parser_advance(p);
    }
    return NULL;  // nothing to do where it's written
}

// --- mark box as [not] fragile ----------------------------------------------------------

Stmt *parser_parse_mark(Parser *p, const Token *verb) {
    static const char *const as[] = {"as", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_SET, verb);
    Expr *field = NULL;
    if (!parser_parse_target(p, &stmt->as.assign.name, &field, as)) return NULL;
    if (!parser_expect_word(p, as, "mark box as fragile")) return NULL;
    bool not = parser_at_word(p, 0, "not");
    if (not) parser_advance(p);
    const Token *word = parser_peek(p, 0);
    if (!parser_is_name_word(word)) {
        if (parser_error(p, parser_at_sentence_end(p) ? p->prev_span : word->span,
                         "I expected a describing word here, like \"fragile\".")) {
            diag_note(p->diag, "Write it like \"mark box as fragile\" or \"mark box as not fragile\".");
            diag_info(p->diag, DIAG_MISSING, "a describing word", NULL);
        }
        return NULL;
    }
    Expr *base = field;
    if (!base) {
        base = ast_new_expr(p->arena, EXPR_NAME, stmt->as.assign.name.pos);
        base->as.name = stmt->as.assign.name.text;
        base->display = stmt->as.assign.name.display;
    }
    Expr *target = ast_new_expr(p->arena, EXPR_FIELD, base->pos);
    target->as.field.record = base;
    parser_take_name(p, 1, &target->as.field.name);
    target->pos = ast_pos_join(base->pos, target->as.field.name.pos);
    Expr *value = ast_new_expr(p->arena, EXPR_BOOLEAN, target->as.field.name.pos);
    value->as.boolean = !not;
    stmt->as.assign.field = target;
    stmt->as.assign.value = value;
    return stmt;
}

// --- A if C, otherwise B ----------------------------------------------------------------

// "if" here, then ", otherwise" before the sentence ends (at the top level).
bool parser_at_choice(const Parser *p) {
    if (!parser_at_word(p, 0, "if")) return false;
    int depth = 0;
    for (size_t i = 1;; i++) {
        const Token *token = parser_peek(p, i);
        if (token->kind == TOK_PERIOD || token->kind == TOK_NEWLINE || token->kind == TOK_EOF ||
            token->kind == TOK_DEDENT || token->kind == TOK_INDENT) {
            return false;
        }
        if (token->kind == TOK_LPAREN) depth++;
        if (token->kind == TOK_RPAREN) depth--;
        if (depth == 0 && token->kind == TOK_COMMA && parser_at_word(p, i + 1, "otherwise")) return true;
    }
}

static Expr *new_choice(Parser *p, Expr *value, Expr *condition, Expr *otherwise) {
    Expr *choice = ast_new_expr(p->arena, EXPR_TABLE, ast_pos_join(value->pos, otherwise->pos));
    vec_push(p->arena, &choice->as.table.values, value);
    vec_push(p->arena, &choice->as.table.conditions, condition);
    vec_push(p->arena, &choice->as.table.values, otherwise);
    vec_push(p->arena, &choice->as.table.conditions, (Expr *)NULL);
    return choice;
}

Expr *parser_parse_choice(Parser *p, Expr *value) {
    parser_advance(p);  // if
    Expr *condition = parse_or_condition(p);
    if (p->sentence_failed) return parser_error_expr(p);
    parser_consume(p, 2);  // , otherwise
    Expr *otherwise = parse_expression(p);
    return new_choice(p, value, condition, otherwise);
}

// ", plus N if C" / ", minus N if C" after a value: N added (or taken away)
// only when C is yes. Returns the value unchanged (and reads nothing) when no
// "if" follows N.
Expr *parser_parse_add_if(Parser *p, Expr *value) {
    size_t start = p->pos;
    parser_advance(p);  // ,
    const Token *op = parser_advance(p);
    Expr *amount = parse_join(p);
    if (p->sentence_failed || !parser_at_word(p, 0, "if")) {
        if (!p->sentence_failed) p->pos = start;
        return value;
    }
    parser_advance(p);  // if
    Expr *condition = parse_or_condition(p);
    Expr *zero = ast_new_expr(p->arena, EXPR_NUMBER, amount->pos);
    zero->as.number.text = "0";
    zero->as.number.zero = true;  // the same kind as the amount
    Expr *sum = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(value->pos, condition->pos));
    sum->as.binary.op = strcmp(op->text, "plus") == 0 ? BINARY_ADD : BINARY_SUBTRACT;
    sum->as.binary.op_span = op->span;
    sum->as.binary.left = value;
    sum->as.binary.right = new_choice(p, amount, condition, zero);
    return sum;
}
