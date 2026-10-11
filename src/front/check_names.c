// Names: finding what a name means (a variable made so far, a function, a
// constant), making variables, and the "I don't know anything called"
// message with its most helpful note.

#include <string.h>
#include "front/check_internal.h"

static const Symbol *find(const Symbol *items, size_t len, const char *name) {
    for (size_t i = 0; i < len; i++) {
        if (strcmp(items[i].name, name) == 0) return &items[i];
    }
    return NULL;
}

const Symbol *checker_find_made(const Checker *c, const char *name) {
    if (c->made.len == c->floor) return NULL;  // also avoids NULL + 0 on an empty list
    return find(c->made.items + c->floor, c->made.len - c->floor, name);
}

const Symbol *checker_find_later(const Checker *c, const char *name) {
    return c->later.len ? find(c->later.items, c->later.len, name) : NULL;
}

// Whether a definition in `file` can be used here: only from that file while
// a name like "helpers's double" is looked up, otherwise from this file or
// one it uses.
static bool usable(const Checker *c, unsigned file, bool private) {
    return c->only_file ? file + 1 == c->only_file : checker_visible(c, file, private);
}

// This file's own definition comes first, then one from a file it uses.
const Function *checker_find_function(const Checker *c, const char *name) {
    const Function *found = NULL;
    for (size_t i = 0; i < c->functions.len; i++) {
        const Function *f = &c->functions.items[i];
        if (strcmp(f->name, name) != 0 || !usable(c, f->file, f->private)) continue;
        if (f->file == c->file) return f;
        if (!found) found = f;
    }
    return found;
}

bool checker_shared_method(const Checker *c, const char *name) {
    size_t count = 0;
    for (size_t i = 0; i < c->functions.len; i++) {
        const Function *f = &c->functions.items[i];
        count += strcmp(f->name, name) == 0 && f->definition->as.function.method;
    }
    return count > 1;
}

const Constant *checker_find_constant(const Checker *c, const char *name) {
    const Constant *found = NULL;
    for (size_t i = 0; i < c->constants.len; i++) {
        const Constant *k = &c->constants.items[i];
        if (strcmp(k->name, name) != 0 || !usable(c, k->file, k->private)) continue;
        if (k->file == c->file) return k;
        if (!found) found = k;
    }
    return found;
}

// A variable can't share a name with a function or a constant. Reports and
// returns true if it does.
bool checker_clashes(Checker *c, const Name *name) {
    const Constant *k = checker_find_constant(c, name->text);
    if (k) {
        diag_error(c->diag, name->pos.span, "\"%s\" is a constant, made on %s.", name->display,
                   checker_where(c, k->definition->pos));
        diag_note(c->diag, "Give the variable a different name.");
        return true;
    }
    const Function *f = checker_find_function(c, name->text);
    if (!f) return false;
    diag_error(c->diag, name->pos.span, "\"%s\" is the name of a function, defined on %s.", name->display,
               checker_where(c, f->definition->pos));
    diag_note(c->diag, "Give the variable a different name.");
    return true;
}

void checker_make(Checker *c, const Name *name, TypeVar var) {
    if (checker_find_made(c, name->text) || checker_clashes(c, name)) return;
    Symbol symbol = {name->text, name->display, name->pos.line, NULL, false, var, false,
                     c->function ? c->function->definition : NULL, 0};
    vec_push(c->arena, &c->made, symbol);
}

// The made names closest to `name` by spelling; *count is how many tied.
static const Symbol *closest(const Checker *c, const char *name, size_t *count) {
    size_t limit = strlen(name) <= 3 ? 1 : 2;
    size_t best = limit + 1;
    const Symbol *found = NULL;
    *count = 0;
    for (size_t i = c->floor; i < c->made.len; i++) {
        size_t d = edit_distance(c->arena, name, c->made.items[i].name);
        if (d < best) {
            best = d;
            found = &c->made.items[i];
            *count = 1;
        } else if (d == best) {
            (*count)++;
        }
    }
    return found;
}

static void note_choices(Checker *c, const char *name, size_t limit) {
    StrBuf choices;
    sb_init(&choices, c->arena);
    size_t total = 0;
    for (size_t i = c->floor; i < c->made.len; i++) {
        if (edit_distance(c->arena, name, c->made.items[i].name) == limit) total++;
    }
    size_t n = 0;
    for (size_t i = c->floor; i < c->made.len; i++) {
        if (edit_distance(c->arena, name, c->made.items[i].name) != limit) continue;
        n++;
        if (n > 1) sb_append(&choices, n == total ? " or " : ", ");
        sb_appendf(&choices, "\"%s\"", c->made.items[i].display);
    }
    diag_note(c->diag, "Did you mean %s?", choices.data);
}


void checker_report_unknown(Checker *c, const char *name, const char *display, SourcePos pos) {
    if (checker_report_hidden(c, name, display, pos)) return;
    diag_error(c->diag, pos.span, "I don't know anything called \"%s\".", display);
    size_t count;
    const Symbol *near = closest(c, name, &count);
    const Symbol *later = find(c->later.items, c->later.len, name);
    const Symbol *ended = find(c->ended.items, c->ended.len, name);
    bool in_function = c->function && !c->function->definition->as.function.lambda;
    const Symbol *outside = in_function ? find(c->later.items, c->later.len, name) : NULL;
    const Constant *near_constant = checker_closest_constant(c, name);
    size_t fn_len = c->function ? strlen(c->function->name) : 0, len = strlen(name);
    bool ends_function_name = fn_len > len && c->function->name[fn_len - len - 1] == ' ' &&
                              strcmp(c->function->name + fn_len - len, name) == 0;
    if (ends_function_name) {  // "to greet someone:" is a function called "greet someone"
        const char *display_fn = c->function->display;
        size_t keep = strlen(display_fn) - len - 1;
        diag_note(c->diag, "This function is called \"%s\". To make \"%s\" an input, introduce it with \"with\": "
                           "\"to %.*s with %s:\".", display_fn, display, (int)keep, display_fn, display);
    } else if (outside) {
        diag_note(c->diag, "A function only sees its own inputs and the names it makes. To use \"%s\" here, "
                           "pass it in as an input.", display);
    } else if (ended && ended->loop_item) {
        diag_note(c->diag, "\"%s\" is each item of the loop on line %zu, so it only exists inside that loop. "
                           "To keep one, make a variable before the loop and set it inside.", ended->display, ended->line);
    } else if (ended && ended->loop_number) {
        diag_note(c->diag, "\"%s\" is the number of the loop on line %zu, so it only exists inside that loop. "
                           "To keep it, make a variable before the loop and set it inside.", ended->display, ended->line);
    } else if (ended) {
        diag_note(c->diag, "You made \"%s\" inside %s on line %zu, so it only exists inside that block. "
                           "To use it afterwards, make it before %s.", ended->display, ended->where, ended->line, ended->where);
    } else if (strcmp(name, "letter") == 0 || strcmp(name, "digit") == 0 || strcmp(name, "space") == 0) {
        diag_note(c->diag, "To check whether a character is a %s, write \"is a %s\", with the \"a\".", name, name);
    } else if (near_constant && !near) {
        diag_note(c->diag, "Did you mean \"%s\"? It's a constant, kept on line %zu.", near_constant->display,
                  near_constant->definition->pos.line);
    } else if (near && count == 1) {
        diag_note(c->diag, "Did you mean \"%s\"? You made it on line %zu.", near->display, near->line);
    } else if (near) {
        note_choices(c, name, edit_distance(c->arena, name, near->name));
    } else if (later && later->line > pos.line) {
        diag_note(c->diag, "You make \"%s\" later, on line %zu. Make it before you use it.", later->display, later->line);
    } else {
        diag_note(c->diag, "Make it first, like \"let %s be 0\".", display);
    }
}
