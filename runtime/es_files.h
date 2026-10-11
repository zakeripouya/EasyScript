/* EasyScript runtime, part 13: files and folders, csv files, and the screen.
 *
 * Every path is text the program wrote, used as it is (relative to the
 * folder the program runs in); nothing goes through a shell. Making,
 * renaming, copying and moving never replace something that's already
 * there, and deleting a folder with everything in it refuses paths like
 * "/" and "." that would mean far more than one folder. */
#ifndef ES_FILES_H
#define ES_FILES_H

#ifndef ES_TIME_H
#include "es_time.h"
#endif

#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

/* --- Asking about files and folders ------------------------------------------ */

/* 1 a file, 2 a folder, 0 nothing (or something else). */
static int es_path_kind(const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) return 0;
    return S_ISDIR(st.st_mode) ? 2 : S_ISREG(st.st_mode) ? 1 : 0;
}

/* "file F exists" (folder false) and "folder F exists". Consumes path. */
static bool es_exists(EsText *path, bool folder) {
    bool yes = es_path_kind(path->chars) == (folder ? 2 : 1);
    es_release(path);
    return yes;
}

/* Stops with "I couldn't find the file ..." unless path is a file (or, if folder, a folder). */
static void es_need(int site, const char *path, bool folder) {
    int kind = es_path_kind(path);
    if (kind == (folder ? 2 : 1)) return;
    if (kind) es_fail(site, NULL, "\"%s\" is a %s, not a %s.", path, folder ? "file" : "folder", folder ? "folder" : "file");
    es_fail(site, NULL, "I couldn't find the %s \"%s\".", folder ? "folder" : "file", path);
}

/* Stops unless nothing is at path yet. */
static void es_need_free(int site, const char *path) {
    int kind = es_path_kind(path);
    struct stat st;
    if (kind || lstat(path, &st) == 0) {
        es_fail(site, "Nothing is ever replaced this way. Delete it first, or pick another name.",
                "There's already a %s called \"%s\".", kind == 2 ? "folder" : "file", path);
    }
}

static double es_file_size(int site, EsText *path) {
    es_need(site, path->chars, false);
    struct stat st;
    stat(path->chars, &st);
    es_release(path);
    return (double)st.st_size;
}

/* "the last change of file F": a date, on the local calendar. */
static double es_file_changed(int site, EsText *path) {
    es_need(site, path->chars, false);
    struct stat st;
    stat(path->chars, &st);
    es_release(path);
    struct tm local;
    time_t when = st.st_mtime;
    localtime_r(&when, &local);
    return es_days_from_civil(local.tm_year + 1900L, local.tm_mon + 1, local.tm_mday) * ES_DAY + local.tm_hour * 3600 +
           local.tm_min * 60 + local.tm_sec;
}

static int es_compare_names(const void *a, const void *b) {
    return strcmp(*(char *const *)a, *(char *const *)b);
}

/* "the files in D" (folders false) and "the folders in D": their names, in alphabetical order (by bytes). */
static EsList *es_folder_list(int site, EsText *path, bool folders) {
    es_need(site, path->chars, true);
    DIR *dir = opendir(path->chars);
    if (!dir) es_fail(site, NULL, "I couldn't look in the folder \"%s\": %s.", path->chars, es_reason(errno));
    char **names = NULL;
    size_t n = 0, cap = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        size_t len = strlen(path->chars) + strlen(entry->d_name) + 2;
        char *full = malloc(len);
        if (!full) es_out_of_memory();
        snprintf(full, len, "%s/%s", path->chars, entry->d_name);
        bool wanted = es_path_kind(full) == (folders ? 2 : 1);
        free(full);
        if (!wanted) continue;
        if (n == cap) {
            cap = cap ? 2 * cap : 16;
            names = realloc(names, cap * sizeof(char *));
            if (!names) es_out_of_memory();
        }
        names[n] = malloc(strlen(entry->d_name) + 1);
        if (!names[n]) es_out_of_memory();
        strcpy(names[n++], entry->d_name);
    }
    closedir(dir);
    if (n > 1) qsort(names, n, sizeof(char *), es_compare_names);
    EsList *list = es_list_new(ES_ITEMS_TEXT, n);
    for (size_t i = 0; i < n; i++) {
        es_push_t(list, es_text_copy(names[i], strlen(names[i])));
        free(names[i]);
    }
    free(names);
    es_release(path);
    return list;
}

/* --- Changing files and folders ----------------------------------------------- */

/* "make a new file called F" (empty), "make a folder called F". Consumes path. */
static void es_make(int site, EsText *path, bool folder) {
    es_need_free(site, path->chars);
    if (folder ? mkdir(path->chars, 0777) != 0 : false) {
        es_fail(site, NULL, "I couldn't make the folder \"%s\": %s.", path->chars,
                errno == ENOENT ? "the folder it should go in doesn't exist" : es_reason(errno));
    }
    if (!folder) {
        FILE *f = fopen(path->chars, "wx");
        if (!f || fclose(f) != 0) {
            es_fail(site, NULL, "I couldn't make the file \"%s\": %s.", path->chars,
                    errno == ENOENT ? "the folder it should go in doesn't exist" : es_reason(errno));
        }
    }
    es_release(path);
}

/* "rename the file A to B" (or the folder). Consumes both. */
static void es_rename(int site, EsText *from, EsText *to, bool folder) {
    es_need(site, from->chars, folder);
    es_need_free(site, to->chars);
    if (rename(from->chars, to->chars) != 0) {
        es_fail(site, NULL, "I couldn't rename \"%s\" to \"%s\": %s.", from->chars, to->chars, es_reason(errno));
    }
    es_release(from);
    es_release(to);
}

/* "copy the file A to B". Consumes both. */
static void es_copy_file(int site, EsText *from, EsText *to) {
    es_need(site, from->chars, false);
    es_need_free(site, to->chars);
    FILE *in = fopen(from->chars, "rb");
    if (!in) es_fail(site, NULL, "I couldn't read the file \"%s\": %s.", from->chars, es_reason(errno));
    FILE *out = fopen(to->chars, "wbx");
    if (!out) es_fail(site, NULL, "I couldn't make the file \"%s\": %s.", to->chars, es_reason(errno));
    size_t size = 1 << 16, n;
    char *chunk = malloc(size);
    if (!chunk) es_out_of_memory();
    while ((n = fread(chunk, 1, size, in)) > 0) {
        if (fwrite(chunk, 1, n, out) != n) break;
    }
    free(chunk);
    bool failed = ferror(in) || ferror(out);
    fclose(in);
    if (fclose(out) != 0 || failed) es_fail(site, NULL, "I couldn't copy \"%s\" to \"%s\".", from->chars, to->chars);
    es_release(from);
    es_release(to);
}

/* "move the file A to the folder D": it keeps its name. Consumes both. */
static void es_move(int site, EsText *from, EsText *folder_path, bool folder) {
    es_need(site, from->chars, folder);
    es_need(site, folder_path->chars, true);
    const char *slash = strrchr(from->chars, '/');
    const char *name = slash ? slash + 1 : from->chars;
    size_t len = folder_path->len + strlen(name) + 2;
    char *target = malloc(len);
    if (!target) es_out_of_memory();
    snprintf(target, len, "%s/%s", folder_path->chars, name);
    es_need_free(site, target);
    if (rename(from->chars, target) != 0) {
        es_fail(site, NULL, "I couldn't move \"%s\" to the folder \"%s\": %s.", from->chars, folder_path->chars,
                es_reason(errno));
    }
    free(target);
    es_release(from);
    es_release(folder_path);
}

static void es_delete_tree(int site, const char *path) {
    DIR *dir = opendir(path);
    if (!dir) es_fail(site, NULL, "I couldn't look in the folder \"%s\": %s.", path, es_reason(errno));
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        size_t len = strlen(path) + strlen(entry->d_name) + 2;
        char *full = malloc(len);
        if (!full) es_out_of_memory();
        snprintf(full, len, "%s/%s", path, entry->d_name);
        struct stat st;
        if (lstat(full, &st) == 0 && S_ISDIR(st.st_mode)) {  /* a link to a folder is removed, never followed */
            es_delete_tree(site, full);
        } else if (unlink(full) != 0) {
            es_fail(site, NULL, "I couldn't delete \"%s\": %s.", full, es_reason(errno));
        }
        free(full);
    }
    closedir(dir);
    if (rmdir(path) != 0) es_fail(site, NULL, "I couldn't delete the folder \"%s\": %s.", path, es_reason(errno));
}

/* A path that means more than one folder of the program's own: "", "/", ".", "..", or "~". */
static bool es_too_wide(const char *path) {
    size_t n = strlen(path);
    while (n > 1 && path[n - 1] == '/') n--;
    return n == 0 || (n == 1 && (path[0] == '/' || path[0] == '.' || path[0] == '~')) ||
           (n == 2 && strncmp(path, "..", 2) == 0);
}

/* "delete the file F", "delete the folder F [and everything in it]". Consumes path. */
static void es_delete(int site, EsText *path, bool folder, bool everything) {
    es_need(site, path->chars, folder);
    if (!folder) {
        if (unlink(path->chars) != 0) es_fail(site, NULL, "I couldn't delete the file \"%s\": %s.", path->chars, es_reason(errno));
    } else if (everything) {
        if (es_too_wide(path->chars)) {
            es_fail(site, "Name the folder itself, like \"delete the folder \"old photos\" and everything in it\".",
                    "I won't delete \"%s\" and everything in it, because that's more than one folder of yours.",
                    path->chars);
        }
        es_delete_tree(site, path->chars);
    } else if (rmdir(path->chars) != 0) {
        if (errno == ENOTEMPTY || errno == EEXIST) {
            es_fail(site, "To delete it with everything in it, add \"and everything in it\".",
                    "The folder \"%s\" isn't empty.", path->chars);
        }
        es_fail(site, NULL, "I couldn't delete the folder \"%s\": %s.", path->chars, es_reason(errno));
    }
    es_release(path);
}

/* --- The screen ----------------------------------------------------------------- */

/* "clear the screen": only when the output is a screen, so a file or a test never gets the codes. */
static void es_clear_screen(void) {
    if (!isatty(STDOUT_FILENO)) return;
    fflush(stdout);
    es_out("\033[2J\033[H", 7);
}

#endif
