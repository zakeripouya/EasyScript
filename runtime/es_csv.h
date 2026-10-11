/* EasyScript runtime, part 14: csv files.
 *
 * "read csv file F and call it rows" gives a list of rows, each a map from
 * the column's name (from the header, the first row) to its text, in the
 * header's order. Fields can be quoted ("..."), with commas, line breaks
 * and doubled quotes ("") inside; lines can end with \n or \r\n; blank lines
 * are skipped. "save rows as csv file F" writes the columns of the first
 * row's keys, in order, quoting fields that need it. */
#ifndef ES_CSV_H
#define ES_CSV_H

#ifndef ES_FILES_H
#include "es_files.h"
#endif

typedef struct {
    int site;
    const char *name;  /* the file's name, for messages */
    const char *p;
    size_t len, pos;
    long line;         /* of the file, from 1 */
    EsBuf field;
} EsCsv;

/* One field into r->field; returns the character that ended it (',' '\n' or 0 at the end). */
static char es_csv_field(EsCsv *r) {
    r->field.len = 0;
    if (r->pos < r->len && r->p[r->pos] == '"') {
        long start = r->line;
        r->pos++;
        for (;;) {
            if (r->pos >= r->len) {
                es_fail(r->site, NULL, "In %s, the quote on line %ld never ends.", r->name, start);
            }
            char c = r->p[r->pos++];
            if (c == '"') {
                if (r->pos < r->len && r->p[r->pos] == '"') {
                    r->pos++;
                } else {
                    break;
                }
            } else if (c == '\n') {
                r->line++;
            }
            es_buf_add(&r->field, &c, 1);
        }
        if (r->pos < r->len && r->p[r->pos] != ',' && r->p[r->pos] != '\n' && r->p[r->pos] != '\r') {
            es_fail(r->site, "A quoted field ends with its quote, then a comma or the end of the line.",
                    "In %s, line %ld has more after a closing quote.", r->name, r->line);
        }
    } else {
        size_t start = r->pos;
        while (r->pos < r->len && r->p[r->pos] != ',' && r->p[r->pos] != '\n' && r->p[r->pos] != '\r') r->pos++;
        es_buf_add(&r->field, r->p + start, r->pos - start);
    }
    if (r->pos >= r->len) return 0;
    char c = r->p[r->pos++];
    if (c == '\r') {
        if (r->pos < r->len && r->p[r->pos] == '\n') r->pos++;
        c = '\n';
    }
    if (c == '\n') r->line++;
    return c;
}

/* The fields of the next row (skipping blank lines) into *fields; false at the end. *line is where it starts. */
static bool es_csv_row(EsCsv *r, EsList *fields, long *line) {
    while (r->pos < r->len && (r->p[r->pos] == '\n' || r->p[r->pos] == '\r')) {
        if (r->p[r->pos] == '\n') r->line++;
        r->pos++;
    }
    if (r->pos >= r->len) return false;
    *line = r->line;
    char end;
    do {
        end = es_csv_field(r);
        es_push_t(fields, es_text_copy(r->field.data ? r->field.data : "", r->field.len));
    } while (end == ',');
    return true;
}

static void es_csv_clear(EsList *fields) {
    for (size_t i = 0; i < fields->len; i++) es_release(fields->at.t[i]);
    fields->len = 0;
}

static EsList *es_read_csv(int site, EsText *path) {
    const char *slash = strrchr(path->chars, '/');
    EsText *text = es_read_file(site, es_retain(path));
    EsCsv r = {site, slash ? slash + 1 : path->chars, text->chars, text->len, 0, 1, {NULL, 0, 0}};
    EsList *header = es_list_new(ES_ITEMS_TEXT, 0), *fields = es_list_new(ES_ITEMS_TEXT, 0);
    long line;
    if (!es_csv_row(&r, header, &line)) es_fail(site, NULL, "%s is empty, so it has no header row.", r.name);
    for (size_t i = 0; i < header->len; i++) {
        for (size_t j = 0; j < i; j++) {
            if (es_text_equal(header->at.t[i], header->at.t[j])) {
                es_fail(site, "Each column needs its own name.", "The header of %s has the column \"%s\" twice.",
                        r.name, header->at.t[i]->chars);
            }
        }
    }
    EsList *rows = es_list_new(ES_ITEMS_MAP, 0);
    while (es_csv_row(&r, fields, &line)) {
        if (fields->len != header->len) {
            es_fail(site, NULL, "In %s, line %ld has %zu value%s, but the header has %zu.", r.name, line, fields->len,
                    fields->len == 1 ? "" : "s", header->len);
        }
        EsMap *row = es_map_new(ES_ITEMS_TEXT, ES_ITEMS_TEXT, header->len);
        for (size_t i = 0; i < header->len; i++) {
            EsValue key = {.t = es_retain(header->at.t[i])}, value = {.t = fields->at.t[i]};
            es_map_store(row, key, value);
        }
        fields->len = 0; /* the row took them */
        es_push_m(rows, row);
    }
    free(r.field.data);
    es_csv_clear(header);
    es_list_release(header);
    es_list_release(fields);
    es_release(text);
    es_release(path);
    return rows;
}

/* A field as csv: quoted when it holds a comma, a quote or a line break, or starts or ends with a space. */
static void es_csv_add(EsBuf *b, const char *s, size_t n) {
    bool quote = n > 0 && (s[0] == ' ' || s[n - 1] == ' ');
    for (size_t i = 0; i < n && !quote; i++) quote = s[i] == ',' || s[i] == '"' || s[i] == '\n' || s[i] == '\r';
    if (!quote) {
        es_buf_add(b, s, n);
        return;
    }
    es_buf_add(b, "\"", 1);
    for (size_t i = 0; i < n; i++) {
        es_buf_add(b, s + i, 1);
        if (s[i] == '"') es_buf_add(b, "\"", 1); /* a quote inside is doubled */
    }
    es_buf_add(b, "\"", 1);
}

static void es_csv_value(EsBuf *b, char kind, EsValue v) {
    if (kind == 't') {
        es_csv_add(b, v.t->chars, v.t->len);
    } else if (kind == 'b') {
        es_buf_add(b, v.b ? "yes" : "no", v.b ? 3 : 2);
    } else {
        EsText *t = es_number_text(v.n);
        es_csv_add(b, t->chars, t->len);
        es_release(t);
    }
}

/* "save rows as csv file F": `kind` is the values' ('n', 't' or 'b'). Consumes rows and path. */
static void es_save_csv(int site, EsList *rows, EsText *path, char kind) {
    EsBuf b = {NULL, 0, 0};
    const EsMap *first = rows->len ? rows->at.m[0] : NULL;
    size_t columns = first ? first->len : 0;
    for (size_t i = 0, c = 0; first && i < first->used; i++) {
        if (!first->entries[i].hash) continue;
        if (c++) es_buf_add(&b, ",", 1);
        es_csv_add(&b, first->entries[i].key.t->chars, first->entries[i].key.t->len);
    }
    if (first) es_buf_add(&b, "\n", 1);
    for (size_t r = 0; r < rows->len; r++) {
        EsMap *row = rows->at.m[r];
        if (row->len != columns) {
            es_fail(site, NULL, "Row %zu of the list has %zu value%s, but the first row has %zu.", r + 1, row->len,
                    row->len == 1 ? "" : "s", columns);
        }
        for (size_t i = 0, c = 0; i < first->used; i++) {
            if (!first->entries[i].hash) continue;
            EsValue key = first->entries[i].key;
            size_t at = es_map_find(row, key, es_key_hash(row, key));
            if (at == ES_NOT_FOUND) {
                es_fail(site, NULL, "Row %zu of the list has no value for \"%s\", but the first row does.", r + 1,
                        key.t->chars);
            }
            if (c++) es_buf_add(&b, ",", 1);
            es_csv_value(&b, kind, row->entries[at].value);
        }
        es_buf_add(&b, "\n", 1);
    }
    FILE *f = fopen(path->chars, "wb");
    if (!f) {
        es_fail(site, NULL, "I couldn't write to the file \"%s\": %s.", path->chars,
                errno == ENOENT ? "the folder it should go in doesn't exist" : es_reason(errno));
    }
    bool short_write = b.len && fwrite(b.data, 1, b.len, f) != b.len;
    if (fclose(f) != 0 || short_write) {
        es_fail(site, NULL, "I couldn't write to the file \"%s\": %s.", path->chars, es_reason(errno));
    }
    free(b.data);
    es_list_release(rows);
    es_release(path);
}

#endif
