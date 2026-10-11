#define _XOPEN_SOURCE 700
#define _DARWIN_C_SOURCE  // realpath under strict feature macros

#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "modules.h"
#include "front/lexer.h"

// A "use NAME" sentence, at `span` in its file's own text.
typedef struct {
    const char *module;
    Span span;
} UseSentence;

typedef struct {
    const char *path;     // as opened
    const char *real;     // its real path, so a file reached two ways is read once
    const char *dir;      // the folder its uses are looked for in
    const char *display;  // "helpers.es"
    const char *module;   // "helpers"
    const char *text;
    size_t len;
    Vec(UseSentence) uses;
    Vec(size_t) used;     // the files its uses found (SIZE_MAX: not found)
    int state;            // 0 not visited, 1 being loaded (its uses are), 2 loaded
} LoadFile;

// A mistake found while loading, reported once the files are joined.
typedef struct {
    size_t file;
    Span span;
    const char *message;
    const char *note;
} LoadError;

typedef struct {
    Arena *arena;
    Vec(LoadFile) files;
    Vec(size_t) order;    // files in the order they're joined
    Vec(size_t) stack;    // the files being loaded, outermost first
    Vec(LoadError) errors;
} Loader;

static const char *reason(int err) {
    switch (err) {
    case ENOENT: return "it doesn't exist";
    case EACCES: return "permission was denied";
    case EISDIR: return "it's a folder, not a file";
    default: return strerror(err);
    }
}

// The whole file, or NULL with errno set.
static char *read_all(Arena *arena, const char *path, size_t *len) {
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    struct stat st;
    if (fstat(fileno(file), &st) == 0 && S_ISDIR(st.st_mode)) {
        fclose(file);
        errno = EISDIR;
        return NULL;
    }
    long length = fseek(file, 0, SEEK_END) == 0 ? ftell(file) : -1;
    char *data = length >= 0 && fseek(file, 0, SEEK_SET) == 0 ? arena_alloc(arena, (size_t)length + 1) : NULL;
    bool ok = data && fread(data, 1, (size_t)length, file) == (size_t)length;
    int err = errno;
    fclose(file);
    if (!ok) {
        errno = err ? err : EIO;
        return NULL;
    }
    data[length] = '\0';
    *len = (size_t)length;
    return data;
}

static const char *base_name(const char *path) {
    const char *slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

static const char *dir_name(Arena *arena, const char *path) {
    const char *slash = strrchr(path, '/');
    if (!slash) return ".";
    if (slash == path) return "/";
    return arena_sprintf(arena, "%.*s", (int)(slash - path), path);
}

static bool word_is(const Token *token, const char *word) {
    return token->kind == TOK_WORD && strcmp(token->text, word) == 0;
}

// Every "use NAME." (or "use the NAME file.") at the start of a sentence.
// Other shapes are left to the parser, which explains them.
static void find_uses(Loader *ld, LoadFile *file) {
    Diag *quiet = diag_new(ld->arena, file->text, file->len);  // the real lexing reports its mistakes
    TokenList tokens = lex(ld->arena, quiet, file->text, file->len);
    for (size_t i = 0; i + 1 < tokens.len; i++) {
        const Token *t = &tokens.items[i];
        TokenKind before = i ? tokens.items[i - 1].kind : TOK_NEWLINE;
        bool start = before == TOK_NEWLINE || before == TOK_INDENT || before == TOK_DEDENT || before == TOK_PERIOD;
        if (!start || !word_is(t, "use") || tokens.items[i + 1].kind != TOK_WORD) continue;
        size_t end = i + 2;
        if (word_is(&tokens.items[end], "file")) end++;
        TokenKind after = tokens.items[end].kind;
        if (word_is(&tokens.items[i + 1], "same") ||
            (after != TOK_PERIOD && after != TOK_NEWLINE && after != TOK_EOF)) continue;
        const Token *last = &tokens.items[end - 1];
        UseSentence use = {tokens.items[i + 1].text, {t->span.offset, last->span.offset + last->span.length - t->span.offset}};
        vec_push(ld->arena, &file->uses, use);
    }
}

static size_t find_file(const Loader *ld, const char *real) {
    for (size_t i = 0; i < ld->files.len; i++) {
        if (strcmp(ld->files.items[i].real, real) == 0) return i;
    }
    return SIZE_MAX;
}

static void add_error(Loader *ld, size_t file, Span span, const char *message, const char *note) {
    LoadError error = {file, span, message, note};
    vec_push(ld->arena, &ld->errors, error);
}

// "a.es uses b.es, b.es uses c.es, and c.es uses a.es": the circle from `from` (on the stack) back to it.
static const char *describe_circle(Loader *ld, size_t from) {
    size_t at = 0;
    while (ld->stack.items[at] != from) at++;
    StrBuf sb;
    sb_init(&sb, ld->arena);
    size_t n = ld->stack.len - at;
    for (size_t k = 0; k < n; k++) {
        const char *user = ld->files.items[ld->stack.items[at + k]].display;
        const char *used = ld->files.items[k + 1 < n ? ld->stack.items[at + k + 1] : from].display;
        if (k > 0) sb_append(&sb, k + 1 == n ? (n == 2 ? " and " : ", and ") : ", ");
        sb_appendf(&sb, "%s uses %s", user, used);
    }
    return sb.data;
}

static void load(Loader *ld, size_t index);

// Adds the file a "use" sentence names (reading it, and what it uses), or reports why it can't.
static size_t use_file(Loader *ld, size_t user, const UseSentence *use) {
    const LoadFile *from = &ld->files.items[user];
    const char *path = arena_sprintf(ld->arena, "%s/%s.es", from->dir, use->module);
    const char *display = base_name(path);
    char *real = realpath(path, NULL);
    if (!real) {
        add_error(ld, user, use->span,
                  errno == ENOENT ? arena_sprintf(ld->arena, "I couldn't find the file \"%s\" in the same folder as %s.",
                                                  display, from->display)
                                  : arena_sprintf(ld->arena, "I couldn't read the file \"%s\": %s.", display, reason(errno)),
                  arena_sprintf(ld->arena, "\"use %s\" reads %s from the folder %s is in. Check the spelling of its name.",
                                use->module, display, from->display));
        return SIZE_MAX;
    }
    const char *kept = arena_strdup(ld->arena, real);
    free(real);
    size_t found = find_file(ld, kept);
    if (found != SIZE_MAX) {
        if (found == user) {
            add_error(ld, user, use->span, arena_sprintf(ld->arena, "%s can't use itself.", from->display),
                      "Everything a file defines can already be used in it.");
        } else if (ld->files.items[found].state == 1) {
            const char *circle = describe_circle(ld, found);  // the stack ends with this file
            add_error(ld, user, use->span, arena_sprintf(ld->arena, "These files use each other in a circle: %s.", circle),
                      "Move what they share into another file that each of them uses.");
        }
        return found;
    }
    size_t len = 0;
    char *text = read_all(ld->arena, kept, &len);
    if (!text) {
        add_error(ld, user, use->span, arena_sprintf(ld->arena, "I couldn't read the file \"%s\": %s.", display,
                                                     reason(errno)), NULL);
        return SIZE_MAX;
    }
    LoadFile file = {path, kept, dir_name(ld->arena, path), display, use->module, text, len, {0}, {0}, 0};
    vec_push(ld->arena, &ld->files, file);
    size_t index = ld->files.len - 1;
    load(ld, index);
    return index;
}

static void load(Loader *ld, size_t index) {
    ld->files.items[index].state = 1;
    vec_push(ld->arena, &ld->stack, index);
    find_uses(ld, &ld->files.items[index]);
    for (size_t i = 0; i < ld->files.items[index].uses.len; i++) {
        UseSentence use = ld->files.items[index].uses.items[i];
        size_t found = use_file(ld, index, &use);  // may grow ld->files: index, not a pointer
        vec_push(ld->arena, &ld->files.items[index].used, found);
    }
    (void)vec_pop(&ld->stack);
    ld->files.items[index].state = 2;
    vec_push(ld->arena, &ld->order, index);
}

// Joins the files (each ending with a newline) in load order.
static void join(Loader *ld, Sources *out) {
    size_t n = ld->order.len;
    StrBuf sb;
    sb_init(&sb, ld->arena);
    out->names = arena_alloc(ld->arena, n * sizeof(char *));
    out->starts = arena_alloc(ld->arena, n * sizeof(size_t));
    size_t *place = arena_alloc(ld->arena, ld->files.len * sizeof(size_t));  // file index -> joined order
    for (size_t k = 0; k < n; k++) {
        const LoadFile *file = &ld->files.items[ld->order.items[k]];
        place[ld->order.items[k]] = k;
        out->names[k] = file->display;
        out->starts[k] = sb.len;
        sb_append_n(&sb, file->text, file->len);
        if (n > 1 && (file->len == 0 || file->text[file->len - 1] != '\n')) sb_append_char(&sb, '\n');
    }
    out->source = sb.data ? sb.data : arena_strdup(ld->arena, "");
    out->len = sb.len;
    out->count = n;
    if (n < 2) return;
    for (size_t k = 0; k < n; k++) {
        const LoadFile *file = &ld->files.items[ld->order.items[k]];
        SourceFile source = {file->module, file->display, {0}};
        for (size_t u = 0; u < file->used.len; u++) {
            if (file->used.items[u] != SIZE_MAX) vec_push(ld->arena, &source.uses, place[file->used.items[u]]);
        }
        vec_push(ld->arena, &out->files, source);
    }
    for (size_t i = 0; i < ld->errors.len; i++) ld->errors.items[i].file = place[ld->errors.items[i].file];
}

Sources *sources_load(Arena *arena, const char *path, const char *dir) {
    Loader ld = {0};
    ld.arena = arena;
    size_t len = 0;
    char *text = read_all(arena, path, &len);
    if (!text) {
        int err = errno;
        fprintf(stderr, "I couldn't %s the file \"%s\": %s.\n", err == EISDIR ? "read" : "open", path, reason(err));
        exit(1);
    }
    char *real = realpath(path, NULL);
    const char *kept = arena_strdup(arena, real ? real : path);
    free(real);
    const char *display = base_name(path);
    size_t name_len = strlen(display);
    if (name_len > 3 && strcmp(display + name_len - 3, ".es") == 0) name_len -= 3;
    const char *module = arena_sprintf(arena, "%.*s", (int)name_len, display);
    LoadFile first = {path, kept, dir ? dir : dir_name(arena, path), display, module, text, len, {0}, {0}, 0};
    vec_push(arena, &ld.files, first);
    load(&ld, 0);

    Sources *out = arena_alloc(arena, sizeof(Sources));
    join(&ld, out);
    out->diag = diag_new(arena, out->source, out->len);
    if (out->count > 1) diag_set_files(out->diag, out->names, out->starts, out->count);
    for (size_t i = 0; i < ld.errors.len; i++) {
        const LoadError *error = &ld.errors.items[i];
        Span span = {error->span.offset + out->starts[error->file], error->span.length};
        diag_error(out->diag, span, "%s", error->message);
        if (error->note) diag_note(out->diag, "%s", error->note);
    }
    return out;
}
