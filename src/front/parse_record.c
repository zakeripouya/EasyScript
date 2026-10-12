// Records: kinds of things the program defines, their fields, and methods.
//
//   a dog is a kind of thing with:          a new dog with name "Rex" and age 3   (a new circle, likewise)
//       a name, which is text.              rex's name        the order's customer's name
//       an age, which is a number, starting at 0.
//       an owner, which is a person or nothing.
//
//   set rex's age to 4.   add 1 to rex's age.   increase it's age by 1.
//   bark rex.   have a birthday for rex.   rename rex with "Max".   (methods)
//
// The pre-scan (parse_names.c) finds every record kind and field name first,
// so "a dog" can be a kind of value anywhere, and a field's name of several
// words ("start time") is recognized after 's. The lexer makes "rex's" the
// word "rex" and a possessive.

#include <string.h>
#include "front/parse_internal.h"

static const char *const record_example = "a dog is a kind of thing with:";
static const char *const field_example = "a name, which is text.";

// --- Kinds -------------------------------------------------------------------------

int parser_match_record(const Parser *p, size_t ahead, bool plural, size_t *words) {
    int best = -1;
    *words = 0;
    for (size_t i = 0; p->records && i < p->records->len; i++) {
        const RecordKind *kind = &p->records->items[i];
        size_t n = parser_match_words_at(p, ahead, plural ? kind->plural : kind->name);
        if (n > *words) {
            *words = n;
            best = (int)i;
        }
    }
    return best;
}

// --- Defining ------------------------------------------------------------------------

// The pre-scan found it (at the start of a line), so it's a known kind.
bool parser_at_record_definition(const Parser *p) {
    size_t n = 0, words;
    while (parser_is_name_word(parser_peek(p, n))) n++;
    return n > 0 && parser_match_words_at(p, n, "is kind of thing") == 4 && parser_match_record(p, 0, false, &words) >= 0 &&
           words == n;
}

static void skip_sentence(Parser *p) {
    while (!parser_at_sentence_end(p)) parser_advance(p);
}

static bool expect_comma(Parser *p) {
    if (parser_at(p, 0, TOK_COMMA)) {
        parser_advance(p);
        return true;
    }
    const Token *token = parser_peek(p, 0);
    bool reported = parser_at_sentence_end(p)
                        ? parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))
                        : parser_error(p, token->span, "I expected \",\" here, but found %s.", parser_describe(p, token));
    if (reported) diag_note(p->diag, "Write each field like \"%s\".", field_example);
    return false;
}

// a NAME, which is KIND [, starting at E]
static bool parse_field(Parser *p, Field *field) {
    static const char *const which[] = {"which", NULL};
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    if (!parser_parse_name(p, &field->name, which) || !expect_comma(p) ||
        !parser_expect_phrase(p, "which is", "which is", field_example) || !parser_parse_kind(p, false, &field->type)) {
        return false;
    }
    if (parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "starting")) parser_advance(p);
    if (parser_at_word(p, 0, "starting")) {
        if (!parser_expect_phrase(p, "starting at", "starting at", "an age, which is a number, starting at 0.")) {
            return false;
        }
        field->initial = parse_expression(p);
        if (p->sentence_failed) return false;
    }
    field->pos = ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0});
    if (parser_at_sentence_end(p)) return true;
    if (parser_error(p, parser_peek(p, 0)->span, "I expected the field to end after %s.",
                     parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Write one field per sentence, like \"%s\".", field_example);
    }
    return false;
}

// The indented lines after the colon, one field per sentence, up to and
// including the DEDENT that ends them.
static void parse_fields(Parser *p, Stmt *stmt) {
    while (!parser_at(p, 0, TOK_EOF)) {
        TokenKind kind = parser_peek(p, 0)->kind;
        if (kind == TOK_DEDENT) {
            parser_advance(p);
            return;
        }
        if (kind == TOK_NEWLINE || kind == TOK_PERIOD) {
            parser_advance(p);
            continue;
        }
        if (kind == TOK_INDENT) {  // reported, then skipped up to the end of its block
            const Token *indent = parser_advance(p);
            if (parser_error(p, indent->span, "This line is indented more than the fields above it.")) {
                diag_note(p->diag, "Line the fields up with each other.");
            }
            for (size_t depth = 1; depth > 0 && !parser_at(p, 0, TOK_EOF);) {
                TokenKind next = parser_advance(p)->kind;
                depth += next == TOK_INDENT ? 1 : next == TOK_DEDENT ? (size_t)-1 : 0;
            }
            p->sentence_failed = false;
            continue;
        }
        Field field = {0};
        if (parse_field(p, &field)) vec_push(p->arena, &stmt->as.record.fields, field);
        skip_sentence(p);
        p->sentence_failed = false;
    }
}

// The colon, then the fields on the lines below, indented.
static bool parse_record_body(Parser *p, Stmt *stmt) {
    static const char *const with[] = {"with", NULL};
    if (!parser_expect_word(p, with, record_example)) return false;
    if (!parser_at(p, 0, TOK_COLON)) {
        const Token *token = parser_peek(p, 0);
        if (parser_error(p, token->span, "I expected \":\" here, but found %s.", parser_describe(p, token))) {
            diag_note(p->diag, "Write it like \"%s\", with its fields indented below.", record_example);
        }
        return false;
    }
    parser_advance(p);
    if (!parser_at(p, 0, TOK_NEWLINE) || !parser_at(p, 1, TOK_INDENT)) {
        Span span = parser_at(p, 0, TOK_NEWLINE) ? p->prev_span : parser_peek(p, 0)->span;
        if (parser_error(p, span, "The fields of \"%s\" go on the lines below, indented.", stmt->as.record.name.display)) {
            diag_note(p->diag, "Write each one on its own line, like \"    %s\".", field_example);
        }
        return false;
    }
    parser_consume(p, 2);
    parse_fields(p, stmt);
    return true;
}

Stmt *parse_record(Parser *p) {
    static const char *const is[] = {"is", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_RECORD, parser_peek(p, 0));
    size_t words;
    stmt->as.record.index = (size_t)parser_match_record(p, 0, false, &words);
    bool ok = parser_parse_name(p, &stmt->as.record.name, is);
    if (ok) {
        parser_consume(p, 4);  // is kind of thing
        stmt->pos = ast_pos_join(stmt->pos, (SourcePos){p->prev_span, 0, 0, 0});
        ok = parse_record_body(p, stmt);
    }
    if (!ok) parser_skip_line_and_block(p);
    p->ended_with_block = true;
    if (!ok) return NULL;
    RecordKind *kind = &p->records->items[stmt->as.record.index];
    if (!kind->definition) kind->definition = stmt;
    return stmt;
}

// --- "a dog has a name (text) and an age (a number)." ---------------------------------

bool parser_at_has_definition(const Parser *p) {
    size_t n = parser_has_definition_words(p, p->pos), words;
    return n > 0 && parser_match_record(p, 0, false, &words) >= 0 && words == n;
}

static const char *const has_example = "a dog has a name (text) and an age (a number).";

// "(KIND)", "(KIND, starting at E)" or "(starting at E)" after a field's name.
static bool parse_has_details(Parser *p, Field *field) {
    const Token *open = parser_advance(p);
    if (!parser_at_word(p, 0, "starting")) {
        if (!parser_parse_kind(p, false, &field->type)) return false;
        if (parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "starting")) parser_advance(p);
    }
    if (parser_at_word(p, 0, "starting")) {
        if (!parser_expect_phrase(p, "starting at", "starting at", "an age (a number, starting at 0)")) return false;
        field->initial = parse_expression(p);
        if (p->sentence_failed) return false;
    }
    if (parser_at(p, 0, TOK_RPAREN)) {
        parser_advance(p);
        return true;
    }
    const Token *token = parser_peek(p, 0);
    if (parser_error(p, token->span, "I expected \")\" to close the \"(\" on line %zu, but found %s.", open->line,
                     parser_describe(p, token))) {
        diag_note(p->diag, "Write a field's kind in parentheses after its name, like \"%s\".", has_example);
        diag_info(p->diag, DIAG_MISSING, "\")\"", NULL);
    }
    return false;
}

// NAME has FIELD [(...)] [, | and] FIELD [(...)] ...
Stmt *parser_parse_has(Parser *p) {
    static const char *const has[] = {"has", NULL};
    static const char *const after_field[] = {"and", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_RECORD, parser_peek(p, 0));
    size_t words;
    stmt->as.record.index = (size_t)parser_match_record(p, 0, false, &words);
    stmt->as.record.has = true;
    if (!parser_parse_name(p, &stmt->as.record.name, has)) return NULL;
    parser_advance(p);  // has
    for (;;) {
        Field field = {0};
        field.type = TYPE_UNKNOWN;
        SourcePos start = parser_token_pos(parser_peek(p, 0));
        if (parser_at_word(p, 0, "can") && parser_at_word(p, 1, "be")) {  // "and can be fragile"
            if (!parser_parse_can_be_field(p, &field)) return NULL;
            vec_push(p->arena, &stmt->as.record.fields, field);
            if (parser_at_sentence_end(p)) break;
            bool comma = parser_at(p, 0, TOK_COMMA);
            if (comma) parser_advance(p);
            if (parser_at_word(p, 0, "and")) parser_advance(p);
            continue;
        }
        if (parser_at_word(p, 0, "some") && parser_is_name_word(parser_peek(p, 1))) parser_advance(p);  // "some pets"
        if (!parser_parse_name(p, &field.name, after_field)) return NULL;
        if (parser_at(p, 0, TOK_LPAREN) && !parse_has_details(p, &field)) return NULL;
        field.pos = ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0});
        vec_push(p->arena, &stmt->as.record.fields, field);
        if (parser_at_sentence_end(p)) break;
        bool comma = parser_at(p, 0, TOK_COMMA);
        if (comma) parser_advance(p);
        bool and = parser_at_word(p, 0, "and");
        if (and) parser_advance(p);
        if (!comma && !and) {
            const Token *token = parser_peek(p, 0);
            if (parser_error(p, token->span, "I expected \",\" or \"and\" before the next field, but found %s.",
                             parser_describe(p, token))) {
                diag_note(p->diag, "Write the fields like \"%s\".", has_example);
                diag_info(p->diag, DIAG_MISSING, "\",\" or \"and\" between the fields", NULL);
                diag_choice(p->diag, ",");
                diag_choice(p->diag, "and");
            }
            return NULL;
        }
    }
    RecordKind *kind = &p->records->items[stmt->as.record.index];
    if (!kind->definition) kind->definition = stmt;
    return stmt;
}

// --- Fields ----------------------------------------------------------------------------

size_t parser_known_field_words(const Parser *p, size_t ahead) {
    size_t best = 0;
    for (size_t i = 0; i < p->fields.len; i++) {
        size_t n = parser_match_words_at(p, ahead, p->fields.items[i].text);
        if (n > best) best = n;
    }
    return best;
}

// How many words the field name starting `ahead` tokens from here has: the
// longest known one, or one word; 0 if no name starts there.
static size_t field_words(const Parser *p, size_t ahead) {
    size_t best = parser_known_field_words(p, ahead);
    return best ? best : parser_is_name_word(parser_peek(p, ahead)) ? 1 : 0;
}

Expr *parser_parse_field(Parser *p, Expr *record) {
    parser_advance(p);  // 's
    size_t words = field_words(p, 0);
    if (words == 0) {
        if (parser_error(p, p->prev_span, "Something is missing after \"'s\".")) {
            diag_note(p->diag, "Write the name of a field after it, like \"rex's name\".");
        }
        return parser_error_expr(p);
    }
    const Token *word = parser_peek(p, 0);
    if (record->kind == EXPR_IT && words == 1 && parser_is_adjective(p, word->text) &&
        parser_known_field_words(p, 0) == 0) {  // "if it's abroad": it is abroad
        parser_advance(p);
        return parser_adjective_call(p, record, word->text, ast_pos_join(record->pos, parser_token_pos(word)));
    }
    Expr *expr = ast_new_expr(p->arena, EXPR_FIELD, record->pos);
    expr->as.field.record = record;
    parser_take_name(p, words, &expr->as.field.name);
    expr->pos = ast_pos_join(record->pos, expr->as.field.name.pos);
    return expr;
}

bool parser_parse_target(Parser *p, Name *name, Expr **field, const char *const *next) {
    *field = NULL;
    if (parser_at_every_target(p)) {  // "set every monster's color to blue"
        *field = parser_parse_every_target(p);
        return !p->sentence_failed;
    }
    if (parser_at_lookup(p)) {  // "set the value for "tea" in prices to 3"
        *field = parser_parse_lookup(p);
        if ((*field)->kind == EXPR_MAP_GET && !(*field)->as.lookup.fallback) return !p->sentence_failed;
        if ((*field)->kind == EXPR_MAP_GET &&
            parser_error(p, (*field)->as.lookup.fallback->pos.span, "A value that changes can't have \", or\".")) {
            diag_note(p->diag, "\", or\" gives another value when the key is missing; to change the value, the key "
                               "has to be there. Check first with \"has\".");
        }
        return false;
    }
    Expr *base;
    size_t words = parser_field_words(p, 0);
    if (words > 0 && parser_at_word(p, words, "of")) {  // "set the age of rex to 5"
        base = parse_primary(p);
        if (base->kind != EXPR_FIELD || p->sentence_failed) return false;
        *field = base;
        return true;
    }
    if (parser_at(p, 0, TOK_LPAREN)) {  // "set (item 2 of players)'s name to ..."
        const Token *open = parser_peek(p, 0);
        base = parse_primary(p);
        if (base->kind == EXPR_FIELD && !p->sentence_failed) {
            *field = base;
            return true;
        }
        if (parser_error(p, open->span, "I expected a name or a field here, like \"rex's name\".")) {
            diag_note(p->diag, "A value in parentheses can only be changed through one of its fields, like "
                               "\"(item 1 of dogs)'s name\".");
        }
        return false;
    } else if (parser_at_word(p, 0, "it") && parser_at(p, 1, TOK_POSSESSIVE)) {
        base = ast_new_expr(p->arena, EXPR_IT, parser_token_pos(parser_advance(p)));
    } else {
        if (!parser_parse_name(p, name, next)) return false;
        if (!parser_at(p, 0, TOK_POSSESSIVE)) return true;
        base = ast_new_expr(p->arena, EXPR_NAME, name->pos);
        base->as.name = name->text;
        base->display = name->display;
    }
    while (parser_at(p, 0, TOK_POSSESSIVE)) {
        base = parser_parse_field(p, base);
        if (base->kind == EXPR_ERROR) return false;
    }
    *field = base;
    return true;
}

// --- New records ----------------------------------------------------------------------------

// "new fragile urgent": how many yes/no fields come before the kind's name.
static size_t yes_words(const Parser *p, size_t ahead) {
    size_t n = 0, words;
    while (parser_peek(p, ahead + n)->kind == TOK_WORD && parser_is_yes_field(p, parser_peek(p, ahead + n)->text) &&
           parser_match_record(p, ahead + n, false, &words) < 0) {
        n++;
    }
    return n;
}

static bool at_new_at(const Parser *p, size_t ahead) {
    size_t words;
    if (!parser_at_word(p, ahead, "new")) return false;
    ahead += 1 + yes_words(p, ahead + 1);
    return parser_match_record(p, ahead, false, &words) >= 0 || parser_match_variant(p, ahead, &words) >= 0;
}

bool parser_at_new(const Parser *p) {
    return at_new_at(p, 0);
}

// After a field's value: "and" or "," and another field. Before another new
// value ("a list of a new point with x 1 and y 2, a new point ..."), or, in an
// item of a list or map, something that can't be a field's name (the map's
// next key), the new value ends and the list or map around it goes on.
static bool at_next_field(const Parser *p) {
    bool separator = (parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call")) || parser_at(p, 0, TOK_COMMA);
    return separator && !at_new_at(p, 1) && (!p->in_items || parser_peek(p, 1)->kind == TOK_WORD);
}

static bool is_parenthesized(const Parser *p, const Expr *expr) {
    Span span = expr->pos.span;
    return span.length >= 2 && p->source[span.offset] == '(' && p->source[span.offset + span.length - 1] == ')';
}

// NAME VALUE, where VALUE is a value up to a comparison (arithmetic and
// "followed by" are fine).
static bool parse_field_value(Parser *p, Expr *expr) {
    size_t words = field_words(p, 0);
    if (words == 0) {
        const Token *token = parser_peek(p, 0);
        bool reported = parser_at_sentence_end(p)
                            ? parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))
                            : parser_error(p, token->span, "I expected the name of a field here, but found %s.",
                                           parser_describe(p, token));
        if (reported) diag_note(p->diag, "Give each field's name and then its value, like \"a new dog with name \"Rex\" "
                                         "and age 3\".");
        return false;
    }
    FieldValue value = {0};
    parser_take_name(p, words, &value.field);
    value.value = parse_join(p);
    if (p->sentence_failed) return false;
    const Expr *inner = value.value;
    if (inner->kind == EXPR_NEW && inner->as.new_record.values.len > 0 && !is_parenthesized(p, inner)) {
        if (parser_error(p, inner->pos.span, "A new record inside another one needs parentheses.")) {
            diag_note(p->diag, "Otherwise it isn't clear which one the fields after it belong to. Write it like "
                               "\"(%s)\".", parser_text(p, inner->pos.span));
        }
        return false;
    }
    vec_push(p->arena, &expr->as.new_record.values, value);
    return true;
}

Expr *parser_parse_new(Parser *p) {
    const Token *new_word = parser_advance(p);
    SourcePos start = parser_token_pos(new_word);
    if (new_word->filler.length > 0) {  // the span starts at "a new": messages quote it whole
        start.span.length += start.span.offset - new_word->filler.offset;
        start.span.offset = new_word->filler.offset;
    }
    Vec(FieldValue) described = {0};  // "a new fragile parcel": fragile is yes
    for (size_t n = yes_words(p, 0); n > 0; n--) {
        FieldValue value = {0};
        parser_take_name(p, 1, &value.field);
        Expr *yes = ast_new_expr(p->arena, EXPR_BOOLEAN, value.field.pos);
        yes->as.boolean = true;
        value.value = yes;
        vec_push(p->arena, &described, value);
    }
    size_t words, variant_words;
    int index = parser_match_record(p, 0, false, &words);
    int variant = parser_match_variant(p, 0, &variant_words);
    Expr *expr = ast_new_expr(p->arena, EXPR_NEW, start);
    if (variant >= 0 && (index < 0 || variant_words > words)) {  // "a new circle": a shape
        words = variant_words;
        expr->as.new_record.variant = (size_t)variant;
        expr->as.new_record.type = type_sum(p->records->variants.items[variant].sum);
    } else {
        expr->as.new_record.type = type_record((size_t)index);
    }
    parser_consume(p, words);
    for (size_t i = 0; i < described.len; i++) vec_push(p->arena, &expr->as.new_record.values, described.items[i]);
    if (parser_at_word(p, 0, "with")) {
        parser_advance(p);
        do {
            if (!parse_field_value(p, expr)) return parser_error_expr(p);
        } while (at_next_field(p) && parser_advance(p));
    }
    expr->pos = ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0});
    return expr;
}

// --- Methods ----------------------------------------------------------------------------

void parser_parse_method_args(Parser *p, Expr *call) {
    if (parser_at_word(p, 0, "for")) parser_advance(p);
    bool sentence = p->call_sentence;  // for the inputs after the record
    p->call_sentence = false;
    p->method_record = true;  // the "with" after it belongs to the method
    Expr *record = parse_unary(p);
    p->method_record = false;
    p->call_sentence = sentence;
    vec_push(p->arena, &call->as.call.args, record);
    call->pos = ast_pos_join(call->pos, record->pos);
    if (parser_at_word(p, 0, "and") && !parser_at_word(p, 1, "call")) {
        parser_advance(p);
        parser_parse_call_args(p, call, false);
    } else if (parser_at_call_intro(p, call->as.call.name)) {  // with/using/of, or the definition's word
        bool commas = parser_at_word(p, 0, "using");
        parser_advance(p);
        parser_parse_call_args(p, call, commas);
    }
    p->call_sentence = false;
}
