#ifndef MODULES_H
#define MODULES_H

// Finds the files a program uses ("use helpers." loads helpers.es from the
// same folder as the file that says it), in an order where every file comes
// after the files it uses (the file that's run is last), and joins them into
// one source. Each file is read once, however many files use it.

#include "common/ast.h"
#include "common/diag.h"

typedef struct {
    const char *source;   // every file, each ending with a newline
    size_t len;
    size_t count;         // files
    const char **names;   // "helpers.es"
    size_t *starts;       // where each file starts in source
    SourceFiles files;  // what each file is called by "use", and which files it uses
    Diag *diag;           // over source, already holding errors found while loading (a missing file, a circle)
} Sources;

// Reads `path` and every file it uses, directly or through others. `dir`
// (or NULL: the folder of `path`) is where the first file's uses are looked
// for. A file that can't be read at all stops the compiler with a message.
Sources *sources_load(Arena *arena, const char *path, const char *dir);

#endif
