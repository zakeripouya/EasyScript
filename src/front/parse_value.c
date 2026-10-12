// Values: the tightest level of the expression grammar. Literals, names,
// calls (NAME of A and B), groups in parentheses, the prefix forms ("length
// of X", "contents of file X"), and the list and map forms that start with a
// word (parse_list.c, parse_map.c, and "only" in parse_expr.c). The
// operators around them are in parse_expr.c.

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include "front/parse_internal.h"

static Expr *expected_value(Parser *p) {
    if (parser_reject_bare_decimal(p)) return parser_error_expr(p);
    const Token *token = parser_peek(p, 0);
    bool after_open_paren = p->pos > 0 && p->tokens[p->pos - 1].kind == TOK_LPAREN;
    // A ":" or "," straight after an operator ("if x plus:") also means the value is missing.
    bool ends_here = parser_at_sentence_end(p) || token->kind == TOK_COLON || token->kind == TOK_COMMA;
    if (ends_here || (token->kind == TOK_RPAREN && after_open_paren)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "Add a value there, like a number, some text in quotes, or a name.");
        }
    } else if (token->kind == TOK_RPAREN) {
        parser_report_leftover(p);
    } else if (parser_error(p, token->span, "I expected a value here, but found %s.", parser_describe(p, token))) {
        diag_note(p->diag, "A value is a number, some text in quotes, yes or no, nothing, or a name.");
    }
    return parser_error_expr(p);
}

static Expr *literal(Parser *p, ExprKind kind) {
    return ast_new_expr(p->arena, kind, parser_token_pos(parser_advance(p)));
}

static Type currency_word(const char *word) {
    if (strcmp(word, "dollars") == 0 || strcmp(word, "dollar") == 0) return TYPE_DOLLARS;
    if (strcmp(word, "euros") == 0 || strcmp(word, "euro") == 0) return TYPE_EUROS;
    if (strcmp(word, "pounds") == 0 || strcmp(word, "pound") == 0) return TYPE_POUNDS;
    return 0;
}

// "4.50 dollars": the number in cents, as money (at most two decimals).
static Expr *money_after_number(Parser *p, Expr *expr, const Token *token) {
    const Token *word = parser_advance(p);
    expr->as.number.money = currency_word(word->text);
    p->records->money = true;
    expr->pos = ast_pos_join(expr->pos, parser_token_pos(word));
    const char *point = strchr(token->text, '.');
    size_t decimals = point ? strlen(point + 1) : 0;
    if (decimals > 2) {
        if (parser_error(p, expr->pos.span, "An amount of money has at most two decimals, for the cents.")) {
            diag_note(p->diag, "Round it first, like \"%.*s %s\".", (int)(point - token->text + 3), token->text,
                      word->text);
            diag_info(p->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        }
        return parser_error_expr(p);
    }
    double cents = round(strtod(token->text, NULL) * 100);
    if (cents >= 9007199254740992.0) {
        if (parser_error(p, expr->pos.span, "This amount of money is too big for me to keep exactly.")) {
            diag_note(p->diag, "Amounts can go up to 90 trillion.");
            diag_info(p->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        }
        return parser_error_expr(p);
    }
    expr->as.number.text = arena_sprintf(p->arena, "%.0f", cents);
    expr->as.number.is_decimal = false;
    return expr;
}

static Expr *parse_number(Parser *p) {
    Expr *expr = literal(p, EXPR_NUMBER);
    const Token *token = &p->tokens[p->pos - 1];
    expr->as.number.text = token->text;
    expr->as.number.is_decimal = strchr(token->text, '.') != NULL;
    const Token *next = parser_peek(p, 0);
    if (next->kind == TOK_WORD && currency_word(next->text) && !parser_match_name(p, 0, false)) {
        return money_after_number(p, expr, token);  // 4.50 dollars
    }
    return expr;
}

// "$1,200": the lexer has the currency and the cents ("dollars 120000").
static Expr *parse_money(Parser *p) {
    Expr *expr = literal(p, EXPR_NUMBER);
    const Token *token = &p->tokens[p->pos - 1];
    const char *space = strchr(token->text, ' ');
    expr->as.number.money = currency_word(arena_sprintf(p->arena, "%.*s", (int)(space - token->text), token->text));
    p->records->money = true;
    expr->as.number.text = space + 1;
    return expr;
}

static Expr *text_piece(Parser *p) {
    Expr *expr = literal(p, EXPR_TEXT);
    const Token *token = &p->tokens[p->pos - 1];
    expr->as.text.value = token->text;
    expr->as.text.len = token->text_len;
    return expr;
}

static Expr *join(Parser *p, Expr *left, Expr *right, Span op_span) {
    Expr *joined = ast_new_expr(p->arena, EXPR_BINARY, ast_pos_join(left->pos, right->pos));
    joined->as.binary.op = BINARY_JOIN;
    joined->as.binary.op_span = op_span;
    joined->as.binary.left = left;
    joined->as.binary.right = right;
    return joined;
}

// "Hello, [name]!" is "Hello, " followed by name followed by "!": the
// lexer splits it into STRING [ ... ] STRING. Empty pieces after the first
// are left out.
static Expr *parse_text(Parser *p) {
    Expr *expr = text_piece(p);
    while (parser_at(p, 0, TOK_LBRACKET)) {
        size_t close = 1;
        while (!parser_at(p, close, TOK_RBRACKET) && !parser_at(p, close, TOK_EOF)) close++;  // the lexer ends each value with one
        if (parser_peek(p, close)->text_len == 0) {  // no "]" in the text: the lexer reported it
            p->sentence_failed = true;
            return parser_error_expr(p);
        }
        const Token *open = parser_advance(p);
        if (parser_at(p, 0, TOK_RBRACKET)) {
            if (parser_error(p, (Span){open->span.offset, parser_peek(p, 0)->span.offset + 1 - open->span.offset},
                                    "There's no value between these brackets.")) {
                diag_note(p->diag, "Put a value in them, like \"[name]\". To put a \"[\" in text, write it twice: "
                                   "\"[[\".");
                diag_info(p->diag, DIAG_MISSING, "a value between the brackets", NULL);
            }
            return parser_error_expr(p);
        }
        bool as_ends_value = p->as_ends_value, no_call = p->no_call;
        p->as_ends_value = p->no_call = false;
        Expr *value = parse_expression(p);
        p->as_ends_value = as_ends_value;
        p->no_call = no_call;
        if (p->sentence_failed) return value;
        if (!parser_at(p, 0, TOK_RBRACKET)) {
            const Token *token = parser_peek(p, 0);
            if (parser_error(p, token->span, "I expected \"]\" to end the value in the text, but found %s.",
                             parser_describe(p, token))) {
                diag_note(p->diag, "The brackets hold one value, like \"[total plus 1]\".");
                diag_info(p->diag, DIAG_MISSING, "\"]\"", NULL);
            }
            return parser_error_expr(p);
        }
        parser_advance(p);
        expr = join(p, expr, value, open->span);
        Expr *rest = text_piece(p);  // the lexer always puts text after "]"
        expr = rest->as.text.len > 0 ? join(p, expr, rest, open->span) : expr;
        expr->pos = ast_pos_join(expr->pos, rest->pos);
    }
    return expr;
}

static Expr *parse_group(Parser *p) {
    const Token *open = parser_advance(p);
    bool as_ends_value = p->as_ends_value, no_call = p->no_call, no_best = p->no_best;  // inside parentheses, "as" converts again
    bool in_items = p->in_items;
    p->as_ends_value = p->no_call = p->no_best = p->in_items = false;
    Expr *inner = parse_expression(p);
    p->in_items = in_items;
    p->as_ends_value = as_ends_value;
    p->no_call = no_call;
    p->no_best = no_best;
    if (!parser_at(p, 0, TOK_RPAREN)) {
        if (parser_error(p, open->span, "This \"(\" is never closed.")) {
            diag_note(p->diag, "Add a \")\" where the group ends, on the same line.");
        }
        return inner;
    }
    const Token *close = parser_advance(p);
    inner->pos = ast_pos_join(parser_token_pos(open), parser_token_pos(close));
    return inner;
}

// "length of X" and "contents of file X" take a single primary, so
// "length of x plus 1" means "(length of x) plus 1".
static Expr *parse_prefixed(Parser *p, ExprKind kind, size_t words) {
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    parser_consume(p, words);
    Expr *operand = parse_primary(p);
    Expr *expr = ast_new_expr(p->arena, kind, ast_pos_join(start, operand->pos));
    expr->as.operand = operand;
    return expr;
}

static bool is_parenthesized(const Parser *p, const Expr *expr) {
    Span span = expr->pos.span;
    return span.length >= 2 && p->source[span.offset] == '(' && p->source[span.offset + span.length - 1] == ')';
}

// An argument directly followed by arithmetic could belong to the call or
// to the whole sentence ("double using 21 plus 1"), so that's an error.
// A last argument in parentheses is already closed, so then the arithmetic
// belongs outside: "fib of (n minus 1) plus 1".
static Expr *reject_ambiguous_call(Parser *p, Expr *call) {
    if (is_parenthesized(p, call->as.call.args.items[call->as.call.args.len - 1])) return call;
    size_t len;
    if (!parser_match_arithmetic(p, &len)) return call;
    Span op_span = parser_consume(p, len);
    Expr *right = parse_unary(p);
    const Expr *last_arg = call->as.call.args.items[call->as.call.args.len - 1];
    Span head = {call->pos.span.offset, last_arg->pos.span.offset - call->pos.span.offset};
    SourcePos whole = ast_pos_join(call->pos, right->pos);
    const char *call_text = parser_text(p, call->pos.span);
    const char *op_text = parser_text(p, op_span);
    const char *right_text = parser_text(p, right->pos.span);
    if (parser_error(p, whole.span, "\"%s\" could mean two things.", parser_text(p, whole.span))) {
        diag_note(p->diag, "Use parentheses to say which: \"(%s) %s %s\" or \"%s(%s %s %s)\".", call_text, op_text,
                  right_text, parser_text(p, head), parser_text(p, last_arg->pos.span), op_text, right_text);
    }
    return ast_new_expr(p->arena, EXPR_ERROR, whole);
}

// The word the call's definition puts before input `index` ("in" in "to
// column of cell in grid:"), or NULL.
static const char *joiner_before(const Parser *p, const Expr *call, size_t index) {
    if (!call->as.call.name) return NULL;
    for (size_t i = 0; i < p->names.len; i++) {
        const KnownName *known = &p->names.items[i];
        if (strcmp(known->text, call->as.call.name) != 0) continue;
        return known->joiners && index < known->joiner_count ? known->joiners[index] : NULL;
    }
    return NULL;
}

static bool at_argument_separator(const Parser *p, const Expr *call, bool commas) {
    if (parser_at_and(p)) return true;
    const char *joiner = joiner_before(p, call, call->as.call.args.len);
    if (joiner && parser_at_word(p, 0, joiner)) return true;  // column of cell in grid
    return commas && parser_at(p, 0, TOK_COMMA);
}

// In a call sentence ("report with "bad" and "I expected " followed by
// word"), the call's result isn't used, so arithmetic or "followed by" after
// an input can only be part of it: each input is a whole value up to "and".
void parser_parse_call_args(Parser *p, Expr *call, bool commas) {
    bool sentence = p->call_sentence;
    p->call_sentence = false;
    do {
        Expr *arg = sentence ? parse_join(p) : parse_unary(p);
        vec_push(p->arena, &call->as.call.args, arg);
        call->pos = ast_pos_join(call->pos, arg->pos);
    } while (at_argument_separator(p, call, commas) && parser_advance(p));
}

Expr *parser_finish_call(Parser *p, Expr *call) {
    return call->as.call.args.len > 0 ? reject_ambiguous_call(p, call) : call;
}

// NAME of A and B / NAME with A and B / NAME using A, B (and NAME to A for a
// function defined with "to"). Each argument is a unary expression.
static Expr *parse_call(Parser *p, const Name *name) {
    const Token *intro = parser_advance(p);  // "of", "with", "using", or "to"
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, name->pos);
    call->as.call.name = name->text;
    call->display = name->display;
    parser_parse_call_args(p, call, strcmp(intro->text, "using") == 0);
    return parser_finish_call(p, call);
}

// A name (the longest known one, if several words start one), or a call.
static Expr *parse_name_or_call(Parser *p) {
    const KnownName *known = parser_match_name(p, 0, false);
    Name name;
    parser_take_name(p, known ? known->words : 1, &name);
    bool method_record = p->method_record;
    p->method_record = false;
    if ((method_record || p->no_call) && !parser_is_function(p, name.text)) {
        Expr *expr = ast_new_expr(p->arena, EXPR_NAME, name.pos);
        expr->as.name = name.text;
        expr->display = name.display;
        return expr;
    }
    // "word with "a" replaced by "b"" replaces text, and "total with commas" writes a number, unless the name is a function.
    bool replace = (parser_at_replace(p) || parser_match_words(p, "with commas")) && !parser_is_function(p, name.text);
    if (parser_is_method(p, name.text) && (parser_at_word(p, 0, "for") || parser_at_call_intro(p, name.text) ||
                                           parser_starts_operand(parser_peek(p, 0)))) {
        Expr *call = ast_new_expr(p->arena, EXPR_CALL, name.pos);  // bark rex / have a birthday for rex
        call->as.call.name = name.text;
        call->display = name.display;
        if (parser_at_call_intro(p, name.text)) parser_advance(p);
        parser_parse_method_args(p, call);
        return parser_finish_call(p, call);
    }
    if (!replace && parser_at_call_intro(p, name.text)) return parse_call(p, &name);
    Expr *expr = ast_new_expr(p->arena, EXPR_NAME, name.pos);
    expr->as.name = name.text;
    expr->display = name.display;
    return expr;
}

// "the name of rex": the field "name" of rex, like "rex's name". The record
// is a primary, so "the name of the owner of rex" works from the right. A
// function with the field's name makes it ambiguous: an error.
static Expr *parse_field_of(Parser *p, size_t words) {
    Name name;
    parser_take_name(p, words, &name);
    Span of = parser_advance(p)->span;
    if (parser_at_sentence_end(p)) {
        if (parser_error(p, of, "Something is missing after \"of\".")) {
            diag_note(p->diag, "Say whose %s it is, like \"the %s of rex\".", name.display, name.display);
            diag_info(p->diag, DIAG_MISSING, arena_sprintf(p->arena, "whose %s it is", name.display), NULL);
        }
        return parser_error_expr(p);
    }
    Expr *record = parse_primary(p);
    if (p->sentence_failed) return record;
    Expr *expr = ast_new_expr(p->arena, EXPR_FIELD, ast_pos_join(name.pos, record->pos));
    if (parser_is_function(p, name.text)) {
        const char *whole = parser_text(p, expr->pos.span), *owner = parser_text(p, record->pos.span);
        if (parser_error(p, expr->pos.span, "\"%s\" could mean two things.", whole)) {
            diag_note(p->diag, "\"%s\" is a field and a definition. For the field, write \"%s's %s\"; for the definition, "
                               "write \"%s with %s\".", name.display, owner, name.display, name.display, owner);
            diag_info(p->diag, DIAG_TWO_MEANINGS, arena_sprintf(p->arena, "which \"%s\" is meant", name.display), NULL);
            diag_choice(p->diag, "%s's %s", owner, name.display);
            diag_choice(p->diag, "%s with %s", name.display, owner);
        }
        return parser_error_expr(p);
    }
    expr->as.field.record = record;
    expr->as.field.name = name;
    return expr;
}

// "every NAME in" anywhere but the start of a value.
static Expr *reject_inner_every(Parser *p) {
    const Token *word = parser_advance(p);
    if (parser_error(p, word->span, "\"%s\" has to start the value, because everything after it is worked out for "
                                    "each item.", word->text)) {
        diag_note(p->diag, "Put it in parentheses, like \"(%s score in scores plus 5)\".", word->text);
    }
    return parser_error_expr(p);
}

static Expr *parse_word(Parser *p) {
    const Token *token = parser_peek(p, 0);
    const char *word = token->text;
    if (strcmp(word, "no") == 0) {  // "no score is below 0" (a test), not the value no
        Expr *none = parser_parse_aggregate_word(p);
        if (none) return none;
    }
    if (strcmp(word, "yes") == 0 || strcmp(word, "true") == 0 || strcmp(word, "no") == 0 ||
        strcmp(word, "false") == 0) {
        Expr *expr = literal(p, EXPR_BOOLEAN);
        expr->as.boolean = word[0] == 'y' || word[0] == 't';
        return expr;
    }
    if (strcmp(word, "nothing") == 0) return literal(p, EXPR_NOTHING);
    if (strcmp(word, "reason") == 0 && p->in_failure) {  // "if that fails, say the reason"
        Expr *reason = literal(p, EXPR_LIBRARY);
        reason->as.library.op = LIB_REASON;
        reason->as.library.op_span = reason->pos.span;
        return reason;
    }
    if (strcmp(word, "it") == 0) return parser_chain_it(p, literal(p, EXPR_IT));
    if (strcmp(word, "they") == 0 && p->describing) return literal(p, EXPR_IT);  // "if they have more than 5 orders"
    if ((strcmp(word, "its") == 0 || strcmp(word, "their") == 0) &&
        parser_field_words(p, 1) > 0) {  // "its radius", "their orders": "it's radius"
        Expr *it = parser_chain_it(p, literal(p, EXPR_IT));
        Expr *expr = ast_new_expr(p->arena, EXPR_FIELD, it->pos);
        expr->as.field.record = it;
        parser_take_name(p, parser_field_words(p, 0), &expr->as.field.name);
        expr->pos = ast_pos_join(it->pos, expr->as.field.name.pos);
        return expr;
    }
    if (parser_at_word(p, 0, "what") && (parser_at(p, 1, TOK_POSSESSIVE) || parser_at_word(p, 1, "is")) &&
        parser_at_word(p, 2, "in")) {  // what's in file F; what's in it (after a step that used a file)
        if (parser_at_bare_file(p, 3)) {  // what's in the file (the step before's)
            SourcePos start = parser_token_pos(parser_peek(p, 0));
            parser_consume(p, 3);
            Expr *path = parser_chain_file(p);
            Expr *contents = ast_new_expr(p->arena, EXPR_FILE_CONTENTS, ast_pos_join(start, path->pos));
            contents->as.operand = path;
            return contents;
        }
        if (parser_at_word(p, 3, "file")) return parse_prefixed(p, EXPR_FILE_CONTENTS, 4);
        if (parser_at_word(p, 3, "it") && p->chain_file) return parse_prefixed(p, EXPR_FILE_CONTENTS, 3);
        Span span = parser_consume(p, 3);
        if (parser_error(p, span, "\"%s\" only reads files, so it needs \"file\" before the file's name.",
                         parser_text(p, span))) {
            diag_note(p->diag, "Write it like \"show me what's in file \"notes.txt\"\". To show a list, write \"show me "
                               "scores\".");
            diag_info(p->diag, DIAG_MISSING, "\"file\"", NULL);
            diag_choice(p->diag, "what's in file");
        }
        return parser_error_expr(p);
    }
    if (parser_match_words(p, "length of")) return parse_prefixed(p, EXPR_LENGTH, 2);
    if (parser_match_words(p, "contents of file")) return parse_prefixed(p, EXPR_FILE_CONTENTS, 3);
    if (parser_match_words(p, "contents of")) {
        Span span = parser_consume(p, 2);
        if (parser_error(p, span, "Something is missing after \"%s\".", parser_text(p, span))) {
            diag_note(p->diag, "Did you mean \"contents of file\"?");
        }
        return parser_error_expr(p);
    }
    if (parser_is_operator_word(word)) return expected_value(p);
    if (parser_at_module(p, 0)) return parser_parse_module_value(p, parse_name_or_call);  // helpers's double
    if (parser_at_new(p)) return parser_parse_new(p);
    size_t words;
    if (strcmp(word, "make") == 0 && !parser_match_name(p, 0, false) &&
        (parser_match_kind_name(p, 1, false, &words) || parser_match_variant(p, 1, &words) >= 0)) {
        // "make a circle with radius 3": made like a record
        if (parser_error(p, token->span, "\"make\" doesn't make values; a new value is written with \"new\".")) {
            Span rest = parser_peek(p, 1)->span;
            diag_note(p->diag, "Write it like \"a new %s\".", parser_text(p, rest));
        }
        return parser_error_expr(p);
    }
    Expr *defined = parser_parse_defined_word(p);  // "the best customer", "the loyal customers"
    if (defined) return defined;
    if (parser_at_selection(p)) return parser_parse_selection(p);  // "the numbers in scores where ..."
    const KnownName *known = parser_match_name(p, 0, false);
    if (known && known->words > 1) return parse_name_or_call(p);  // "late fee", before any other meaning
    Expr *library = parser_parse_library_word(p);  // "the square root of X", "file F exists", ...
    if (library) return library;
    if (parser_at_function_value(p)) return parser_parse_function_value(p);
    if (parser_at_call_value(p)) return parser_parse_call_value(p);
    Expr *map = parser_parse_map_word(p);
    if (map) return map;
    Expr *list = parser_parse_list_word(p);
    if (list) return list;
    Expr *text = parser_parse_text_word(p);
    if (text) return text;
    Expr *question = parser_parse_aggregate_word(p);  // "how many", "any", "the total of", "the top 3", ...
    if (question) return question;
    if (parser_at_only(p)) return parser_parse_only(p);
    if (parser_at_every(p)) return reject_inner_every(p);
    if (parser_at_field_across(p)) return parser_parse_field_across(p);  // the names of the drinks
    size_t field = parser_field_words(p, 0);
    if (field > 0 && parser_at_word(p, field, "of")) return parse_field_of(p, field);  // the name of rex

    return parse_name_or_call(p);
}

static Expr *parse_value(Parser *p) {
    if (p->pending_item) {  // "every NAME in L ...": the item comes first
        Expr *item = p->pending_item;
        p->pending_item = NULL;
        return item;
    }
    switch (parser_peek(p, 0)->kind) {
    case TOK_NUMBER: return parser_at_most_common(p) ? parser_parse_most_common(p) : parse_number(p);
    case TOK_MONEY: return parse_money(p);
    case TOK_STRING: return parse_text(p);
    case TOK_LPAREN: return parse_group(p);
    case TOK_WORD: return parse_word(p);
    default: return expected_value(p);
    }
}

// A value and its fields: "rex", "rex's owner's name".
Expr *parse_primary(Parser *p) {
    Expr *expr = parse_value(p);
    while (parser_at(p, 0, TOK_POSSESSIVE) && expr->kind != EXPR_ERROR) expr = parser_parse_field(p, expr);
    return expr;
}
