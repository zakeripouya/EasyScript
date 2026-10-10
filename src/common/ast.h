#ifndef AST_H
#define AST_H

#include <stdbool.h>
#include <stddef.h>
#include "common/arena.h"
#include "common/diag.h"
#include "common/util.h"

// Where a node came from. `span` covers the whole node as written (including
// surrounding parentheses); line and column (1-based, counted in characters)
// are those of its first token.
typedef struct {
    Span span;
    size_t line;
    size_t column;
} SourcePos;

// --- Types -----------------------------------------------------------------

// The kind of a value, worked out by the checker before the program runs. A
// list holds one kind of item, so a list type is its innermost kind plus
// TYPE_LIST for each level of list: TYPE_LIST + TYPE_NUMBER is a list of
// numbers, 2 * TYPE_LIST + TYPE_TEXT a list of lists of text. Two types are
// the same exactly when they're equal numbers.
typedef unsigned Type;

enum {
    TYPE_UNKNOWN,  // not decided (yet); TYPE_LIST + TYPE_UNKNOWN is a list of something not decided
    TYPE_NUMBER,
    TYPE_TEXT,
    TYPE_YESNO,
    TYPE_NOTHING,  // only what a function gives back when it gives back no value
    TYPE_ERROR,    // stands in after a type error, so one mistake gives one message
    TYPE_LIST = 8,
};

static inline Type type_list_of(Type item) { return item + TYPE_LIST; }
static inline bool type_is_list(Type type) { return type >= TYPE_LIST; }
static inline Type type_item(Type list) { return list - TYPE_LIST; }
static inline Type type_base(Type type) { return type % TYPE_LIST; }  // the innermost kind
static inline bool type_counted(Type type) { return type == TYPE_TEXT || type_is_list(type); }  // reference counted

// "a number", "text", "a yes/no value", "nothing", "a list of numbers",
// "a list of lists of text", "a list" (items not decided).
const char *ast_type_name(Arena *arena, Type type);

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
} ExprKind;

typedef enum {
    UNARY_NEGATE,
    UNARY_NOT,
} UnaryOp;

typedef enum {
    BINARY_OR,
    BINARY_AND,  // logical for two yes/no values, joins text otherwise; the checker decides (its type)
    BINARY_EQUAL,
    BINARY_NOT_EQUAL,
    BINARY_LESS,
    BINARY_LESS_EQUAL,
    BINARY_GREATER,
    BINARY_GREATER_EQUAL,
    BINARY_ADD,
    BINARY_SUBTRACT,
    BINARY_JOIN,  // "followed by"
    BINARY_MULTIPLY,
    BINARY_DIVIDE,
    BINARY_MODULO,
    BINARY_CONTAINS,      // L contains X
    BINARY_NOT_CONTAINS,  // L does not contain X
    BINARY_STARTS_WITH,      // T starts with X
    BINARY_NOT_STARTS_WITH,  // T does not start with X
    BINARY_ENDS_WITH,        // T ends with X
    BINARY_NOT_ENDS_WITH,    // T does not end with X
} BinaryOp;

typedef enum {
    CONVERT_TO_NUMBER,
    CONVERT_TO_TEXT,
} ConvertTarget;

// Which item of a list: item N, the first item, or the last item.
typedef enum {
    ITEM_AT,
    ITEM_FIRST,
    ITEM_LAST,
} ItemWhere;

typedef enum {
    LIST_SUM,        // L added together
    LIST_BIGGEST,    // the biggest of L
    LIST_SMALLEST,   // the smallest of L
    LIST_REVERSED,   // L reversed
    LIST_COPY,       // a copy of L
    LIST_IS_EMPTY,   // L is empty (lists and text)
    LIST_NOT_EMPTY,  // L is not empty / isn't empty
} ListOp;

// Which part of a list a slice takes. Every slice is a new list.
typedef enum {
    SLICE_RANGE,          // items FROM to TO of L (TO NULL: "to the end")
    SLICE_FIRST,          // the first COUNT items in L
    SLICE_LAST,           // the last COUNT items in L
    SLICE_WITHOUT_FIRST,  // L without the first item
    SLICE_WITHOUT_LAST,   // L without the last item
    SLICE_EVERY,          // every STEP-th item of L ("every second item": 2, 4, 6...; "every other item": 1, 3, 5...)
} SliceKind;

// What a text tool does. Each makes something new; text never changes.
typedef enum {
    TEXT_CHARACTERS,  // the characters of T (a list of text)
    TEXT_WORDS,       // the words of T
    TEXT_LINES,       // the lines of T
    TEXT_UPPERCASE,   // T in uppercase
    TEXT_LOWERCASE,   // T in lowercase
    TEXT_TRIM,        // T without extra spaces
    TEXT_SPLIT,       // T split by ARG
    TEXT_JOINED,      // L joined with ARG (L a list)
    TEXT_REPLACE,     // T with ARG replaced by ARG2
    TEXT_POSITION,    // the position of ARG in T (T text or a list; 0 if it isn't there)
    TEXT_CODE,        // the code of T (one character)
    TEXT_FROM_CODE,   // the character with code T (a number)
    TEXT_IS_LETTER,   // T is a letter (negated: is not a letter)
    TEXT_IS_DIGIT,    // T is a digit
    TEXT_IS_SPACE,    // T is a space
} TextOp;

typedef struct Expr Expr;

// A name written in the program, e.g. a variable. A name can be several
// words ("hourly rate"); `text` is how it's matched (lowercased, one space
// between words) and `display` how it was written here ("Hourly Rate",
// "clientName"), for messages.
typedef struct {
    const char *text;
    const char *display;
    SourcePos pos;
} Name;

struct Expr {
    ExprKind kind;
    SourcePos pos;
    Type type;            // filled in by the checker
    const char *display;  // EXPR_NAME, EXPR_CALL: the name as written here, for messages
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
        Vec(Expr *) items;  // EXPR_LIST, in order (at least one)
        Type list_type;     // EXPR_EMPTY_LIST: the list's type, e.g. a list of numbers
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
} StmtKind;

// A constant's value, worked out by the compiler (see front/consteval.h).
typedef enum {
    CONST_NUMBER,
    CONST_TEXT,
    CONST_YESNO,
    CONST_LIST,  // a fixed list ("keep weekdays as a list of ..."): numbers, text or yes/no, never lists
} ConstKind;

typedef struct ConstValue ConstValue;
struct ConstValue {
    ConstKind kind;
    double number;
    bool yes;
    const char *text;  // NUL-terminated
    size_t len;        // CONST_TEXT: bytes; CONST_LIST: items
    Type list_type;    // CONST_LIST: e.g. a list of text
    ConstValue *items; // CONST_LIST
};

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
} ListChange;

typedef struct Stmt Stmt;

// A block is a list of statements, in order. A program is a block.
typedef Vec(Stmt *) Block;

// One branch of an if: "if C", "otherwise if C", or a final "otherwise"
// (condition NULL).
typedef struct {
    Expr *condition;
    Block body;
    SourcePos pos;  // the branch's first line, from "if"/"otherwise" to its colon
} IfBranch;

struct Stmt {
    StmtKind kind;
    SourcePos pos;  // the whole statement, without its period
    Span verb;      // the word that chose the statement as written, e.g. "display"
    union {
        struct {
            Name name;
            Expr *value;
        } assign;  // STMT_LET, STMT_SET
        struct {
            ChangeOp op;
            Name target;
            Expr *amount;
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
        } if_stmt;
        struct {
            Name name;
            const char *intro;     // the word before the inputs: "with", "of", "using" or "to" (NULL: none)
            Vec(Name) params;
            Vec(Type) declared;    // per input: the kind written in "(a number)", or TYPE_UNKNOWN
            Type declared_result;  // "giving back a number", or TYPE_UNKNOWN
            Vec(Type) param_types; // filled in by the checker
            Type result;           // filled in by the checker (TYPE_NOTHING: gives back no value)
            Block body;
        } function;
        Expr *returned;  // STMT_RETURN; NULL for a bare "return"
        Expr *call;      // STMT_CALL: an EXPR_CALL
        struct {
            Name name;
            Expr *value;
            ConstValue folded;  // filled in by the checker
        } constant;
        struct {
            LoopKind kind;
            Expr *from, *to, *step;  // LOOP_COUNT (step NULL: 1)
            bool down;               // "count down": never counts up
            Name var;                // LOOP_COUNT: "number" unless named; LOOP_EACH: each item
            bool var_named;          // the program chose the name (otherwise it's "number")
            Expr *times;             // LOOP_TIMES
            Expr *condition;         // LOOP_WHILE, LOOP_UNTIL
            Expr *list;              // LOOP_EACH
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
        } list;
    } as;
};

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

// Indented outline, one node per line: "label [line:column]", children
// indented two spaces. Statement children come in source order (for files:
// the text, then the file). Used by `easyscript ast`.
void ast_dump_block(const Block *block, StrBuf *out);

#endif
