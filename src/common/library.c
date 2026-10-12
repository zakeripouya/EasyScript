// Names of the standard library's values and sentences, for `easyscript ast`
// and for messages.

#include "common/ast.h"

const char *ast_library_op_name(LibraryOp op) {
    static const char *const names[] = {
        "square root of", "to the power of", "absolute value of", "rounded", "rounded up", "rounded down",
        "rounded to decimal places", "bigger of", "smaller of", "pi", "random number from", "random decimal between",
        "pick one of", "shuffled", "roll a die", "time now", "today", "tomorrow", "yesterday", "days ago",
        "days from now", "date part", "is in", "days between", "is before", "is after", "file exists",
        "folder exists", "lines of file", "files in", "folders in", "size of file", "last change of file",
        "half of", "twice", "percent of", "is infinite", "per",
        "words given to the program", "number of words given to the program", "word given to the program",
        "text sent to the program", "lines sent to the program", "setting called", "key being pressed", "worked",
    };
    return names[op];
}

const char *ast_date_part_name(int part) {
    static const char *const names[] = {"year", "month", "day", "weekday", "hour", "minute"};
    return names[part];
}

const char *ast_period_name(int period) {
    static const char *const names[] = {"this month", "last month", "next month", "this year", "last year",
                                        "next year"};
    return names[period];
}

const char *ast_library_action_name(LibraryAction op) {
    static const char *const names[] = {
        "wait", "use the same random numbers", "shuffle", "clear the screen", "make a file", "make a folder",
        "rename", "copy", "move", "delete the file", "delete the folder", "read csv file", "save as csv file",
        "read the table", "complain", "run", "wait for a key",
    };
    return names[op];
}
