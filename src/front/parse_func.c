// Top-level definitions: functions (defining, giving back, calling as a
// statement) and constants ("keep rate as 0.2").
//
//   to make tea:                         give back E / return E / return
//   to greet with someone:               ("of", "using" and "to" work like "with")
//   to area with width and height:       to add tax to amount:
//   to area with width (a number) and height (a number), giving back a number:
//   to average of scores (a list of numbers), giving back a number:
//                                        (the kinds are optional; the checker
//                                        works them out when they're left out)
//
//   greet "Paris".   greet with "Paris".   call greet with "Paris".   add tax to 100.
//
// A function's name is every word up to the word that introduces its inputs
// (with, of, using, to), so it can be several words ("add tax"); "to" calls
// it only if the definition used "to". Calls inside expressions ("area of 3
// and 4") are in parse_value.c. The pre-scan (parse_names.c) finds every
// "to NAME" at the start of a line first, so a sentence can call a function
// that's defined further down.

#include <string.h>
#include "front/parse_internal.h"

// --- Defining ------------------------------------------------------------------

// A one-word function name can't be a sentence word; "add tax" can.
static bool reject_statement_word_name(Parser *p, const Name *name) {
    if (!parser_is_statement_word(name->text)) return false;
    if (parser_error(p, name->pos.span, "\"%s\" starts sentences in EasyScript, so it can't be the name of a function.",
                     name->text)) {
        diag_note(p->diag, "Pick another name, like \"%s_it\".", name->text);
    }
    return true;
}

static bool report_missing_input(Parser *p) {
    if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
        diag_note(p->diag, "Name the inputs, like \"to area with width and height:\", or leave out \"%s\".",
                  parser_text(p, p->prev_span));
    }
    return false;
}

// --- Kinds of values (parser_parse_kind is in parse_list.c) ----------------------

static bool at_giving_back(const Parser *p) {
    return parser_at_word(p, 0, "giving") && parser_at_word(p, 1, "back");
}

// "(a number)" after an input's name.
static bool parse_param_kind(Parser *p, Type *type) {
    parser_advance(p);  // (
    if (!parser_parse_kind(p, false, type)) return false;
    if (parser_at(p, 0, TOK_RPAREN)) {
        parser_advance(p);
        return true;
    }
    if (parser_error(p, parser_peek(p, 0)->span, "I expected \")\" after the kind of value, but found %s.",
                     parser_describe(p, parser_peek(p, 0)))) {
        diag_note(p->diag, "Write the kind in parentheses after the input's name, like \"width (a number)\".");
    }
    return false;
}

// [with | using] P [(KIND)] [and | ,] Q [(KIND)] ... [, giving back KIND] up to the colon.
static bool parse_params(Parser *p, Stmt *stmt) {
    static const char *const after_param[] = {"and", "giving", NULL};
    bool introduced = parser_at_word(p, 0, "with") || parser_at_word(p, 0, "using") || parser_at_word(p, 0, "of") ||
                      parser_at_word(p, 0, "to");
    if (introduced) stmt->as.function.intro = parser_advance(p)->text;
    const Token *next = parser_peek(p, 0);
    if (!introduced && next->kind == TOK_LPAREN) {  // "to greet who (text):"
        if (parser_error(p, next->span, "A kind in parentheses goes after an input, and inputs come after \"with\".")) {
            const char *name = stmt->as.function.name.display;
            const char *space = strchr(name, ' ');  // "greet who": the input was probably "who"
            if (space) {
                diag_note(p->diag, "Write it like \"to %.*s with %s (text):\".", (int)(space - name), name, space + 1);
            } else {
                diag_note(p->diag, "Write it like \"to %s with someone (text):\".", name);
            }
        }
        return false;
    }
    if (next->kind == TOK_COLON && next->filler.length > 0) {
        Name ignored;
        return parser_parse_name(p, &ignored, NULL);  // "to greet a:": explains that "a" is filler
    }
    if (introduced && (next->kind == TOK_COLON || parser_at_sentence_end(p))) return report_missing_input(p);
    if (!introduced && parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "giving")) parser_advance(p);  // no inputs
    while (!parser_at(p, 0, TOK_COLON) && !parser_at_sentence_end(p) && !at_giving_back(p)) {
        Name param = {0};
        Type declared = TYPE_UNKNOWN;
        if (!parser_parse_name(p, &param, after_param)) return false;
        if (parser_at(p, 0, TOK_LPAREN) && !parse_param_kind(p, &declared)) return false;
        vec_push(p->arena, &stmt->as.function.params, param);
        vec_push(p->arena, &stmt->as.function.declared, declared);
        if (!parser_at_word(p, 0, "and") && !parser_at(p, 0, TOK_COMMA)) break;
        parser_advance(p);
    }
    if (at_giving_back(p)) {
        parser_consume(p, 2);
        return parser_parse_kind(p, true, &stmt->as.function.declared_result);
    }
    return true;
}

Stmt *parse_function(Parser *p, const Token *verb) {
    static const char *const after_name[] = {"with", "using", "of", "to", "giving", NULL};
    Stmt *stmt = parser_new_stmt(p, STMT_FUNCTION, verb);
    bool ok = parser_parse_name(p, &stmt->as.function.name, after_name) &&
              (strchr(stmt->as.function.name.text, ' ') || !reject_statement_word_name(p, &stmt->as.function.name)) &&
              parse_params(p, stmt);
    if (!ok) {
        parser_skip_line_and_block(p);
    } else {
        SourcePos header = stmt->pos;
        ok = parser_parse_block(p, &stmt->as.function.body, &header, NULL, "this function");
    }
    p->ended_with_block = true;  // set last: statements inside the block reset it
    return ok ? stmt : NULL;
}

// --- Constants -------------------------------------------------------------------

// keep NAME as E / keep NAME at E. Whether E is fixed before the program runs
// is checked (and E worked out) by the checker.
Stmt *parse_constant(Parser *p, const Token *verb) {
    static const char *const as_at[] = {"as", "at", NULL};
    static const char *const example = "keep tax_rate as 0.2";
    Stmt *stmt = parser_new_stmt(p, STMT_CONSTANT, verb);
    if (!parser_parse_name(p, &stmt->as.constant.name, as_at) || !parser_expect_word(p, as_at, example)) return NULL;
    stmt->as.constant.value = parse_expression(p);
    return stmt;
}

// --- Giving back -----------------------------------------------------------------

Stmt *parse_return(Parser *p, const Token *verb) {
    static const char *const back[] = {"back", NULL};
    if (strcmp(verb->text, "give") == 0 && !parser_expect_word(p, back, "give back total")) return NULL;
    Stmt *stmt = parser_new_stmt(p, STMT_RETURN, verb);
    if (!parser_at_sentence_end(p)) stmt->as.returned = parse_expression(p);
    if (stmt->as.returned && stmt->as.returned->kind == EXPR_NOTHING) stmt->as.returned = NULL;  // = "return"
    return stmt;
}

// --- Calling as a statement --------------------------------------------------------

bool parser_at_call_intro(const Parser *p, const char *function) {
    if (parser_at_word(p, 0, "with") || parser_at_word(p, 0, "using") || parser_at_word(p, 0, "of")) return true;
    if (!parser_at_word(p, 0, "to")) return false;
    for (size_t i = 0; i < p->names.len; i++) {
        if (strcmp(p->names.items[i].text, function) == 0) return p->names.items[i].to_inputs;
    }
    return false;
}

Stmt *parse_call_statement(Parser *p, const Token *verb, const KnownName *function) {
    static const char *const after_name[] = {"with", "using", "of", "to", NULL};
    Name name = {0};
    if (function) {
        parser_take_name(p, function->words, &name);  // the function's name starts the sentence
    } else if (!parser_parse_name(p, &name, after_name)) {  // after "call"
        return NULL;
    }
    Stmt *stmt = parser_new_stmt(p, STMT_CALL, verb);
    Expr *call = ast_new_expr(p->arena, EXPR_CALL, name.pos);
    call->as.call.name = name.text;
    call->display = name.display;
    bool commas = false;
    bool introduced = parser_at_call_intro(p, name.text);
    if (introduced) {
        commas = parser_at_word(p, 0, "using");
        parser_advance(p);
    }
    if (introduced && parser_at_sentence_end(p)) {
        if (parser_error(p, p->prev_span, "Something is missing after %s.", parser_quoted(p, p->prev_span))) {
            diag_note(p->diag, "Add the values to give it, like \"greet with \"Paris\"\", or leave out \"%s\".",
                      parser_text(p, p->prev_span));
        }
        return NULL;
    }
    if (!parser_at_sentence_end(p)) {
        parser_parse_call_args(p, call, commas);
        call = parser_finish_call(p, call);
    }
    stmt->as.call = call;
    return stmt;
}
