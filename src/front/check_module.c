// Files that use other files. Each file sees its own definitions and the
// ones (not made private) of the files it uses; its own come first. A name
// two used files both define is only a mistake where it's used without
// saying which ("helpers's double"). Files other than the one that's run can
// only define things. Kinds of record, kinds with variants and interfaces
// share one set of names across the program.

#include <string.h>
#include "front/check_internal.h"

static bool several_files(const Checker *c) {
    return c->program->files.len > 1;
}

static const char *file_display(const Checker *c, unsigned file) {
    return c->program->files.items[file].display;
}

static bool file_uses(const Checker *c, unsigned user, unsigned used) {
    const SourceFile *file = &c->program->files.items[user];
    for (size_t i = 0; i < file->uses.len; i++) {
        if (file->uses.items[i] == used) return true;
    }
    return false;
}

bool checker_visible(const Checker *c, unsigned file, bool private) {
    return file == c->file || (!private && several_files(c) && file_uses(c, c->file, file));
}

const char *checker_where(Checker *c, SourcePos pos) {
    if (!several_files(c) || pos.file == c->file) return arena_sprintf(c->arena, "line %zu", pos.line);
    return arena_sprintf(c->arena, "line %zu of %s", pos.line, file_display(c, pos.file));
}

// A function (not a method: those are told apart by their record) or a constant named `name` in `file`.
static const Stmt *defined_in(const Checker *c, const char *name, unsigned file, bool *private) {
    for (size_t i = 0; i < c->functions.len; i++) {
        const Function *f = &c->functions.items[i];
        if (f->file == file && strcmp(f->name, name) == 0 && !f->definition->as.function.method) {
            *private = f->private;
            return f->definition;
        }
    }
    for (size_t i = 0; i < c->constants.len; i++) {
        const Constant *k = &c->constants.items[i];
        if (k->file == file && strcmp(k->name, name) == 0) {
            *private = k->private;
            return k->definition;
        }
    }
    return NULL;
}

static size_t module_file(const Checker *c, const char *module) {
    for (size_t i = 0; i < c->program->files.len; i++) {
        if (strcmp(c->program->files.items[i].name, module) == 0) return i;
    }
    return (size_t)-1;
}

// "helpers's double": looks only in helpers.es, which this file has to use.
static bool enter_module(Checker *c, const Expr *expr, const char *name) {
    size_t file = module_file(c, expr->module);
    if (file == (size_t)-1 || file == c->file || !file_uses(c, c->file, (unsigned)file)) {
        diag_error(c->diag, expr->pos.span, "This file doesn't use %s.es, so \"%s's\" doesn't name anything here.",
                   expr->module, expr->module);
        diag_note(c->diag, "Add \"use %s.\" at the top of this file.", expr->module);
        return false;
    }
    bool private = false;
    const Stmt *def = defined_in(c, name, (unsigned)file, &private);
    if (!def) {
        diag_error(c->diag, expr->pos.span, "%s doesn't define anything called \"%s\".", file_display(c, (unsigned)file),
                   expr->display);
        diag_note(c->diag, "\"%s's\" can only name a function or a constant made in %s.", expr->module,
                  file_display(c, (unsigned)file));
        return false;
    }
    if (private) {
        diag_error(c->diag, expr->pos.span, "\"%s\" is private to %s, so only that file can use it.", expr->display,
                   file_display(c, (unsigned)file));
        diag_note(c->diag, "To share it, take out \"privately\" on %s.", checker_where(c, def->pos));
        return false;
    }
    c->only_file = (unsigned)file + 1;
    if (expr->kind == EXPR_NAME) c->floor = c->made.len;  // not a name this file made
    return true;
}

// The files (other than this one) whose definitions of `name` this file can see; how many.
static size_t visible_files(Checker *c, const char *name, unsigned *files, size_t most) {
    size_t n = 0;
    for (unsigned file = 0; file < c->program->files.len && n < most; file++) {
        bool private = false;
        if (file != c->file && defined_in(c, name, file, &private) && checker_visible(c, file, private)) {
            files[n++] = file;
        }
    }
    return n;
}

bool checker_enter_names(Checker *c, const Expr *expr, const char *name, NameContext *saved) {
    saved->only_file = c->only_file;
    saved->floor = c->floor;
    if (expr->module) return enter_module(c, expr, name);
    bool private;
    if (!several_files(c) || checker_find_made(c, name) || defined_in(c, name, c->file, &private)) return true;
    unsigned files[3];  // enough to tell "both" from "several"
    size_t n = visible_files(c, name, files, 3);
    if (n < 2) return true;
    if (n == 2) {
        diag_error(c->diag, expr->pos.span, "\"%s\" is defined in both %s and %s.", expr->display,
                   file_display(c, files[0]), file_display(c, files[1]));
    } else {
        diag_error(c->diag, expr->pos.span, "\"%s\" is defined in several of the files this one uses.", expr->display);
    }
    diag_note(c->diag, "Say which one you mean, like \"%s's %s\" or \"%s's %s\".", c->program->files.items[files[0]].name,
              expr->display, c->program->files.items[files[1]].name, expr->display);
    return false;
}

void checker_leave_names(Checker *c, const NameContext *saved) {
    c->only_file = saved->only_file;
    c->floor = saved->floor;
}

void checker_resolved(Checker *c, const Expr *expr, const Stmt *definition) {
    if (!several_files(c) || definition->pos.file == c->main_file) return;
    struct ModuleUse use = {(Expr *)expr, definition};
    vec_push(c->arena, &c->module_uses, use);
}

bool checker_report_hidden(Checker *c, const char *name, const char *display, SourcePos pos) {
    if (!several_files(c)) return false;
    for (unsigned file = 0; file < c->program->files.len; file++) {
        bool private = false;
        const Stmt *def = defined_in(c, name, file, &private);
        if (!def || file == c->file) continue;
        if (private) {
            diag_error(c->diag, pos.span, "\"%s\" is private to %s, so only that file can use it.", display,
                       file_display(c, file));
            diag_note(c->diag, "To share it, take out \"privately\" on %s.", checker_where(c, def->pos));
        } else {
            diag_error(c->diag, pos.span, "\"%s\" is defined in %s, which this file doesn't use.", display,
                       file_display(c, file));
            diag_note(c->diag, "Add \"use %s.\" at the top of this file.", c->program->files.items[file].name);
        }
        return true;
    }
    return false;
}

bool checker_check_module_stmt(Checker *c, const Stmt *stmt) {
    if (!several_files(c) || stmt->pos.file == c->main_file) return true;
    switch (stmt->kind) {
    case STMT_FUNCTION:
    case STMT_CONSTANT:
    case STMT_RECORD:
    case STMT_SUM:
    case STMT_INTERFACE:
    case STMT_USE: return true;
    default: break;
    }
    Span verb = stmt->verb.length ? stmt->verb : stmt->pos.span;
    diag_error(c->diag, verb, "%s is used by another file, so it can only define things.", file_display(c, stmt->pos.file));
    diag_note(c->diag, "Only the file you run can do things. Move this sentence into %s, or into something "
                       "defined with \"to\".", file_display(c, c->main_file));
    return false;
}

void checker_check_use(Checker *c, const Stmt *stmt) {
    if (c->blocks == 0 && !c->function && !c->lambda) return;
    diag_error(c->diag, stmt->pos.span, "\"use\" goes at the top of a file, not inside a block.");
    diag_note(c->diag, "Move \"use %s.\" to the start of the file, with no spaces before it.", stmt->as.module.display);
}

void checker_rename_modules(Checker *c) {
    if (!several_files(c)) return;
    for (size_t i = 0; i < c->functions.len; i++) {
        Function *f = &c->functions.items[i];
        Stmt *def = f->definition;
        if (f->file == c->main_file || def->as.function.method) continue;
        def->as.function.name.text = arena_sprintf(c->arena, "%s 0m%u", f->name, f->file);
    }
    for (size_t i = 0; i < c->constants.len; i++) {
        Constant *k = &c->constants.items[i];
        if (k->file == c->main_file) continue;
        k->definition->as.constant.name.text = arena_sprintf(c->arena, "%s 0m%u", k->name, k->file);
    }
    for (size_t i = 0; i < c->module_uses.len; i++) {
        Expr *expr = c->module_uses.items[i].expr;
        const Stmt *def = c->module_uses.items[i].definition;
        const char *name = def->kind == STMT_FUNCTION ? def->as.function.name.text : def->as.constant.name.text;
        if (expr->kind == EXPR_NAME) expr->as.name = name;
        if (expr->kind == EXPR_CALL) expr->as.call.name = name;
        if (expr->kind == EXPR_FUNCTION_REF) expr->as.ref.name = name;
    }
}
