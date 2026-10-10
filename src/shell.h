#ifndef SHELL_H
#define SHELL_H

#include "common/arena.h"

// The interactive shell (`easyscript` with no arguments). `self` is how to
// run this compiler again; the session's source and the answers to earlier
// "ask"s are kept in the two files given (in the caller's temp directory).
void shell_run(Arena *arena, char *self, const char *session_path, const char *answers_path);

#endif
