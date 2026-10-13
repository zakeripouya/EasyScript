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

#include <ctype.h>

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

/* Writes the csv in b to the file (consuming path), then frees b. */
static void es_csv_write(int site, EsBuf *b, EsText *path) {
    FILE *f = fopen(path->chars, "wb");
    if (!f) {
        free(b->data);
        es_fail(site, NULL, "I couldn't write to the file \"%s\": %s.", path->chars,
                errno == ENOENT ? "the folder it should go in doesn't exist" : es_reason(errno));
    }
    bool short_write = b->len && fwrite(b->data, 1, b->len, f) != b->len;
    free(b->data);
    if (fclose(f) != 0 || short_write) {
        es_fail(site, NULL, "I couldn't write to the file \"%s\": %s.", path->chars, es_reason(errno));
    }
    es_release(path);
}

/* A value as a csv field, with dates written like 2026-10-31 (and a time if it has one). */
static void es_csv_cell(EsBuf *b, char kind, EsValue v) {
    if ((kind == 'n' && es_is_blank_n(v.n)) || ((kind == 'u' || kind == 'e' || kind == 'p') && v.n == (double)ES_BLANK_CENTS)) {
        return; /* a table's blank cell is saved blank */
    }
    if (kind == 'u' || kind == 'e' || kind == 'p') { /* "$1200.00": no commas, so the cell needs no quotes */
        static const char *const signs[] = {"$", "\xE2\x82\xAC", "\xC2\xA3"};
        long long cents = (long long)v.n, whole = cents < 0 ? -cents : cents;
        char text[64];
        int n = snprintf(text, sizeof text, "%s%s%lld.%02lld", cents < 0 ? "-" : "",
                         signs[kind == 'u' ? 0 : kind == 'e' ? 1 : 2], whole / 100, whole % 100);
        es_buf_add(b, text, (size_t)n);
        return;
    }
    if (kind != 'd') {
        es_csv_value(b, kind, v);
        return;
    }
    long year;
    int month, day;
    double days = floor(v.n / ES_DAY);
    es_civil_from_days(days, &year, &month, &day);
    long seconds = (long)(v.n - days * ES_DAY);
    long hour = seconds / 3600, minute = seconds % 3600 / 60;
    const char *format = seconds ? "%04ld-%02d-%02d %02ld:%02ld" : "%04ld-%02d-%02d";
    int n = snprintf(NULL, 0, format, year, month, day, hour, minute);
    char *text = malloc((size_t)n + 1);
    if (!text) es_out_of_memory();
    snprintf(text, (size_t)n + 1, format, year, month, day, hour, minute);
    es_buf_add(b, text, (size_t)n);
    free(text);
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
    es_csv_write(site, &b, path);
    es_list_release(rows);
}

/* --- "read the table in F": rows as records ------------------------------------------ */

/* How many letters to change to turn a into b (for "Did you mean"). */
static size_t es_distance(const char *a, size_t n, const char *b) {
    size_t m = strlen(b);
    size_t *row = malloc((m + 1) * sizeof *row);
    if (!row) es_out_of_memory();
    for (size_t j = 0; j <= m; j++) row[j] = j;
    for (size_t i = 1; i <= n; i++) {
        size_t diagonal = row[0];
        row[0] = i;
        for (size_t j = 1; j <= m; j++) {
            size_t above = row[j];
            size_t change = diagonal + (tolower((unsigned char)a[i - 1]) != tolower((unsigned char)b[j - 1]));
            row[j] = change < above + 1 ? change : above + 1;
            if (row[j - 1] + 1 < row[j]) row[j] = row[j - 1] + 1;
            diagonal = above;
        }
    }
    size_t d = row[m];
    free(row);
    return d;
}

/* A field the file has no column for: the program stops, naming the closest column. */
static _Noreturn void es_no_column(int site, const EsCsv *r, const EsList *header, const char *field) {
    static char *hint; /* kept, so it's still reachable when the program stops */
    const EsText *best = NULL;
    size_t best_d = strlen(field) <= 3 ? 2 : 3;
    for (size_t i = 0; i < header->len; i++) {
        size_t d = es_distance(header->at.t[i]->chars, header->at.t[i]->len, field);
        if (d < best_d) best_d = d, best = header->at.t[i];
    }
    if (best) {
        int n = snprintf(NULL, 0, "Did you mean \"%.*s\"?", (int)best->len, best->chars);
        hint = malloc((size_t)n + 1);
        if (!hint) es_out_of_memory();
        snprintf(hint, (size_t)n + 1, "Did you mean \"%.*s\"?", (int)best->len, best->chars);
    }
    es_fail(site, hint, "%s has no column called \"%s\".", r->name, field);
}

/* What a cell that doesn't fit its column holds instead, if it's a number or
 * an amount ("an amount in euros"), or NULL. */
static const char *es_cell_holds(const EsText *cell) {
    static const char *const amounts[] = {"an amount in dollars", "an amount in euros", "an amount in pounds"};
    static const char *const signs[] = {"$", "\xE2\x82\xAC", "\xC2\xA3"};
    static const char *const words[][2] = {{"dollars", "dollar"}, {"euros", "euro"}, {"pounds", "pound"}};
    long long cents;
    double number;
    size_t end = cell->len;
    while (end > 0 && cell->chars[end - 1] == ' ') end--;
    for (int k = 0; k < 3; k++) {  /* written with its sign or its word */
        bool marked = strstr(cell->chars, signs[k]) != NULL;
        for (int w = 0; w < 2 && !marked; w++) {
            size_t n = strlen(words[k][w]);
            marked = end >= n && es_word_is(cell->chars + end - n, n, words[k][w]);
        }
        if (marked && es_parse_money(cell->chars, cell->len, k, &cents)) return amounts[k];
    }
    return es_parse_number(cell->chars, cell->len, &number) ? "a number" : NULL;
}

/* A cell read as its field's kind: 't' text, 'n' a number, 'b' yes or no, 'd' a
 * date (2026-10-31, maybe with a time, 2026-10-31 14:05), 'u' 'e' 'p' an
 * amount. A blank cell in a column of numbers or amounts is nothing (the
 * blank marks in es_runtime.h), as a spreadsheet leaves it out of totals.
 * Takes the cell. */
static void es_table_cell(const EsCsv *r, long line, const char *field, char kind, EsText *cell, char *slot) {
    if (kind == 't') {
        *(EsText **)slot = cell;
        return;
    }
    size_t blank = 0;
    while (blank < cell->len && cell->chars[blank] == ' ') blank++;
    if (blank == cell->len && (kind == 'n' || kind == 'u' || kind == 'e' || kind == 'p')) { /* nothing: skipped by totals */
        if (kind == 'n') *(double *)slot = es_blank_number();
        if (kind != 'n') *(long long *)slot = ES_BLANK_CENTS;
        es_release(cell);
        return;
    }
    const char *what = kind == 'n' ? "a number" : kind == 'b' ? "yes or no" : kind == 'd' ? "a date like 2026-10-31"
                       : kind == 'u' ? "an amount in dollars" : kind == 'e' ? "an amount in euros" : "an amount in pounds";
    bool ok = false;
    if (kind == 'u' || kind == 'e' || kind == 'p') {
        ok = es_parse_money(cell->chars, cell->len, kind == 'u' ? 0 : kind == 'e' ? 1 : 2, (long long *)slot);
    } else if (kind == 'n') {
        ok = es_parse_number(cell->chars, cell->len, (double *)slot);
    } else if (kind == 'b') {
        bool yes = es_column_is(cell, "yes") || es_column_is(cell, "true");
        ok = yes || es_column_is(cell, "no") || es_column_is(cell, "false");
        *(bool *)slot = yes;
    } else {
        int y, mo, d, h = 0, mi = 0, sec = 0, used = 0;
        int got = sscanf(cell->chars, "%d-%d-%d%n %d:%d%n:%d%n", &y, &mo, &d, &used, &h, &mi, &used, &sec, &used);
        ok = got >= 3 && (size_t)used == cell->len && mo >= 1 && mo <= 12 && d >= 1 && d <= 31 && h < 24 && mi < 60;
        *(double *)slot = es_days_from_civil(y, mo, d) * ES_DAY + h * 3600 + mi * 60 + sec;
    }
    if (ok) {
        es_release(cell);
        return;
    }
    const char *holds = kind == 'n' || kind == 'u' || kind == 'e' || kind == 'p' ? es_cell_holds(cell) : NULL;
    if (holds) {
        es_fail(r->site, NULL, "In %s, line %ld, \"%s\" is \"%.*s\", %s, but the program uses it as %s.", r->name, line,
                field, (int)cell->len, cell->chars, holds, what);
    }
    es_fail(r->site, NULL, "In %s, line %ld, \"%s\" is \"%.*s\", which isn't %s.", r->name, line, field, (int)cell->len,
            cell->chars, what);
}

/* What "read the table in F" is building, kept here so it's still reachable
 * when a mistake in the file stops the program (nothing is lost at es_fail). */
static struct {
    EsCsv csv;
    EsText *path, *text;
    EsList *header, *fields, *rows;
    size_t *column;
} es_table_read;

/* "read the table in F": every row as a record of `type`, its fields read
 * from the columns with their names. Consumes path. */
static EsList *es_read_table(int site, EsText *path, const EsRecordType *type) {
    es_table_read.path = path;
    const char *slash = strrchr(path->chars, '/');
    EsText *text = es_table_read.text = es_read_file(site, es_retain(path));
    EsCsv *r = &es_table_read.csv;
    *r = (EsCsv){site, slash ? slash + 1 : path->chars, text->chars, text->len, 0, 1, {NULL, 0, 0}};
    if (text->len >= 4 && memcmp(text->chars, "PK\003\004", 4) == 0) { /* a workbook (a zip file), not a csv file */
        es_fail(site, "Save the sheet as a csv file in Excel (\"Save As\", then \"CSV\"), and read that.",
                "EasyScript can't read Excel files yet, and %s is one, not a csv file.", r->name);
    }
    EsList *header = es_table_read.header = es_list_new(ES_ITEMS_TEXT, 0);
    EsList *fields = es_table_read.fields = es_list_new(ES_ITEMS_TEXT, 0);
    long line;
    if (!es_csv_row(r, header, &line)) es_fail(site, NULL, "%s is empty, so it has no header row.", r->name);
    size_t *column = es_table_read.column = malloc(((size_t)type->fields + 1) * sizeof *column);
    if (!column) es_out_of_memory();
    for (int f = 2; f < type->fields; f++) {  /* (fields 0 and 1 keep the header and the row's cells) */
        column[f] = header->len;
        for (size_t i = 0; i < header->len && column[f] == header->len; i++) {
            if (es_column_is(header->at.t[i], type->field_names[f])) column[f] = i;
        }
        if (column[f] == header->len) es_no_column(site, r, header, type->field_names[f]);
    }
    EsList *rows = es_table_read.rows = es_list_new(ES_ITEMS_RECORD, 0);
    while (es_csv_row(r, fields, &line)) {
        if (fields->len != header->len) {
            es_fail(site, NULL, "In %s, line %ld has %zu value%s, but the header has %zu.", r->name, line, fields->len,
                    fields->len == 1 ? "" : "s", header->len);
        }
        EsRecord *row = es_record_new(type);
        memset((char *)row + sizeof(EsRecord), 0, type->size - sizeof(EsRecord));
        es_push_r(rows, row);
        ES_FIELD(row, EsList *, type->offsets[0]) = es_list_retain(header);
        EsList *cells = es_list_new(ES_ITEMS_TEXT, fields->len);
        for (size_t i = 0; i < fields->len; i++) es_push_t(cells, es_retain(fields->at.t[i]));
        ES_FIELD(row, EsList *, type->offsets[1]) = cells;
        for (int f = 2; f < type->fields; f++) {
            es_table_cell(r, line, type->field_names[f], type->kinds[f], es_retain(fields->at.t[column[f]]),
                          (char *)row + type->offsets[f]);
        }
        es_csv_clear(fields);
    }
    free(column);
    free(r->field.data);
    es_list_release(header); /* (the rows keep it) */
    es_list_release(fields);
    es_release(text);
    es_release(path);
    memset(&es_table_read, 0, sizeof es_table_read);
    return rows;
}

/* "save totals as "totals.csv"" for a map: a "key" and a "value" column, a
 * row for each entry in order. `keys` and `values` are the kinds ('n', 't',
 * 'b'; dates 'd'). Consumes m and path. */
static void es_save_map(int site, EsMap *m, EsText *path, char keys, char values) {
    EsBuf b = {NULL, 0, 0};
    es_buf_add(&b, "key,value\n", 10);
    for (size_t i = 0; i < m->used; i++) {
        if (!m->entries[i].hash) continue;
        es_csv_cell(&b, keys, m->entries[i].key);
        es_buf_add(&b, ",", 1);
        es_csv_cell(&b, values, m->entries[i].value);
        es_buf_add(&b, "\n", 1);
    }
    es_csv_write(site, &b, path);
    es_map_release(m);
}

/* "save sales as "out.csv"" for a list of records: a column for each field
 * (numbers, text, yes/no, dates). Consumes rows and path. */
static void es_save_field(EsBuf *b, const EsRecord *row, int f) {
    const char *at = (const char *)row + row->type->offsets[f];
    EsValue v = {0};
    char kind = row->type->kinds[f];
    if (kind == 't') v.t = *(EsText *const *)at;
    if (kind == 'n' || kind == 'd') v.n = *(const double *)at;
    if (kind == 'u' || kind == 'e' || kind == 'p') v.n = (double)*(const long long *)at;
    if (kind == 'b') v.b = *(const bool *)at;
    if (kind != 'l' && kind != 'r' && kind != 'm') es_csv_cell(b, kind, v);
}

static void es_save_records(int site, EsList *rows, EsText *path, const EsRecordType *type) {
    EsBuf b = {NULL, 0, 0};
    bool table = type->fields >= 2 && strcmp(type->field_names[0], "0header") == 0;
    const EsList *header = table && rows->len && rows->at.r[0] ? ES_FIELD(rows->at.r[0], EsList *, type->offsets[0]) : NULL;
    if (header) { /* a table's rows: every column of the file, with the program's values for the ones it uses */
        for (size_t c = 0; c < header->len; c++) {
            if (c) es_buf_add(&b, ",", 1);
            es_csv_add(&b, header->at.t[c]->chars, header->at.t[c]->len);
        }
    }
    for (int f = table ? 2 : 0; !header && f < type->fields; f++) {
        if (f > (table ? 2 : 0)) es_buf_add(&b, ",", 1);
        es_csv_add(&b, type->field_names[f], strlen(type->field_names[f]));
    }
    es_buf_add(&b, "\n", 1);
    for (size_t r = 0; r < rows->len; r++) {
        const EsRecord *row = rows->at.r[r];
        if (!row) continue;
        if (header) {
            const EsList *cells = ES_FIELD(row, EsList *, type->offsets[1]);
            for (size_t c = 0; c < header->len; c++) {
                if (c) es_buf_add(&b, ",", 1);
                int field = -1;
                for (int f = 2; f < type->fields && field < 0; f++) {
                    if (es_column_is(header->at.t[c], type->field_names[f])) field = f;
                }
                if (field >= 0) {
                    es_save_field(&b, row, field);
                } else if (c < cells->len) {
                    es_csv_add(&b, cells->at.t[c]->chars, cells->at.t[c]->len);
                }
            }
        }
        for (int f = table ? 2 : 0; !header && f < type->fields; f++) {
            if (f > (table ? 2 : 0)) es_buf_add(&b, ",", 1);
            es_save_field(&b, row, f);
        }
        es_buf_add(&b, "\n", 1);
    }
    es_csv_write(site, &b, path);
    es_list_release(rows);
}

#endif
