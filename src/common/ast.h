#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>
#include "common/arena.h"
#include "common/diag.h"
#include "common/util.h"
#include "common/type.h"
#include "common/ast_ops.h"

// Where a node came from. `span` covers the whole node as written (including
// surrounding parentheses); line and column (1-based, counted in characters)
// are those of its first token.
typedef struct {
    Span span;
    size_t line;
    size_t column;
    unsigned file;  // which file of the program (0 when it has one), with `line` counted in that file
} SourcePos;

// --- Expressions -----------------------------------------------------------

typedef enum {
    EXPR_ERROR,          // stands in for something that failed to parse
    EXPR_NUMBER,
    EXPR_TEXT,
    EXPR_BOOLEAN,        // yes/no/true/false
    EXPR_NOTHING,
    EXPR_IT,
    EXPR_NAME,           // a variable, or a function called with no inputs
    EXPR_CALL,           // NAME of A and B / NAME with A and B / NAME using A, B
    EXPR_LENGTH,         // length of X
    EXPR_FILE_CONTENTS,  // contents of file X
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_CONVERT,        // X as a number / X as text
    EXPR_LIST,           // a list of A, B and C
    EXPR_EMPTY_LIST,     // an empty list of numbers
    EXPR_ITEM,           // item N of L / the first item in L / the last item in L
    EXPR_LIST_OP,        // L added together, the biggest of L, L reversed, L is empty, a copy of L, ...
    EXPR_EVERY,          // every NAME in L <rest of the expression, worked out for each item>
    EXPR_ONLY,           // only L that are <comparison>
    EXPR_SLICE,          // items 2 to 4 of L, the first 3 items in L, L without the last item, ...
    EXPR_TEXT_OP,        // the words of T, T in uppercase, T split by ",", the position of X in T, ...
    EXPR_FIELD,          // R's NAME (a field of a record)
    EXPR_NEW,            // a new dog with name "Rex" and age 3
    EXPR_MAP,            // a map of "coffee" to 3, "tea" to 2
    EXPR_EMPTY_MAP,      // an empty map from text to numbers
    EXPR_MAP_GET,        // the value for K in M [, or D]
    EXPR_MAP_WITHOUT,    // M without K: a new map (as.lookup: key and map, no fallback)
    EXPR_KIND_TEST,      // X is a circle / X is not a circle (a variant of a kind with variants)
    EXPR_FUNCTION_REF,   // the function double (a function defined with "to", not called)
    EXPR_LAMBDA,         // a function taking x that gives back x times 2 / a function taking word: (a block)
    EXPR_APPLY,          // F applied to every X in L, the Xs in L that pass F, L combined using F, L sorted by F
    EXPR_LIBRARY,        // the standard library's values: the square root of X, a random number from A to B, today, ...
} ExprKind;

typedef struct Expr Expr;

typedef struct FieldValue FieldValue;

// A name written in the program, e.g. a variable. A name can be several
// words ("hourly rate"); `text` is how it's matched (lowercased, one space
// between words) and `display` how it was written here ("Hourly Rate",
// "clientName"), for messages.
typedef struct {
    const char *text;
    const char *display;
    SourcePos pos;
} Name;

// "name "Rex"" in "a new dog with name "Rex" and age 3".
struct FieldValue {
    Name field;
    Expr *value;
    size_t index;  // filled in by the checker: the field's place in the definition
};

struct Expr {
    ExprKind kind;
    SourcePos pos;
    Type type;            // filled in by the checker
    bool narrowed;        // filled in by the checker: "a dog or nothing" checked not to be nothing here
    bool lifted;          // filled in by the checker: given where "<its kind> or nothing" is kept
    const char *display;  // EXPR_NAME, EXPR_CALL: the name as written here, for messages
    const char *module;   // EXPR_NAME, EXPR_CALL: "helpers's double": the file it's from ("helpers"), or NULL
    union {
        struct {
            const char *text;  // as written, e.g. "19.99"
            bool is_decimal;
        } number;
        struct {
            const char *value;  // decoded; may contain any byte but NUL
            size_t len;
        } text;
        bool boolean;
        const char *name;  // EXPR_NAME: matched lowercased, one space between words (see Name)
        struct {
            const char *name;
            Vec(Expr *) args;
            const struct Stmt *target;  // the definition called (set by the checker)
            Expr *callee;  // a function kept as a value ("call f with 3", "f with 3"): what's called. Set by
                           // the parser when it isn't a name ("call item 1 of handlers"), by the checker for a
                           // name that's a variable; name is then unused. NULL for a function defined with "to".
            bool dispatch; // set by the checker: a method called on a value of an interface (picked while running)
        } call;
        Expr *operand;  // EXPR_LENGTH, EXPR_FILE_CONTENTS
        struct {
            UnaryOp op;
            Span op_span;
            Expr *operand;
        } unary;
        struct {
            BinaryOp op;
            Span op_span;  // the operator as written, e.g. "is greater than"
            Expr *left;
            Expr *right;
        } binary;
        struct {
            ConvertTarget target;
            Span op_span;
            Expr *operand;
        } convert;
        Vec(Expr *) items;  // EXPR_LIST, in order (at least one); EXPR_MAP: key, value, key, value, ...
        Type list_type;     // EXPR_EMPTY_LIST, EXPR_EMPTY_MAP: its type, e.g. a list of numbers
        struct {
            ItemWhere where;
            Expr *position;   // ITEM_AT; NULL for the first and last item
            Expr *list;       // the list, or the text for a character
            bool characters;  // "character 2 of T": a character of text, not an item of a list
        } item;
        struct {
            ListOp op;
            Span op_span;
            Expr *operand;
        } list_op;
        struct {
            Name var;    // the name each item gets
            Expr *list;
            Expr *body;  // worked out for each item; starts with the item's name
        } every;
        struct {
            Expr *list;
            Expr *condition;  // a comparison whose left side is EXPR_IT (the item)
        } only;
        struct {
            SliceKind kind;
            Expr *list;
            Expr *from;   // SLICE_RANGE: the first position; SLICE_FIRST/SLICE_LAST: how many
            Expr *to;     // SLICE_RANGE: the last position, or NULL for the end
            size_t step;   // SLICE_EVERY: 2 for "every second item" and "every other item"
            size_t start;  // SLICE_EVERY: the first position taken (STEP, or 1 for "every other item")
            bool characters;  // "characters 2 to 4 of T": characters of text, not items of a list
        } slice;
        struct {
            TextOp op;
            Span op_span;
            Expr *operand;  // the text (the list for TEXT_JOINED, the number for TEXT_FROM_CODE)
            Expr *arg;      // TEXT_SPLIT, TEXT_JOINED: the separator; TEXT_REPLACE: what to look for;
                            // TEXT_POSITION: what to find
            Expr *arg2;     // TEXT_REPLACE: what to put instead
            bool negated;   // TEXT_IS_*: "is not a letter", "isn't a digit"
        } text_op;
        struct {
            Expr *record;  // whose field it is
            Name name;     // the field's name
            size_t index;  // filled in by the checker: its place in the definition
            size_t variant;  // filled in by the checker for a kind with variants: the variant it's checked
                             // to be (Records.variants)
        } field;
        struct {
            Type type;               // the record kind, or the kind with variants
            size_t variant;          // for a kind with variants: which (Records.variants)
            Vec(FieldValue) values;  // as written, in order
        } new_record;
        struct {
            Expr *operand;
            size_t variant;  // Records.variants
            bool negated;    // "is not a circle", "isn't a circle"
            Span op_span;    // "is a circle"
        } kind_test;
        struct {
            Expr *key;
            Expr *map;
            Expr *fallback;  // ", or D": the value when the key isn't there (NULL: that's an error)
        } lookup;  // EXPR_MAP_GET, EXPR_MAP_WITHOUT
        struct {
            const char *name;
            const struct Stmt *target;  // the definition (set by the checker)
        } ref;  // EXPR_FUNCTION_REF
        struct Stmt *lambda;  // EXPR_LAMBDA: a STMT_FUNCTION (function.lambda set) holding its inputs and body
        struct {
            LibraryOp op;
            Span op_span;  // the phrase as written ("rounded up", "is before")
            Expr *a, *b;   // its parts, in the order they're written (NULL when it has fewer)
            int part;      // LIB_DATE_PART: a DatePart; LIB_IN_PERIOD: a Period
        } library;
        struct {
            ApplyKind kind;
            Expr *function;   // a value holding a function (a function's name means the function itself)
            Expr *list;
            Expr *start;      // APPLY_COMBINE: ", starting from S" (NULL: the first item)
            bool descending;  // APPLY_SORT: "biggest first"
        } apply;
    } as;
};

// --- Statements ------------------------------------------------------------

typedef enum {
    STMT_LET,          // let X be E                 (creates X)
    STMT_SET,          // set X to E / change X to E (changes X)
    STMT_CHANGE,       // add/subtract/increase/decrease/multiply/divide
    STMT_SAY,          // say/print/show/display/write E
    STMT_ASK,          // ask E and call the answer X
    STMT_WRITE_FILE,   // write E to file F
    STMT_APPEND_FILE,  // append E to file F
    STMT_READ_FILE,    // read file F and call it X
    STMT_STOP,         // stop the program
    STMT_IF,           // if ... / otherwise if ... / otherwise
    STMT_LOOP,         // count / repeat / while / until / forever
    STMT_BREAK,        // stop the loop
    STMT_CONTINUE,     // skip this one
    STMT_FUNCTION,     // to NAME [with] P and Q:  (top level only)
    STMT_RETURN,       // give back E / return E / return
    STMT_CALL,         // greet "Paris" / greet with "Paris" / call greet with "Paris"
    STMT_CONSTANT,     // keep NAME as E / keep NAME at E  (top level only)
    STMT_LIST,         // add E to the end of L, insert, remove, set item N of L to E, sort
    STMT_RECORD,       // a dog is a kind of thing with: (fields indented below; top level only)
    STMT_SUM,          // a shape is one of: (variants indented below; top level only)
    STMT_MATCH,        // check which kind S is: (when it's a circle: ... otherwise: ...)
    STMT_INTERFACE,    // something that can speak is anything with: (ways indented below; top level only)
    STMT_USE,          // use helpers (another file's definitions; loaded before parsing)
    STMT_LIBRARY,      // the standard library's sentences: wait 2 seconds, shuffle L, delete the file F, ...
} StmtKind;

// What a sentence of the standard library does (STMT_LIBRARY), with its parts in a and b.
typedef enum {
    DO_WAIT,           // wait A seconds (a is in seconds: the parser scales minutes and milliseconds)
    DO_SAME_RANDOM,    // use the same random numbers every time
    DO_SHUFFLE,        // shuffle A
    DO_CLEAR_SCREEN,   // clear the screen
    DO_MAKE_FILE,      // make a new file called A
    DO_MAKE_FOLDER,    // make a folder called A
    DO_RENAME,         // rename the file (or folder) A to B
    DO_COPY,           // copy the file A to B
    DO_MOVE,           // move the file (or folder) A to the folder B
    DO_DELETE_FILE,    // delete the file A
    DO_DELETE_FOLDER,  // delete the folder A [and everything in it: `everything`]
    DO_READ_CSV,       // read csv file A and call it NAME
    DO_SAVE_CSV,       // save A as csv file B
    DO_COMPLAIN,       // complain A (to the error output)
    DO_RUN,            // run A [with parts...] [and call the result NAME]
    DO_WAIT_KEY,       // wait for a key and call it NAME
} LibraryAction;

// A constant's value, worked out by the compiler (see front/consteval.h).
typedef enum {
    CONST_NUMBER,
    CONST_TEXT,
    CONST_YESNO,
    CONST_LIST,  // a fixed list ("keep weekdays as a list of ..."): numbers, text or yes/no, never lists
    CONST_MAP,   // a fixed map ("keep prices as a map of ..."): text or number keys; numbers, text or yes/no values
} ConstKind;

typedef struct ConstValue ConstValue;
struct ConstValue {
    ConstKind kind;
    double number;
    bool yes;
    const char *text;  // NUL-terminated
    size_t len;        // CONST_TEXT: bytes; CONST_LIST: items; CONST_MAP: entries
    Type list_type;    // CONST_LIST: e.g. a list of text
    ConstValue *items; // CONST_LIST; CONST_MAP: the values
    ConstValue *keys;  // CONST_MAP, in order
    Type key_type;     // CONST_MAP: TYPE_TEXT or TYPE_NUMBER
    Type value_type;   // CONST_MAP: TYPE_NUMBER, TYPE_TEXT or TYPE_YESNO
};

// One field of a record kind: "an age, which is a number, starting at 0."
typedef struct {
    Name name;
    Type type;      // its kind, as written
    Expr *initial;  // "starting at E", or NULL
    SourcePos pos;
    ConstValue folded;  // filled in by the checker: the starting value, worked out (unless it's "nothing")
} Field;

typedef enum {
    LOOP_COUNT,    // count [down] from A to B [by S] [as N]; go from; for each N from
    LOOP_TIMES,    // do this N times / repeat N times ("it" is the round, from 1)
    LOOP_WHILE,    // while C / as long as C / repeat while C
    LOOP_UNTIL,    // keep doing this until C / repeat until C (checked before each round)
    LOOP_FOREVER,  // forever
    LOOP_EACH,     // for each N in L ("it" is the item too)
} LoopKind;

typedef enum {
    CHANGE_ADD,       // add E to X, increase X by E
    CHANGE_SUBTRACT,  // subtract E from X, decrease X by E
    CHANGE_MULTIPLY,  // multiply X by E
    CHANGE_DIVIDE,    // divide X by E
} ChangeOp;

typedef enum {
    LIST_ADD_END,   // add E to the end of L
    LIST_INSERT,    // insert E at position N of L
    LIST_REMOVE,    // remove item N from L (or the first or last item)
    LIST_SET_ITEM,  // set item N of L to E (or the first or last item)
    LIST_SORT,      // sort L [by their length] [, biggest/smallest/longest/shortest first]
    LIST_REMOVE_KEY,  // remove K from M (M a map; K is `value`, M is `list`)
    LIST_APPLY,     // apply F to every X in L: `value` is the new list (an EXPR_APPLY reading `hidden`)
    LIST_KEEP,      // keep only the Xs in L where C / that pass F: `value` is EXPR_ONLY or EXPR_APPLY
    LIST_SORT_BY,   // sort L by F [, biggest first]: `value` is an EXPR_APPLY
} ListChange;

// A block is a list of statements, in order. A program is a block.
typedef Vec(Stmt *) Block;

// One variant in the definition of a kind with variants: "a rectangle with
// a width (a number) and a height (a number)".
// Names, as matched (a named type, so a pointer to one can be a parameter:
// an anonymous Vec(...) in a parameter list is a different type everywhere).
typedef Vec(const char *) NameList;

// A name a function made inside another uses from around it ("count" in a
// counter): it's shared, kept in a cell (es_closure.h).
typedef struct {
    const char *name;
    Type type;   // filled in by the checker
    size_t var;  // the checker's (its type variable)
} Capture;

typedef struct {
    Name name;
    size_t id;          // in Records.variants
    Vec(Field) fields;  // in order; none have starting values
    SourcePos pos;
} VariantDef;

// One way of an interface: "a way to greet with name (text), giving back text."
// A kind of record (or with variants) has it when it has a method with the
// same name, inputs and result.
typedef struct {
    Name name;           // "greet"
    const char *intro;   // the word before its inputs ("with"), or NULL
    Vec(Name) params;
    Vec(Type) declared;  // each input's kind (always written)
    Type result;         // TYPE_NOTHING when it gives back nothing
    SourcePos pos;
} Way;

// One part of "check which kind S is": "when it's a circle or a square:",
// "when it's nothing:", or a final "otherwise:".
typedef struct {
    Vec(size_t) variants;  // Records.variants
    bool nothing;          // "when it's nothing" (alone, or with variants)
    bool otherwise;
    Block body;
    SourcePos pos;  // from "when"/"otherwise" to its colon
} MatchBranch;

// One branch of an if: "if C", "otherwise if C", or a final "otherwise"
// (condition NULL).
typedef struct {
    Expr *condition;
    Block body;
    SourcePos pos;  // the branch's first line, from "if"/"otherwise" to its colon
    bool unless;    // "unless C": the branch runs when C is no
} IfBranch;

struct Stmt {
    StmtKind kind;
    SourcePos pos;  // the whole statement, without its period
    Span verb;      // the word that chose the statement as written, e.g. "display"
    bool stop_here; // STMT_RETURN written "stop here"
    union {
        struct {
            Name name;
            Expr *value;
            Expr *field;  // STMT_SET of a field ("set rex's age to 4"): an EXPR_FIELD; name is then unused.
                          // Or a map's value ("set the value for K in M to 4"): an EXPR_MAP_GET
            Type type;    // filled in by the checker: the variable's kind
        } assign;  // STMT_LET, STMT_SET
        struct {
            ChangeOp op;
            Name target;
            Expr *amount;
            Expr *field;  // a field ("add 1 to rex's age"): an EXPR_FIELD (or a map's value: an
                          // EXPR_MAP_GET); target is then unused
            bool append;  // filled in by the checker: "add E to L" where L is a list
        } change;
        Expr *value;  // STMT_SAY
        struct {
            Expr *prompt;
            Name answer;
        } ask;
        struct {
            Expr *text;
            Expr *path;
        } file_write;  // STMT_WRITE_FILE, STMT_APPEND_FILE
        struct {
            Expr *path;
            Name name;
        } read_file;
        struct {
            Vec(IfBranch) branches;  // the "if" first, then each "otherwise"
            bool one_line;           // "if C, S." or "if C then S."
            bool trailing;           // "give back x unless C.": the condition after the sentence (one_line too)
        } if_stmt;
        struct {
            Name name;
            const char *intro;     // the word before the inputs: "with", "of", "using" or "to" (NULL: none)
            bool method;           // "to bark, for a dog:": the first input is the dog
            size_t overload;       // a method sharing its name with another kind's: 1 + its kind's index (checker)
            Vec(Name) params;
            Vec(Type) declared;    // per input: the kind written in "(a number)", or TYPE_UNKNOWN
            Type declared_result;  // "giving back a number", or TYPE_UNKNOWN
            Vec(Type) param_types; // filled in by the checker
            Type result;           // filled in by the checker (TYPE_NOTHING: gives back no value)
            Block body;
            bool private;          // "privately to ...": only its own file can use it
            bool lambda;           // made inside a value ("a function taking x ..."); name.text is "0lN"
            Vec(Capture) captures; // a lambda: the names it uses from around it (checker)
            NameList cells;           // the names it makes (or its inputs) that functions made inside use (checker)
        } function;
        Expr *returned;  // STMT_RETURN; NULL for a bare "return"
        Expr *call;      // STMT_CALL: an EXPR_CALL
        struct {
            Name name;
            Expr *value;
            ConstValue folded;  // filled in by the checker
            bool private;       // "privately keep ...": only its own file can use it
        } constant;
        Name module;  // STMT_USE: the file's name without ".es"
        struct {
            LibraryAction op;
            Expr *a, *b;
            double scale;     // DO_WAIT: seconds per unit written (60 for minutes, 0.001 for milliseconds)
            bool everything;  // DO_DELETE_FOLDER: "and everything in it"
            bool folder;      // DO_RENAME, DO_MOVE: "the folder A" rather than "the file A"
            Name name;        // DO_READ_CSV, DO_RUN, DO_WAIT_KEY: the name made (text NULL: none)
            Type type;        // DO_SAVE_CSV, DO_RUN: filled in by the checker: the list's kind, the parts' kinds
            Vec(Expr *) parts;  // DO_RUN: the words after "with", each text, a number or a list of text
        } library;
        Expr *stop_code;  // STMT_STOP: "with code N" or "saying it failed" (1); NULL: 0
        struct {
            LoopKind kind;
            Expr *from, *to, *step;  // LOOP_COUNT (step NULL: 1)
            bool down;               // "count down": never counts up
            Name var;                // LOOP_COUNT: "number" unless named; LOOP_EACH: each item (a map's key)
            bool var_named;          // the program chose the name (otherwise it's "number")
            Name value_name;         // LOOP_EACH over a map: each value ("for each name and age in ages")
            bool pair;               // LOOP_EACH with two names (value_name is set)
            Expr *times;             // LOOP_TIMES
            Expr *condition;         // LOOP_WHILE, LOOP_UNTIL
            Expr *list;              // LOOP_EACH: the list (or map)
            Block body;
        } loop;
        struct {
            ListChange op;
            Expr *list;
            Expr *value;      // LIST_ADD_END, LIST_INSERT, LIST_SET_ITEM
            ItemWhere where;  // LIST_REMOVE, LIST_SET_ITEM
            Expr *position;   // LIST_INSERT, and ITEM_AT for LIST_REMOVE, LIST_SET_ITEM
            bool by_length;   // LIST_SORT: "by their length"
            bool descending;  // LIST_SORT: "biggest first" / "longest first"
            Expr *hidden;     // LIST_APPLY, LIST_KEEP, LIST_SORT_BY: an EXPR_NAME ("0listN") holding the list
                              // while `value` works out its new items, which then replace the list's
        } list;
        struct {
            Name name;
            size_t index;      // in the program's Records
            Vec(Field) fields;
        } record;
        struct {
            Name name;
            size_t index;  // in Records.sums
            Vec(VariantDef) variants;
        } sum;
        struct {
            Expr *subject;
            Vec(MatchBranch) branches;  // the "when"s in order, then an "otherwise" if there is one
        } match;
        struct {
            Name name;
            size_t index;  // in Records.interfaces
            Vec(Way) ways;
        } iface;
    } as;
};

// One file of a program made of several (modules): "helpers.es" is used
// with "use helpers". The program's statements are all its files', each
// statement's pos.file saying which. The file that's run is the last.
typedef struct {
    const char *name;     // as "use" names it: "helpers" (lowercased)
    const char *display;  // "helpers.es"
    Vec(size_t) uses;     // the files it uses
} SourceFile;

typedef Vec(SourceFile) SourceFiles;

// A parsed program: its statements, and the record kinds it defines.
typedef struct {
    Block body;
    SourceFiles files;  // empty for a program of one file
    Records records;
    NameList cells;           // names the main program makes that functions made inside it use (checker)
} Program;

// --- Helpers ---------------------------------------------------------------

// New zeroed nodes from the arena.
Expr *ast_new_expr(Arena *arena, ExprKind kind, SourcePos pos);
Stmt *ast_new_stmt(Arena *arena, StmtKind kind, SourcePos pos);

// Position spanning from the start of `first` to the end of `last`.
SourcePos ast_pos_join(SourcePos first, SourcePos last);

const char *ast_binary_op_name(BinaryOp op);
const char *ast_unary_op_name(UnaryOp op);
const char *ast_change_op_name(ChangeOp op);
const char *ast_list_op_name(ListOp op);
const char *ast_text_op_name(TextOp op);
// common/library.c
const char *ast_library_op_name(LibraryOp op);
const char *ast_library_action_name(LibraryAction op);
const char *ast_date_part_name(int part);  // "year", ...
const char *ast_period_name(int period);   // "this month", ...

// Indented outline, one node per line: "label [line:column]", children
// indented two spaces. Statement children come in source order (for files:
// the text, then the file). Used by `easyscript ast`.
void ast_dump_program(const Program *program, StrBuf *out);

#endif
