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

// --- Expressions -----------------------------------------------------------

typedef enum {
    EXPR_ERROR,          // stands in for something that failed to parse
    EXPR_NUMBER,
    EXPR_TEXT,
    EXPR_BOOLEAN,        // yes/no/true/false
    EXPR_NOTHING,
    EXPR_IT,
    EXPR_NAME,           // a variable, or a function called with no inputs
    EXPR_CALL,           // NAME using ARG, ARG, ...
    EXPR_LENGTH,         // length of X
    EXPR_FILE_CONTENTS,  // contents of file X
    EXPR_UNARY,
    EXPR_BINARY,
    EXPR_CONVERT,        // X as a number / X as text
} ExprKind;

typedef enum {
    UNARY_NEGATE,
    UNARY_NOT,
} UnaryOp;

typedef enum {
    BINARY_OR,
    BINARY_AND,  // logical for two yes/no values, joins text otherwise; the checker decides
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
} BinaryOp;

typedef enum {
    CONVERT_TO_NUMBER,
    CONVERT_TO_TEXT,
} ConvertTarget;

typedef struct Expr Expr;

struct Expr {
    ExprKind kind;
    SourcePos pos;
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
        const char *name;  // EXPR_NAME, lowercased
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
} StmtKind;

typedef enum {
    CHANGE_ADD,       // add E to X, increase X by E
    CHANGE_SUBTRACT,  // subtract E from X, decrease X by E
    CHANGE_MULTIPLY,  // multiply X by E
    CHANGE_DIVIDE,    // divide X by E
} ChangeOp;

// A variable name written in a statement.
typedef struct {
    const char *text;  // lowercased
    SourcePos pos;
} Name;

typedef struct Stmt Stmt;

// A block is a list of statements, in order. A program is a block.
typedef Vec(Stmt *) Block;

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

// Indented outline, one node per line: "label [line:column]", children
// indented two spaces. Statement children come in source order (for files:
// the text, then the file). Used by `easyscript ast`.
void ast_dump_block(const Block *block, StrBuf *out);

#endif
