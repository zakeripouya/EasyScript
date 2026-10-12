// The operators and phrase kinds of the syntax tree (ast.h): what a unary,
// binary, list, text or library node does. Split from ast.h to keep it short.
#ifndef AST_OPS_H
#define AST_OPS_H

// What a value of the standard library is (EXPR_LIBRARY), with its parts in a, b and c.
typedef enum {
    LIB_SQUARE_ROOT,     // the square root of A
    LIB_POWER,           // A to the power of B
    LIB_ABSOLUTE,        // the absolute value of A
    LIB_ROUNDED,         // A rounded
    LIB_ROUNDED_UP,      // A rounded up
    LIB_ROUNDED_DOWN,    // A rounded down
    LIB_ROUNDED_PLACES,  // A rounded to B decimal places
    LIB_BIGGER,          // the bigger of A and B
    LIB_SMALLER,         // the smaller of A and B
    LIB_PI,              // pi (a name the program doesn't make itself)
    LIB_RANDOM_WHOLE,    // a random number from A to B (whole numbers, both included)
    LIB_RANDOM_DECIMAL,  // a random decimal between A and B
    LIB_PICK,            // pick one of A / a random item of A
    LIB_SHUFFLED,        // A shuffled
    LIB_DIE,             // roll a die
    LIB_NOW,             // the time now
    LIB_TODAY,           // today (a name the program doesn't make itself), and tomorrow, yesterday
    LIB_TOMORROW,
    LIB_YESTERDAY,
    LIB_DAYS_AGO,        // A days ago
    LIB_DAYS_FROM_NOW,   // A days from now
    LIB_DATE_PART,       // A's year (or month, day, weekday, hour, minute), or the year of A: `part`
    LIB_IN_PERIOD,       // A is in this month (or last month, next month, this year, ...): `part`
    LIB_DAYS_BETWEEN,    // the number of days between A and B
    LIB_BEFORE,          // A is before B
    LIB_AFTER,           // A is after B
    LIB_FILE_EXISTS,     // file A exists
    LIB_FOLDER_EXISTS,   // folder A exists
    LIB_FILE_LINES,      // the lines of file A
    LIB_FILES_IN,        // the files in A
    LIB_FOLDERS_IN,      // the folders in A
    LIB_FILE_SIZE,       // the size of file A
    LIB_FILE_CHANGED,    // the last change of file A
    LIB_HALF,            // half of A
    LIB_TWICE,           // twice A
    LIB_PERCENT_OF,      // A percent of B
    LIB_IS_INFINITE,     // A is infinite (part 0) / A is not infinite (part 1)
    LIB_REASON,          // the reason (in "if that fails, ..."): why the sentence tried couldn't be done
    LIB_PERIOD_KEY,      // "per month": A's month as a group ("October 2026"); `part` a DatePart (year, month,
                         // day or weekday). A is the item's date field; its name is NULL until the checker finds it.
    LIB_PROGRAM_WORDS,   // the words given to the program (system tools, parse_system.c)
    LIB_PROGRAM_COUNT,   // the number of words given to the program
    LIB_PROGRAM_WORD,    // word A given to the program
    LIB_SENT_TEXT,       // the text sent to the program
    LIB_SENT_LINES,      // the lines sent to the program
    LIB_SETTING,         // the setting called A (text or nothing)
    LIB_KEY_PRESSED,     // the key being pressed (text or nothing)
    LIB_WORKED,          // A worked (part 0) / A didn't work (part 1): A is a program result
} LibraryOp;

// Which part of a date (LIB_DATE_PART), or which stretch of time (LIB_IN_PERIOD).
typedef enum {
    DATE_YEAR, DATE_MONTH, DATE_DAY, DATE_WEEKDAY, DATE_HOUR, DATE_MINUTE,
} DatePart;
typedef enum {
    PERIOD_THIS_MONTH, PERIOD_LAST_MONTH, PERIOD_NEXT_MONTH, PERIOD_THIS_YEAR, PERIOD_LAST_YEAR, PERIOD_NEXT_YEAR,
} Period;

typedef enum {
    UNARY_NEGATE,
    UNARY_NOT,
} UnaryOp;

typedef enum {
    BINARY_OR,
    BINARY_AND,  // both sides yes (two yes/no values)
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
    BINARY_HAS,              // M has K (M a map)
    BINARY_NOT_HAS,          // M does not have K
    BINARY_PIECES,           // "say A, B": the pieces as text, with a space between them where it's needed
} BinaryOp;

typedef enum {
    CONVERT_TO_NUMBER,
    CONVERT_TO_TEXT,
    CONVERT_TO_EXACT_TEXT,  // "as exact text": digits that read back as the same number
    CONVERT_TO_MONEY,       // "as money": "$1,234.50"
    CONVERT_TO_PERCENT,     // "as a percent": 0.25 is "25%"
    CONVERT_WITH_COMMAS,    // "with commas": "1,234,567.5"
    CONVERT_TO_TABLE,       // "as a table": a list or a map laid out in columns
    CONVERT_TO_TEXT_KIND,   // "as an email": text checked by a kind of text's rule
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
    LIST_SORTED,     // L sorted [smallest first]
    LIST_SORTED_DOWN,          // L sorted biggest first
    LIST_SORTED_LENGTH,        // L sorted by their length [shortest first]
    LIST_SORTED_LENGTH_DOWN,   // L sorted by their length longest first
    LIST_COPY,       // a copy of L
    LIST_IS_EMPTY,   // L is empty (lists and text)
    LIST_NOT_EMPTY,  // L is not empty / isn't empty
    LIST_ENTRIES,    // the number of entries in M (a map)
    LIST_KEYS,       // the keys of M (a list)
    LIST_VALUES,     // the values of M (a list)
} ListOp;

// Which part of a list a slice takes. Every slice is a new list.
typedef enum {
    SLICE_RANGE,          // items FROM to TO of L (TO NULL: "to the end")
    SLICE_FIRST,          // the first COUNT items in L
    SLICE_LAST,           // the last COUNT items in L
    SLICE_WITHOUT_FIRST,  // L without the first item
    SLICE_WITHOUT_LAST,   // L without the last item
    SLICE_EVERY,          // every STEP-th item of L ("every second item": 2, 4, 6...; "every other item": 1, 3, 5...)
    SLICE_WITHOUT_AT,     // L without item FROM (a position)
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

// What a built-in that takes a function does with it (EXPR_APPLY). Each makes something new.
typedef enum {
    APPLY_MAP,      // F applied to every X in L: a list of F of each item
    APPLY_FILTER,   // the Xs in L that pass F: the items F gives yes for
    APPLY_COMBINE,  // L combined using F [, starting from S]: F of what's combined so far and each item
    APPLY_SORT,     // L sorted by F [biggest first]: the items ordered by F of each
} ApplyKind;

// What a question about a list works out (EXPR_AGGREGATE). Each goes through the list once.
typedef enum {
    AGG_COUNT,        // how many L are ... / how many L there are / the number of L
    AGG_ANY,          // any score is ... / any of L are ...: yes once one item passes the test
    AGG_ALL,          // every score is ... / all L are ...: no once one item fails it
    AGG_NONE,         // no score is ... / none of L are ...: no once one item passes it
    AGG_TOTAL,        // the total [FIELD] of L
    AGG_AVERAGE,      // the average [FIELD] of L
    AGG_MEDIAN,       // the median [FIELD] of L
    AGG_TOP,          // the top N L [by FIELD] / the lowest N L (lowest)
    AGG_MOST_COMMON,  // the N most common L
    AGG_BEST,         // the ITEM [in L] with the highest FIELD / the lowest (lowest)
} AggregateKind;

#endif
