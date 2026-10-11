// Interfaces: what a value has to be able to do, written as its ways.
//
//   something that can speak is anything with:
//       a way to speak.
//       a way to greet with name (text), giving back text.
//
// Any kind of record (or kind with variants) that has a method with the
// same name, inputs and result for each way counts, without saying so. The
// interface's name is every word before "is anything with", so it can read
// as a phrase ("something that can speak") or a noun ("a speaker"). The
// pre-scan (parse_names.c) finds every interface and the names of its ways
// first, so a way can be called like a method anywhere.

#include <string.h>
#include "front/parse_internal.h"

static const char *const way_example = "a way to greet with name (text), giving back text.";

static bool report_way(Parser *p, Span span, const char *message) {
    if (parser_error(p, span, message, parser_describe(p, parser_peek(p, 0)))) {
        diag_note(p->diag, "Write each way on its own line, like \"%s\"", way_example);
    }
    return false;
}

static bool at_intro(const Parser *p) {
    for (size_t i = 0; parser_input_intros[i]; i++) {
        if (parser_at_word(p, 0, parser_input_intros[i])) return true;
    }
    return false;
}

// NAME (KIND): one input of a way; its kind has to be written.
static bool parse_way_input(Parser *p, Way *way) {
    static const char *const after[] = {"and", "giving", NULL};
    Name param = {0};
    Type declared = TYPE_UNKNOWN;
    if (!parser_parse_name(p, &param, after)) return false;
    if (!parser_at(p, 0, TOK_LPAREN)) {
        if (parser_error(p, param.pos.span, "Say what kind of value \"%s\" is, in parentheses after it.", param.display)) {
            diag_note(p->diag, "A way says the kind of each input, like \"%s (text)\".", param.display);
        }
        return false;
    }
    if (!parser_parse_param_kind(p, &declared)) return false;
    vec_push(p->arena, &way->params, param);
    vec_push(p->arena, &way->declared, declared);
    return true;
}

// way to NAME [with INPUT (KIND) [and | ,] ...] [[,] giving back KIND] (the "a" is dropped)
static bool parse_way(Parser *p, Way *way) {
    static const char *const after_name[] = {"with", "of", "using", "to", "for", "from", "in", "giving", NULL};
    SourcePos start = parser_token_pos(parser_peek(p, 0));
    if (!parser_match_words(p, "way to")) {
        return report_way(p, parser_peek(p, 0)->span, "Each line of an interface is a way, but this starts with %s.");
    }
    parser_consume(p, 2);
    if (!parser_parse_name(p, &way->name, after_name)) return false;
    way->result = TYPE_NOTHING;
    if (at_intro(p)) {
        way->intro = parser_advance(p)->text;
        do {
            if (!parse_way_input(p, way)) return false;
        } while ((parser_at_word(p, 0, "and") || (parser_at(p, 0, TOK_COMMA) && !parser_at_word(p, 1, "giving"))) &&
                 parser_advance(p));
    }
    size_t skip = parser_at(p, 0, TOK_COMMA) && parser_at_word(p, 1, "giving") ? 1 : 0;
    if (parser_at_word(p, skip, "giving") && parser_at_word(p, skip + 1, "back")) {
        parser_consume(p, skip + 2);
        if (!parser_parse_kind(p, true, &way->result)) return false;
    }
    way->pos = ast_pos_join(start, (SourcePos){p->prev_span, 0, 0, 0});
    if (parser_at_sentence_end(p)) return true;
    return report_way(p, parser_peek(p, 0)->span, "I expected the way to end here, but found %s.");
}

// The indented lines after the colon, one way per sentence, up to and
// including the DEDENT that ends them.
static void parse_ways(Parser *p, Stmt *stmt) {
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
        if (kind == TOK_INDENT) {  // reported, then skipped with its block
            const Token *indent = parser_advance(p);
            if (parser_error(p, indent->span, "This line is indented more than the ways above it.")) {
                diag_note(p->diag, "Line the ways up with each other.");
            }
            for (size_t depth = 1; depth > 0 && !parser_at(p, 0, TOK_EOF);) {
                TokenKind next = parser_advance(p)->kind;
                depth += next == TOK_INDENT ? 1 : next == TOK_DEDENT ? (size_t)-1 : 0;
            }
            p->sentence_failed = false;
            continue;
        }
        Way way = {0};
        if (parse_way(p, &way)) vec_push(p->arena, &stmt->as.iface.ways, way);
        while (!parser_at_sentence_end(p)) parser_advance(p);
        p->sentence_failed = false;
    }
}

Stmt *parse_interface(Parser *p) {
    size_t words = parser_interface_words(p, p->pos);
    Stmt *stmt = parser_new_stmt(p, STMT_INTERFACE, parser_peek(p, 0));
    parser_take_name(p, words, &stmt->as.iface.name);
    for (size_t i = 0; i < p->records->interfaces.len; i++) {
        if (strcmp(p->records->interfaces.items[i].name, stmt->as.iface.name.text) == 0) stmt->as.iface.index = i;
    }
    parser_consume(p, 4);  // is anything with :
    stmt->pos = ast_pos_join(stmt->pos, (SourcePos){p->prev_span, 0, 0, 0});
    p->ended_with_block = true;
    if (!parser_at(p, 0, TOK_NEWLINE) || !parser_at(p, 1, TOK_INDENT)) {
        Span span = parser_at(p, 0, TOK_NEWLINE) ? p->prev_span : parser_peek(p, 0)->span;
        if (parser_error(p, span, "The ways of \"%s\" go on the lines below, indented.", stmt->as.iface.name.display)) {
            diag_note(p->diag, "Write each one on its own line, like \"    a way to speak.\"");
        }
        parser_skip_line_and_block(p);
        p->ended_with_block = true;
        return NULL;
    }
    parser_consume(p, 2);
    parse_ways(p, stmt);
    p->ended_with_block = true;
    RecordKind *kind = &p->records->interfaces.items[stmt->as.iface.index];
    if (!kind->definition) kind->definition = stmt;
    return stmt;
}
