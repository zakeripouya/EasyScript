// A table's columns judged from its file, before the program runs. A column
// of "read the table in "bank.csv"" gets its kind from how the program uses
// it; when that use leaves it as "a number unless something says otherwise"
// (a total, arithmetic, a chart), the file decides: amounts in one currency
// make it money, numbers make it a number. The file is the one in the folder
// the program runs in or, when there's none yet, the text the program itself
// writes into a file of that name. Two reads of the same file use each column
// the same way. Once every kind is known, a column the program uses as a
// number or an amount is checked against the file's cells, so a mistake is
// reported before the program runs (the runtime checks each cell again, for
// files that weren't there or have changed).

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include "front/check_internal.h"

typedef struct {
    const char *text;
    size_t len;
} Cell;

typedef struct {
    long line;  // where the row starts in the file
    Vec(Cell) cells;
} Row;

// A table file looked at before the program runs, with the reads of it.
struct TableFile {
    const char *name;     // as written in quotes
    const char *shown;    // without its folders, as the runtime's messages name it
    Span at;              // where the first read names it
    Vec(size_t) kinds;    // the kinds of row of every read of it (indexes into the records)
    bool found;           // its text was found (on disk, or written by the program)
    Row header;
    Vec(Row) rows;
};

typedef enum { CELL_BLANK, CELL_NUMBER, CELL_MONEY, CELL_TEXT } CellClass;

// --- Reading the file --------------------------------------------------------------------

// The whole file at `path` if it's a file that can be read, or NULL.
static const char *read_whole(Checker *c, const char *path, size_t *len) {
    struct stat st;
    if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) return NULL;
    FILE *file = fopen(path, "rb");
    if (!file) return NULL;
    size_t size = (size_t)st.st_size;
    char *data = arena_alloc(c->arena, size + 1);
    size_t got = fread(data, 1, size, file);
    fclose(file);
    data[got] = '\0';
    *len = got;
    return data;
}

typedef struct {
    const char *p;
    size_t len, pos;
    long line;
    bool broken;  // a quote that never ends, or text after a closing quote: the runtime says so
} Csv;

// One field into *cell, read like the runtime's es_csv_field. Gives back
// what ended it: ',', '\n', or 0 at the end of the file.
static char csv_field(Checker *c, Csv *r, Cell *cell) {
    StrBuf field;
    sb_init(&field, c->arena);
    if (r->pos < r->len && r->p[r->pos] == '"') {
        r->pos++;
        for (;;) {
            if (r->pos >= r->len) {
                r->broken = true;
                return 0;
            }
            char ch = r->p[r->pos++];
            if (ch == '"') {
                if (r->pos < r->len && r->p[r->pos] == '"') {
                    r->pos++;
                } else {
                    break;
                }
            } else if (ch == '\n') {
                r->line++;
            }
            sb_append_char(&field, ch);
        }
        if (r->pos < r->len && r->p[r->pos] != ',' && r->p[r->pos] != '\n' && r->p[r->pos] != '\r') r->broken = true;
    } else {
        size_t start = r->pos;
        while (r->pos < r->len && r->p[r->pos] != ',' && r->p[r->pos] != '\n' && r->p[r->pos] != '\r') r->pos++;
        sb_append_n(&field, r->p + start, r->pos - start);
    }
    cell->text = field.data;
    cell->len = field.len;
    if (r->pos >= r->len) return 0;
    char end = r->p[r->pos++];
    if (end == '\r') {
        if (r->pos < r->len && r->p[r->pos] == '\n') r->pos++;
        end = '\n';
    }
    if (end == '\n') r->line++;
    return end;
}

// The next row (blank lines skipped); false at the end of the file.
static bool csv_row(Checker *c, Csv *r, Row *row) {
    while (r->pos < r->len && (r->p[r->pos] == '\n' || r->p[r->pos] == '\r')) {
        if (r->p[r->pos] == '\n') r->line++;
        r->pos++;
    }
    if (r->pos >= r->len || r->broken) return false;
    row->line = r->line;
    row->cells.items = NULL;
    row->cells.len = row->cells.cap = 0;
    char end;
    do {
        Cell cell;
        end = csv_field(c, r, &cell);
        vec_push(c->arena, &row->cells, cell);
    } while (end == ',' && !r->broken);
    return !r->broken;
}

static void load(Checker *c, TableFile *t, const char *written) {
    size_t len = 0;
    const char *text = read_whole(c, t->name, &len);
    if (!text && written) text = written, len = strlen(written);
    if (!text) return;
    Csv r = {text, len, 0, 1, false};
    if (!csv_row(c, &r, &t->header)) return;
    Row row;
    while (csv_row(c, &r, &row)) {
        if (row.cells.len == t->header.cells.len) vec_push(c->arena, &t->rows, row);
    }
    t->found = !r.broken;
}

// --- What a cell is ----------------------------------------------------------------------

static bool word_is(const char *s, size_t len, const char *word) {
    if (len != strlen(word)) return false;
    for (size_t i = 0; i < len; i++) {
        if ((s[i] | 0x20) != word[i]) return false;
    }
    return true;
}

// An amount written with its currency, the way the runtime reads one
// (es_parse_money): a sign before it ($, €, £) or a word after it (dollars,
// euros, pounds), commas between thousands, at most two decimals. Gives back
// the currency (0 dollars, 1 euros, 2 pounds), or -1.
static int amount_currency(const char *s, size_t len) {
    static const char *const signs[] = {"$", "\xE2\x82\xAC", "\xC2\xA3"};
    static const char *const words[][2] = {{"dollars", "dollar"}, {"euros", "euro"}, {"pounds", "pound"}};
    const char *end = s + len;
    while (s < end && *s == ' ') s++;
    while (end > s && end[-1] == ' ') end--;
    if (s < end && *s == '-') s++;
    int sign = -1, word = -1;
    for (int k = 0; k < 3 && sign < 0; k++) {
        size_t n = strlen(signs[k]);
        if ((size_t)(end - s) >= n && memcmp(s, signs[k], n) == 0) sign = k, s += n;
    }
    double whole = 0;
    int digits = 0, decimals = 0;
    for (; s < end && ((*s >= '0' && *s <= '9') || *s == ','); s++) {
        if (*s != ',') whole = whole * 10 + (*s - '0'), digits++;
    }
    if (s < end && *s == '.') {
        for (s++; s < end && *s >= '0' && *s <= '9' && decimals < 2; s++) decimals++;
    }
    const char *after = s;
    while (s < end && *s == ' ') s++;
    for (int k = 0; k < 3 && word < 0 && s < end; k++) {
        if (word_is(s, (size_t)(end - s), words[k][0]) || word_is(s, (size_t)(end - s), words[k][1])) word = k;
    }
    if (word >= 0) s = end;
    if (word < 0) s = after;
    if (s != end || digits == 0 || whole * 100 >= 9007199254740992.0) return -1;
    if (sign >= 0 && word >= 0 && sign != word) return -1;
    return sign >= 0 ? sign : word;
}

// A number, the way the runtime reads one (es_parse_number).
static bool is_number(const char *s, size_t len) {
    size_t i = 0, end = len;
    while (i < end && s[i] == ' ') i++;
    while (end > i && s[end - 1] == ' ') end--;
    if (i < end && s[i] == '-') i++;
    size_t digits = i;
    while (i < end && s[i] >= '0' && s[i] <= '9') i++;
    if (i == digits) return false;
    if (i < end && s[i] == '.') {
        size_t frac = ++i;
        while (i < end && s[i] >= '0' && s[i] <= '9') i++;
        if (i == frac) return false;
    }
    if (i < end && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        if (i < end && (s[i] == '-' || s[i] == '+')) i++;
        size_t power = i;
        while (i < end && s[i] >= '0' && s[i] <= '9') i++;
        if (i == power) return false;
    }
    return i == end;
}

static CellClass classify(const Cell *cell, int *currency) {
    size_t i = 0;
    while (i < cell->len && cell->text[i] == ' ') i++;
    if (i == cell->len) return CELL_BLANK;
    *currency = amount_currency(cell->text, cell->len);
    if (*currency >= 0) return CELL_MONEY;
    return is_number(cell->text, cell->len) ? CELL_NUMBER : CELL_TEXT;
}

static Type money_type(int currency) {
    return currency == 0 ? TYPE_DOLLARS : currency == 1 ? TYPE_EUROS : TYPE_POUNDS;
}

// "an amount in dollars", "a number", or NULL for text.
static const char *cell_kind(Checker *c, const Cell *cell) {
    int currency = -1;
    CellClass class = classify(cell, &currency);
    if (class == CELL_MONEY) return checker_type_name(c, money_type(currency));
    return class == CELL_NUMBER ? "a number" : NULL;
}

// --- Columns -----------------------------------------------------------------------------

// Whether a header's column is a field's name: the same letters, capitals or
// not, with "_" for a space (the runtime's es_column_is).
static bool column_is(const Cell *column, const char *field) {
    size_t n = strlen(field);
    if (column->len != n) return false;
    for (size_t i = 0; i < n; i++) {
        char x = column->text[i] == '_' ? ' ' : column->text[i];
        if ((x | 0x20) != (field[i] | 0x20)) return false;
    }
    return true;
}

static long column_of(const TableFile *t, const char *field) {
    for (size_t i = 0; i < t->header.cells.len; i++) {
        if (column_is(&t->header.cells.items[i], field)) return (long)i;
    }
    return -1;
}

// The column's kind, as its cells say: amounts in one currency (plain numbers
// may be among them), numbers, or neither (text, or two currencies). *mixed
// is the first row in a second currency, and *first the first amount.
static Type judge(const TableFile *t, long column, const Row **first, const Row **mixed) {
    int currency = -1;
    bool numbers = false;
    *first = *mixed = NULL;
    for (size_t r = 0; r < t->rows.len; r++) {
        const Row *row = &t->rows.items[r];
        int k = -1;
        CellClass class = classify(&row->cells.items[column], &k);
        if (class == CELL_TEXT) return TYPE_TEXT;
        if (class == CELL_NUMBER) numbers = true;
        if (class != CELL_MONEY) continue;
        if (currency < 0) currency = k, *first = row;
        if (k != currency && !*mixed) *mixed = row;
    }
    if (*mixed) return TYPE_ERROR;
    return currency >= 0 ? money_type(currency) : numbers ? TYPE_NUMBER : TYPE_UNKNOWN;
}

// The first row whose cell isn't `kind` (a number, or an amount in one
// currency, where a plain number is fine too). Blank cells fit.
static const Row *misfit(const TableFile *t, long column, Type kind) {
    for (size_t r = 0; r < t->rows.len; r++) {
        const Row *row = &t->rows.items[r];
        int k = -1;
        CellClass class = classify(&row->cells.items[column], &k);
        if (class == CELL_TEXT || (class == CELL_MONEY && (kind == TYPE_NUMBER || money_type(k) != kind))) return row;
    }
    return NULL;
}

static const char *quoted_cell(Checker *c, const Cell *cell) {
    return arena_sprintf(c->arena, "%.*s", (int)cell->len, cell->text);
}

static void report_mixed(Checker *c, const TableFile *t, const Field *field, long column, const Row *first,
                         const Row *mixed) {
    const Cell *a = &first->cells.items[column], *b = &mixed->cells.items[column];
    diag_error(c->diag, t->at, "In %s, \"%s\" has amounts in two currencies: line %ld is \"%s\" and line %ld is \"%s\".",
               t->shown, field->name.display, first->line, quoted_cell(c, a), mixed->line, quoted_cell(c, b));
    diag_note(c->diag, "Write every amount in it in one currency.");
    diag_info(c->diag, DIAG_WRONG_KIND, NULL, "amounts in one currency");
}

static void report_misfit(Checker *c, const TableFile *t, const Field *field, long column, const Row *row,
                          TypeVar var) {
    const Cell *cell = &row->cells.items[column];
    const char *has = cell_kind(c, cell), *uses = checker_kind(c, var);
    if (has) {
        diag_error(c->diag, t->at, "In %s, line %ld, \"%s\" is \"%s\", %s, but the program uses it as %s.", t->shown,
                   row->line, field->name.display, quoted_cell(c, cell), has, uses);
    } else {
        diag_error(c->diag, t->at, "In %s, line %ld, \"%s\" is \"%s\", which isn't %s.", t->shown, row->line,
                   field->name.display, quoted_cell(c, cell), uses);
    }
    diag_note(c->diag, "Line %zu uses \"%s\" as %s.", types_line(c, var), field->name.display, uses);
    diag_info(c->diag, DIAG_WRONG_KIND, NULL, uses);
}

// --- Before and after the kinds nothing decided ------------------------------------------

static TableFile *file_named(Checker *c, const char *name) {
    for (size_t i = 0; i < c->table_files.len; i++) {
        if (strcmp(c->table_files.items[i]->name, name) == 0) return c->table_files.items[i];
    }
    return NULL;
}

// Two reads of one file: each column is one kind for both.
static void link_reads(Checker *c, TableFile *t, size_t later) {
    const RecordKind *kind = &c->records->items[later];
    Stmt *def = kind->definition;
    for (size_t k = 0; k + 1 < t->kinds.len; k++) {
        Stmt *earlier = c->records->items[t->kinds.items[k]].definition;
        for (size_t f = 2; f < def->as.record.fields.len; f++) {
            Field *field = &def->as.record.fields.items[f];
            for (size_t g = 2; g < earlier->as.record.fields.len; g++) {
                Field *other = &earlier->as.record.fields.items[g];
                if (strcmp(field->name.text, other->name.text) != 0) continue;
                TypeVar a = checker_field_kind(c, other, earlier->pos.line), b = checker_field_kind(c, field, def->pos.line);
                if (types_unify(c, a, b)) continue;
                diag_error(c->diag, (Span){kind->file_offset, kind->file_length}, "This reads %s again, but uses \"%s\" as %s. Line %zu reads it too, "
                           "and uses it as %s.", t->shown, field->name.display, checker_kind(c, b), earlier->pos.line,
                           checker_kind(c, a));
                diag_note(c->diag, "Use each column the same way every time the file is read.");
                diag_info(c->diag, DIAG_WRONG_KIND, NULL, checker_kind(c, a));
            }
        }
    }
}

// Finds every table read from a file named in quotes, links the reads of
// one file, and gives each column the program leaves as "a number unless
// something says otherwise" the kind its cells have.
void checker_judge_columns(Checker *c) {
    for (size_t k = 0; k < c->records->len; k++) {
        const RecordKind *kind = &c->records->items[k];
        if (!kind->table || !kind->file) continue;
        TableFile *t = file_named(c, kind->file);
        if (!t) {
            t = arena_alloc(c->arena, sizeof *t);
            t->name = kind->file;
            const char *slash = strrchr(kind->file, '/');
            t->shown = slash ? slash + 1 : kind->file;
            t->at = (Span){kind->file_offset, kind->file_length};
            load(c, t, kind->written);
            vec_push(c->arena, &c->table_files, t);
        }
        vec_push(c->arena, &t->kinds, k);
        if (t->kinds.len > 1) link_reads(c, t, k);
    }
    for (size_t i = 0; i < c->table_files.len; i++) {
        TableFile *t = c->table_files.items[i];
        if (!t->found) continue;
        for (size_t k = 0; k < t->kinds.len; k++) {
            Stmt *def = c->records->items[t->kinds.items[k]].definition;
            for (size_t f = 2; f < def->as.record.fields.len; f++) {
                Field *field = &def->as.record.fields.items[f];
                long column = column_of(t, field->name.text);
                TypeVar var = checker_field_kind(c, field, def->pos.line);
                if (column < 0 || types_of(c, var) != TYPE_UNKNOWN || !types_number_first(c, var)) continue;
                const Row *first, *mixed;
                Type kind = judge(t, column, &first, &mixed);
                if (kind == TYPE_ERROR) {
                    report_mixed(c, t, field, column, first, mixed);
                    types_require(c, var, TYPE_ERROR, def->pos.line);
                } else if (kind == TYPE_NUMBER || type_is_money(kind)) {
                    types_require(c, var, kind, def->pos.line);
                }
            }
        }
    }
}

// Once every kind is known: a column the program uses as a number or an
// amount whose cells say otherwise is an error, at the first read of its file.
void checker_check_columns(Checker *c) {
    for (size_t i = 0; i < c->table_files.len; i++) {
        TableFile *t = c->table_files.items[i];
        if (!t->found) continue;
        Vec(const char *) done = {0};
        for (size_t k = 0; k < t->kinds.len; k++) {
            Stmt *def = c->records->items[t->kinds.items[k]].definition;
            for (size_t f = 2; f < def->as.record.fields.len; f++) {
                Field *field = &def->as.record.fields.items[f];
                bool seen = false;
                for (size_t d = 0; d < done.len && !seen; d++) seen = strcmp(done.items[d], field->name.text) == 0;
                if (seen) continue;
                vec_push(c->arena, &done, field->name.text);
                long column = column_of(t, field->name.text);
                TypeVar var = checker_field_kind(c, field, def->pos.line);
                Type kind = types_of(c, var);
                if (column < 0 || (kind != TYPE_NUMBER && !type_is_money(kind))) continue;
                const Row *row = misfit(t, column, kind);
                if (row) report_misfit(c, t, field, column, row, var);
            }
        }
    }
}
