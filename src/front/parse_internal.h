#ifndef FRONT_PARSE_INTERNAL_H
#define FRONT_PARSE_INTERNAL_H

// Shared by the parser's source files (parse_util.c, parse_expr.c,
// parse_value.c, parse_stmt.c, parse_if.c, parse_loop.c, parse_func.c,
// parse_list.c, parse_kind.c, parse_match.c, parse_text.c, parse_record.c, parse_map.c, parse_names.c).
// Not for use outside src/front/.

#include <stdbool.h>
#include "common/ast.h"
#include "front/lexer.h"

// A name the program introduces somewhere (pre-scan, parse_names.c): every
// function, and every variable or constant whose name is several words.
typedef struct {
    const char *text;  // lowercased, one space between words
    size_t words;
    bool function;
    unsigned intros;   // the words its definitions introduce inputs with, besides "with", "of"
                       // and "using" (which every call can use): bits of parser_intro_bit
    bool method;       // a function "for" a kind of record ("to bark, for a dog:")
} KnownName;

typedef struct {
    Arena *arena;
    Diag *diag;
    const char *source;
    const Token *tokens;
    size_t count;         // tokens[count - 1] is TOK_EOF
    size_t pos;
    Span prev_span;       // the last token or phrase consumed
    bool sentence_failed; // an error was reported; later ones in this sentence are suppressed
    bool ended_with_block;  // the statement just parsed ended with an indented block
    bool ends_with_name;    // ... or ended with a name or keyword ("add 5 to total")
    Vec(SourcePos) open_ifs; // ifs whose blocks are being parsed, innermost last
    bool as_ends_value;     // in a count loop's header, "as" names the loop variable
    Vec(const char *) functions; // every function's name (pre-scan), for suggestions
    Vec(KnownName) names;        // every function, and every name of several words (pre-scan)
    Vec(KnownName) fields;       // every field name of every record kind (pre-scan)
    Records *records;            // the record kinds (pre-scan), filled in with their definitions
    Expr *pending_item;     // "every NAME in L": the item's name, the first value of what follows
    bool method_record;     // parsing a method's record: "rename rex with ..." doesn't call rex
    bool no_call;           // parsing what "call" calls: "call handler with 3" doesn't call handler yet
    size_t lambdas;         // functions made inside values so far (their C names: 0l1, 0l2, ...)
    size_t block_end;       // 1 + the token after a block that ended a value ("a function taking x:"), or 0
    size_t hidden_lists;    // lists sentences like "apply F to every X in L" hold in hidden names so far
    NameList modules;       // the files "use" sentences name ("helpers"), so "helpers's double" names one
} Parser;

// A word or symbol sequence with a meaning, e.g. "is greater than".
typedef struct {
    const char *words;    // space-separated, as the lexer produces them
    const char *display;  // how messages show it ("as a number" for "as number")
    int value;            // e.g. a BinaryOp
    bool needs_operand;   // only matches when a value can follow it
} Phrase;

// --- Tokens ----------------------------------------------------------------

const Token *parser_peek(const Parser *p, size_t ahead);
bool parser_at(const Parser *p, size_t ahead, TokenKind kind);
bool parser_at_word(const Parser *p, size_t ahead, const char *word);
const Token *parser_advance(Parser *p);
// Consumes n tokens and returns their combined span.
Span parser_consume(Parser *p, size_t n);
SourcePos parser_token_pos(const Token *token);
bool parser_at_sentence_end(const Parser *p);

// yes, no, true, false, nothing, it: values, so they can't be names.
bool parser_is_value_word(const char *word);

// True if `token` could begin a value.
bool parser_starts_operand(const Token *token);
// Words with an operator meaning; they can't be used as names.
bool parser_is_operator_word(const char *word);

// --- Phrases ---------------------------------------------------------------

// Number of tokens if the next tokens spell `words`, else 0.
size_t parser_match_words(const Parser *p, const char *words);

// The same, `ahead` tokens from here.
size_t parser_match_words_at(const Parser *p, size_t ahead, const char *words);

// How many leading words of `words` the next tokens spell.
size_t parser_leading_words(const Parser *p, const char *words);

// The longest phrase in the table that the next tokens spell, with its token
// count in *len. If the next tokens are a phrase missing its last word
// ("is greater 5"), reports a "Did you mean" error and returns NULL.
const Phrase *parser_match_phrase(Parser *p, const Phrase *table, size_t n, size_t *len);

// --- Errors ----------------------------------------------------------------

// Reports an error unless one was already reported in this sentence.
// Returns whether it was reported, so the caller knows to add notes.
bool parser_error(Parser *p, Span span, const char *fmt, ...) PRINTF_LIKE(3, 4);

// An EXPR_ERROR node at the next token.
Expr *parser_error_expr(Parser *p);

// The source text of a span (arena copy).
const char *parser_text(const Parser *p, Span span);

// A span quoted for a message: "plus", or the text "Name?" for text.
const char *parser_quoted(const Parser *p, Span span);

// How messages refer to a token: "plus", "the end of the line", ...
const char *parser_describe(const Parser *p, const Token *token);

// Reports whatever is left in a sentence after a complete expression.
void parser_report_leftover(Parser *p);

// The candidates closest to `word` by edit distance, as "a" or "a" or "b";
// NULL if none is close enough.
const char *parser_closest_words(const Parser *p, const char *word, const char *const *candidates, size_t n);

// ".5": reports "write 0.5", consumes both tokens, and returns true.
bool parser_reject_bare_decimal(Parser *p);

// --- Grammar ---------------------------------------------------------------

Expr *parse_expression(Parser *p);
// A single value: a literal, a name, a call, a group, or a prefix form
// ("length of X", "item 2 of X"), with nothing after it.
Expr *parse_primary(Parser *p);
// A value up to a comparison: arithmetic and "followed by".
Expr *parse_join(Parser *p);
// A single value with an optional "-" before it and conversions after it.
Expr *parse_unary(Parser *p);
// The arithmetic or "followed by" operator next, if any (with its token count in *len).
const Phrase *parser_match_arithmetic(Parser *p, size_t *len);
// "every NAME in" (parse_expr.c), and "only L that ..." (only L that are COMPARISON V).
bool parser_at_every(const Parser *p);
bool parser_at_only(const Parser *p);
Expr *parser_parse_only(Parser *p);

// One statement, including a leading "please". NULL after an error. Sets
// ended_with_block and ends_with_name.
Stmt *parse_statement(Parser *p);

// Statements until the end of the file, or (in_block) until and including
// the DEDENT that closes the block.
void parse_statements(Parser *p, Block *block, bool in_block);

// Skips to the end of the line. If an indented block follows, parses and
// discards it, so a broken header doesn't cause errors about its block.
void parser_skip_line_and_block(Parser *p);

// Shared statement helpers (parse_stmt.c), used by parse_if.c and parse_loop.c.
Stmt *parser_new_stmt(Parser *p, StmtKind kind, const Token *verb);
// A variable name. `next` (NULL-terminated, or NULL) lists the words that
// follow the name in this sentence, to explain "let a be 5".
bool parser_parse_name(Parser *p, Name *name, const char *const *next);
// One of `words` (NULL-terminated), or an error showing `example`.
bool parser_expect_word(Parser *p, const char *const *words, const char *example);
// The phrase `words` (without filler), or an error naming `display`.
bool parser_expect_phrase(Parser *p, const char *words, const char *display, const char *example);
// "is", "=", "equals", or "be": the sentence looks like it sets a variable.
bool parser_looks_like_assignment(const Token *next);

// ":" NEWLINE INDENT statements DEDENT. Extends header->span to the colon.
// `if_pos` (or NULL) is the if the block belongs to, for "otherwise" errors;
// `what` names the header in messages ("this \"if\"").
bool parser_parse_block(Parser *p, Block *body, SourcePos *header, const SourcePos *if_pos, const char *what);

// parse_if.c
bool parser_at_otherwise(const Parser *p);  // "otherwise" or "else"
Stmt *parse_if(Parser *p, const Token *verb);
// An "otherwise" that doesn't belong to an if just above it.
void parse_orphan_otherwise(Parser *p, const Stmt *previous);

// Call arguments: single (unary) values separated by "and" (not "and call"),
// and also by commas when `commas` ("NAME using A, B").
void parser_parse_call_args(Parser *p, Expr *call, bool commas);
// After a call's arguments: arithmetic straight after them is ambiguous.
Expr *parser_finish_call(Parser *p, Expr *call);

// parse_stmt.c
bool parser_is_statement_word(const char *word);

// parse_func.c
// "with", "of", "using", or "to", "for", "from" or "in" for a function
// defined with that word.
bool parser_at_call_intro(const Parser *p, const char *function);
// The words that introduce a definition's inputs (NULL-terminated), and the
// bit for one that only calls of a function defined with it can use (0 for
// "with", "of" and "using").
extern const char *const parser_input_intros[];
unsigned parser_intro_bit(const char *word);
Stmt *parse_function(Parser *p, const Token *verb);   // to NAME ...:
// "(a number)" after an input's name, at the "(".
bool parser_parse_param_kind(Parser *p, Type *type);
Stmt *parse_return(Parser *p, const Token *verb);     // give back / return
Stmt *parse_constant(Parser *p, const Token *verb);   // keep NAME as E
// "call NAME ..." (function NULL; at "call") or "NAME ..." (at the function's name).
Stmt *parse_call_statement(Parser *p, const Token *verb, const KnownName *function);

// parse_func_value.c: functions as values
// "function" starts a function value here: "the function double" (a known
// function), or "a function taking x ...", "a function that gives back ...", "a function:".
bool parser_at_function_value(const Parser *p);
Expr *parser_parse_function_value(Parser *p);
// After "call": what's called (a function's name, or any value that holds a
// function) and its inputs. `call` is the EXPR_CALL to fill in.
bool parser_parse_called(Parser *p, Expr *call);
// "call" starts a value here ("let total be call adder with 3").
bool parser_at_call_value(const Parser *p);
Expr *parser_parse_call_value(Parser *p);

// parse_apply.c: built-ins that take a function
// "the numbers in scores where ..." / "... that pass F" / "... combined using F" starts here.
bool parser_at_selection(const Parser *p);
Expr *parser_parse_selection(Parser *p);
// After a value: "applied to every X in L", "combined using F", "sorted by F"; NULL if none is next.
Expr *parser_parse_apply_postfix(Parser *p, Expr *expr);
Stmt *parser_parse_apply(Parser *p, const Token *verb);  // apply F to every X in L
bool parser_at_keep_only(const Parser *p);                // "keep" + "only" (not a constant called "only")
Stmt *parser_parse_keep_only(Parser *p, const Token *verb);
Stmt *parser_finish_sort_by(Parser *p, Stmt *stmt);     // sort L by F, after "by"

// parse_names.c
// Fills p->records and p->fields (every "X is a kind of thing" and every
// "a field, which is"), then p->names and p->functions: every name
// introduced by let, keep, to, as, call, for each and every, and the names
// of record kinds (a method's input is called that).
void parser_find_names(Parser *p);
bool parser_is_method(const Parser *p, const char *name);
// The longest known name (or function, if functions_only) starting `ahead`
// tokens from here, or NULL.
const KnownName *parser_match_name(const Parser *p, size_t ahead, bool functions_only);
bool parser_is_function(const Parser *p, const char *name);
// A word that can be a name, or start one (not an operator or value word).
bool parser_is_name_word(const Token *token);
// Consumes the next `words` tokens as a name.
void parser_take_name(Parser *p, size_t words, Name *name);

// parse_kind.c
// A kind of value: "number" (after the dropped "a"), "text", "yes or no" (or
// "yes/no"), "list of" what it holds, and, when nothing_ok, "nothing".
bool parser_parse_kind(Parser *p, bool nothing_ok, Type *type);
// What a list or map holds, in the plural: "numbers", "lists of text", "dogs", ...
bool parser_parse_items_kind(Parser *p, Type *type);
// The record kind or kind with variants whose name (or, if plural, whose
// plural) the tokens `ahead` spell, with its token count in *words (the
// longest); TYPE_UNKNOWN and 0 words if none.
Type parser_match_kind_name(const Parser *p, size_t ahead, bool plural, size_t *words);
// The variant (Records.variants) whose name the tokens `ahead` spell (the longest), or -1.
int parser_match_variant(const Parser *p, size_t ahead, size_t *words);
// "X is one of:" starts here (found by the pre-scan).
bool parser_at_sum_definition(const Parser *p);
Stmt *parse_sum(Parser *p);  // a shape is one of: (variants indented below)

// parse_iface.c: interfaces
// NAME is anything with: starts at token i (pre-scan); the number of words in NAME, or 0.
size_t parser_interface_words(const Parser *p, size_t i);
Stmt *parse_interface(Parser *p);  // something that can speak is anything with: (ways indented below)

// parse_match.c
// After a value: "is a circle", "is not a circle", "isn't a circle" (a
// variant, written with "a" or "an"); NULL if none is next.
Expr *parser_parse_kind_test(Parser *p, Expr *left);
// check which kind S is: (when it's a circle: ... otherwise: ...)
Stmt *parse_match(Parser *p, const Token *verb);

// parse_list.c
// "every second item", "every 3rd item", "every other item": a slice.
bool parser_at_every_nth(const Parser *p);
// How many tokens a simple value at `ahead` takes (a number, text, a name,
// or a group in parentheses), or 0 if none starts there.
size_t parser_value_length(const Parser *p, size_t ahead);
// "item P", "first item" or "last item", followed by one of `after`.
bool parser_at_item(const Parser *p, const char *const *after);
// A list form starting with a word ("list of", "empty list of", "item",
// "number of items in", "copy of", "biggest of", ...), or NULL if none starts here.
Expr *parser_parse_list_word(Parser *p);
// "add E to the end of L", once "add E to" is parsed and "end of" is next.
Stmt *parser_finish_add_end(Parser *p, Stmt *stmt, Expr *value);
Stmt *parser_parse_set_item(Parser *p, const Token *verb);  // set item P of L to E
Stmt *parser_parse_remove(Parser *p, const Token *verb);
Stmt *parser_parse_insert(Parser *p, const Token *verb);
Stmt *parser_parse_sort(Parser *p, const Token *verb);

// parse_record.c
// "X is a kind of thing" starts here.
bool parser_at_record_definition(const Parser *p);
Stmt *parse_record(Parser *p);  // a dog is a kind of thing with: (fields indented below)
// The record kind whose name (or, if plural, whose plural) the tokens
// `ahead` spell, with its token count in *words; -1 if none.
int parser_match_record(const Parser *p, size_t ahead, bool plural, size_t *words);
// After a value: 's and a field's name ("rex's age"); called at the 's.
Expr *parser_parse_field(Parser *p, Expr *record);
// The longest known field name starting `ahead` tokens from here (0 if none).
size_t parser_known_field_words(const Parser *p, size_t ahead);
// "new" and the name of a record kind or a variant start here.
bool parser_at_new(const Parser *p);
Expr *parser_parse_new(Parser *p);  // a new dog [with name "Rex" and age 3]
// What a sentence changes: a name, a field of one ("rex's age", "it's
// age"), then *field is the EXPR_FIELD, or a value in a map ("the value for
// K in M"), then *field is the EXPR_MAP_GET. `next` is as for parser_parse_name.
bool parser_parse_target(Parser *p, Name *name, Expr **field, const char *const *next);
// A method's inputs: [for] RECORD, then [with | using | of] more inputs.
void parser_parse_method_args(Parser *p, Expr *call);

// parse_map.c
// "map from" (or, if plural, "maps from") starts a kind of map here.
bool parser_at_map_kind(const Parser *p, bool plural);
// map from KEYS to VALUES: "a map from text to numbers".
bool parser_parse_map_kind(Parser *p, Type *type);
// A map form starting with a word ("map of", "empty map from", "value for",
// "keys of", "values of", "number of entries in"), or NULL if none starts here.
Expr *parser_parse_map_word(Parser *p);
// "value for" starts "the value for K in M" here.
bool parser_at_lookup(const Parser *p);
Expr *parser_parse_lookup(Parser *p);

// parse_text.c
// "the characters of T", "the words of T", "the lines of T", "the code of T",
// "the character with code N", "the position of X in T"; NULL if none starts here.
Expr *parser_parse_text_word(Parser *p);
// After a value: "in uppercase", "in lowercase", "without extra spaces",
// "split by X", "joined with X", "with X replaced by Y"; NULL if none is next.
Expr *parser_parse_text_postfix(Parser *p, Expr *expr);
// "L without item N" / "T without character N", at "without" (parse_list.c).
Expr *parser_parse_without_at(Parser *p, Expr *list);
// "with X replaced by" is next (so "with" doesn't start a call's inputs).
bool parser_at_replace(const Parser *p);
// After a value: "is a letter", "is not a digit", "isn't a space"; NULL if none is next.
Expr *parser_parse_char_test(Parser *p, Expr *left);

// parse_module.c: files that use other files
// Fills p->modules from every "use NAME" sentence.
void parser_find_modules(Parser *p);
// tokens[i] ("to") comes straight after "privately" (and maybe a comma) at the start of a line.
bool parser_after_privately(const Parser *p, size_t i);
// "helpers's": a file "use" names, then 's, `ahead` tokens from here.
bool parser_at_module(const Parser *p, size_t ahead);
// "helpers's double with 3" (a value), at "helpers"; `parse_name` parses what's after the 's.
Expr *parser_parse_module_value(Parser *p, Expr *(*parse_name)(Parser *p));
Stmt *parser_parse_module_call(Parser *p);                  // "helpers's greet with "Ann"", at "helpers"
Stmt *parser_parse_use(Parser *p, const Token *verb);        // use helpers
Stmt *parser_parse_privately(Parser *p, const Token *verb);  // privately to ... / privately keep ...

// parse_library.c: the standard library's values
// A value starting with its words ("the square root of", "a random number from", "file F exists", ...); NULL if none.
Expr *parser_parse_library_word(Parser *p);
// After a value: "rounded", "to the power of Y", "shuffled", "days ago", ...; NULL if none is next.
Expr *parser_parse_library_postfix(Parser *p, Expr *expr);
// After a value: "is before E", "is after E", "is in this month", ...; NULL if none is next.
Expr *parser_parse_library_test(Parser *p, Expr *left);
// parse_library_stmt.c: the standard library's sentences
// "make a new file", "rename the file", "copy the file", "move the file", "delete the file", "clear the
// screen", "read csv file", "use the same random numbers": a sentence of the library starts here.
bool parser_at_library_sentence(const Parser *p);
Stmt *parser_parse_library_sentence(Parser *p);
Stmt *parser_parse_wait(Parser *p, const Token *verb);     // wait 2 seconds
Stmt *parser_parse_shuffle(Parser *p, const Token *verb);  // shuffle L
Stmt *parser_parse_save(Parser *p, const Token *verb);     // save L as csv file F
Stmt *parser_parse_put(Parser *p, const Token *verb);      // put X in file F

// parse_loop.c: a loop starting with `verb` (count, go, for, do, repeat,
// while, as, keep, forever), and stop/break/skip/continue/move on.
Stmt *parse_loop(Parser *p, const Token *verb);
Stmt *parse_loop_control(Parser *p, const Token *verb);

#endif
