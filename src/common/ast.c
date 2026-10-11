#include <string.h>
#include "common/ast.h"

Expr *ast_new_expr(Arena *arena, ExprKind kind, SourcePos pos) {
    Expr *expr = arena_alloc(arena, sizeof(Expr));
    expr->kind = kind;
    expr->pos = pos;
    return expr;
}

Stmt *ast_new_stmt(Arena *arena, StmtKind kind, SourcePos pos) {
    Stmt *stmt = arena_alloc(arena, sizeof(Stmt));
    stmt->kind = kind;
    stmt->pos = pos;
    return stmt;
}

SourcePos ast_pos_join(SourcePos first, SourcePos last) {
    SourcePos pos = first;
    size_t end = last.span.offset + last.span.length;
    pos.span.length = end > first.span.offset ? end - first.span.offset : 0;
    return pos;
}

// "a dog", "an apple": the article that goes before a word.
static const char *article(const char *word) {
    return strchr("aeiouAEIOU", word[0]) ? "an" : "a";
}

static const RecordKind *record_kind(const Records *records, Type type) {
    size_t index = type_record_index(type);
    return records && index < records->len ? &records->items[index] : NULL;
}

Type ast_map_type(Arena *arena, Records *records, Type key, Type value) {
    if (records->maps.len == 0) {  // maps[0]: a map whose kinds aren't decided
        MapKind undecided = {TYPE_UNKNOWN, TYPE_UNKNOWN};
        vec_push(arena, &records->maps, undecided);
    }
    for (size_t i = 0; i < records->maps.len; i++) {
        if (records->maps.items[i].key == key && records->maps.items[i].value == value) return TYPE_MAP + (Type)i;
    }
    MapKind kind = {key, value};
    vec_push(arena, &records->maps, kind);
    return TYPE_MAP + (Type)(records->maps.len - 1);
}

static const MapKind *map_kind(const Records *records, Type map) {
    size_t index = map - TYPE_MAP;
    return records && index < records->maps.len ? &records->maps.items[index] : NULL;
}

Type ast_map_key(const Records *records, Type map) {
    const MapKind *kind = map_kind(records, map);
    return kind ? kind->key : TYPE_UNKNOWN;
}

Type ast_map_value(const Records *records, Type map) {
    const MapKind *kind = map_kind(records, map);
    return kind ? kind->value : TYPE_UNKNOWN;
}

bool ast_type_undecided(const Records *records, Type type) {
    Type base = type_base(type);
    if (type_is_map(base)) {
        return ast_type_undecided(records, ast_map_key(records, base)) ||
               ast_type_undecided(records, ast_map_value(records, base));
    }
    return base == TYPE_UNKNOWN || base == TYPE_MAYBE;
}

static const char *map_name(Arena *arena, const Records *records, Type type);

// What a list or map holds, in the plural: "numbers", "lists of text",
// "dogs", "maps from text to numbers"; NULL when the innermost kind isn't decided.
static const char *plural_name(Arena *arena, const Records *records, Type type) {
    static const char *const plural[] = {NULL, "numbers", "text", "yes/no values"};
    StrBuf sb;
    sb_init(&sb, arena);
    for (Type t = type; type_is_list(t); t = type_item(t)) sb_append(&sb, "lists of ");
    Type base = type_base(type);
    if (base == TYPE_NUMBER || base == TYPE_TEXT || base == TYPE_YESNO) {
        sb_append(&sb, plural[base]);
        return sb.data;
    }
    const RecordKind *kind = type_is_record(base) ? record_kind(records, base) : NULL;
    if (kind) {
        sb_appendf(&sb, "%s%s", kind->plural, type_is_maybe(base) ? " or nothing" : "");
        return sb.data;
    }
    if (!type_is_map(base)) return NULL;
    sb_appendf(&sb, "maps%s", map_name(arena, records, base) + 5);  // "a map from ..." without "a map"
    return sb.data;
}

// "a list of numbers", "a list of lists of text", "a list of dogs"; "a list"
// (or "a list of lists") when the innermost kind isn't decided.
static const char *list_name(Arena *arena, const Records *records, Type type) {
    const char *items = plural_name(arena, records, type_item(type));
    if (items) return arena_sprintf(arena, "a list of %s", items);
    StrBuf sb;
    sb_init(&sb, arena);
    sb_append(&sb, "a list");
    for (Type t = type_item(type); type_is_list(t); t = type_item(t)) sb_append(&sb, " of lists");
    return sb.data;
}

// "a map from text to numbers", "a map from numbers to lists of text"; "a
// map" when its keys or values aren't decided.
static const char *map_name(Arena *arena, const Records *records, Type type) {
    Type key = ast_map_key(records, type);
    const char *values = plural_name(arena, records, ast_map_value(records, type));
    if ((key != TYPE_TEXT && key != TYPE_NUMBER) || !values) return "a map";
    return arena_sprintf(arena, "a map from %s to %s", key == TYPE_TEXT ? "text" : "numbers", values);
}

const char *ast_type_name(Arena *arena, const Records *records, Type type) {
    if (type_is_list(type)) return list_name(arena, records, type);
    if (type_is_map(type)) return map_name(arena, records, type);
    if (type == TYPE_MAYBE) return "a record or nothing";
    const RecordKind *kind = type_is_record(type) ? record_kind(records, type) : NULL;
    if (kind) {
        return arena_sprintf(arena, "%s %s%s", article(kind->display), kind->display,
                             type_is_maybe(type) ? " or nothing" : "");
    }
    switch (type) {
    case TYPE_NUMBER: return "a number";
    case TYPE_TEXT: return "text";
    case TYPE_YESNO: return "a yes/no value";
    case TYPE_NOTHING: return "nothing";
    default: return "an unknown kind of value";
    }
}

const char *ast_binary_op_name(BinaryOp op) {
    static const char *const names[] = {
        "or", "and", "equal", "not equal", "less", "less or equal", "greater",
        "greater or equal", "add", "subtract", "join", "multiply", "divide", "modulo",
        "contains", "does not contain", "starts with", "does not start with", "ends with", "does not end with",
        "has", "does not have",
    };
    return names[op];
}

const char *ast_unary_op_name(UnaryOp op) {
    return op == UNARY_NEGATE ? "negate" : "not";
}

const char *ast_change_op_name(ChangeOp op) {
    static const char *const names[] = {"add", "subtract", "multiply", "divide"};
    return names[op];
}

const char *ast_list_op_name(ListOp op) {
    static const char *const names[] = {
        "added together", "biggest of", "smallest of", "reversed", "copy of", "is empty", "is not empty",
        "number of entries in", "keys of", "values of",
    };
    return names[op];
}

const char *ast_text_op_name(TextOp op) {
    static const char *const names[] = {
        "characters of", "words of", "lines of", "in uppercase", "in lowercase", "without extra spaces",
        "split by", "joined with", "replaced", "position of", "code of", "character with code",
        "is a letter", "is a digit", "is a space",
    };
    return names[op];
}

// What the dump writes to, and the program's record kinds (for their names).
typedef struct {
    StrBuf *out;
    const Records *records;
} Dump;

static void dump_expr(const Expr *expr, size_t depth, Dump *d);

static void dump_line(const SourcePos *pos, size_t depth, Dump *d, const char *label) {
    sb_append_repeat(d->out, ' ', depth * 2);
    sb_append(d->out, label);
    sb_appendf(d->out, " [%zu:%zu]\n", pos->line, pos->column);
}

static void dump_children(const Expr *a, const Expr *b, size_t depth, Dump *d) {
    if (a) dump_expr(a, depth + 1, d);
    if (b) dump_expr(b, depth + 1, d);
}

// The label for a leaf or a node whose label carries a value.
static const char *expr_label(const Expr *expr, StrBuf *scratch, const Records *records) {
    switch (expr->kind) {
    case EXPR_ERROR: return "error";
    case EXPR_NUMBER: sb_appendf(scratch, "number %s", expr->as.number.text); break;
    case EXPR_TEXT:
        sb_append(scratch, "text ");
        sb_append_quoted(scratch, expr->as.text.value, expr->as.text.len);
        break;
    case EXPR_BOOLEAN: return expr->as.boolean ? "yes" : "no";
    case EXPR_NOTHING: return "nothing";
    case EXPR_IT: return "it";
    case EXPR_NAME: sb_appendf(scratch, "name %s", expr->as.name); break;
    case EXPR_CALL: sb_appendf(scratch, "call %s", expr->as.call.name); break;
    case EXPR_LENGTH: return "length of";
    case EXPR_FILE_CONTENTS: return "contents of file";
    case EXPR_UNARY: return ast_unary_op_name(expr->as.unary.op);
    case EXPR_BINARY: return ast_binary_op_name(expr->as.binary.op);
    case EXPR_CONVERT: return expr->as.convert.target == CONVERT_TO_NUMBER ? "as number" : "as text";
    case EXPR_LIST: return "list of";
    case EXPR_EMPTY_LIST:
        sb_appendf(scratch, "empty %s", ast_type_name(scratch->arena, records, expr->as.list_type) + 2);  // without "a "
        break;
    case EXPR_ITEM: {
        const char *noun = expr->as.item.characters ? "character" : "item";
        const char *where = expr->as.item.where == ITEM_FIRST ? "first " : expr->as.item.where == ITEM_LAST ? "last " : "";
        sb_appendf(scratch, "%s%s", where, noun);
        break;
    }
    case EXPR_LIST_OP: return ast_list_op_name(expr->as.list_op.op);
    case EXPR_EVERY: sb_appendf(scratch, "every %s in", expr->as.every.var.text); break;
    case EXPR_ONLY: return "only";
    case EXPR_SLICE: {
        static const char *const names[] = {"items", "first items", "last items", "without the first item",
                                            "without the last item", "every"};
        static const char *const text_names[] = {"characters", "first characters", "last characters",
                                                 "without the first character", "without the last character", "every"};
        bool characters = expr->as.slice.characters;
        sb_append(scratch, (characters ? text_names : names)[expr->as.slice.kind]);
        const char *noun = characters ? "character" : "item";
        if (expr->as.slice.kind == SLICE_RANGE && !expr->as.slice.to) sb_append(scratch, " to the end");
        if (expr->as.slice.kind == SLICE_EVERY && expr->as.slice.start == 1) sb_appendf(scratch, " other %s", noun);
        if (expr->as.slice.kind == SLICE_EVERY && expr->as.slice.start != 1) {
            sb_appendf(scratch, " %s at a multiple of %zu", noun, expr->as.slice.step);
        }
        break;
    }
    case EXPR_TEXT_OP:
        if (expr->as.text_op.negated) {
            sb_appendf(scratch, "is not %s", ast_text_op_name(expr->as.text_op.op) + 3);
            break;
        }
        return ast_text_op_name(expr->as.text_op.op);
    case EXPR_FIELD: sb_appendf(scratch, "field %s", expr->as.field.name.text); break;
    case EXPR_NEW: sb_appendf(scratch, "new %s", ast_type_name(scratch->arena, records, expr->as.new_record.type) + 2); break;
    case EXPR_MAP: return "map of";
    case EXPR_EMPTY_MAP:
        sb_appendf(scratch, "empty %s", ast_type_name(scratch->arena, records, expr->as.list_type) + 2);  // without "a "
        break;
    case EXPR_MAP_GET: return expr->as.lookup.fallback ? "value for, or" : "value for";
    }
    return scratch->data;
}

static void dump_expr(const Expr *expr, size_t depth, Dump *d) {
    StrBuf label;
    sb_init(&label, d->out->arena);
    dump_line(&expr->pos, depth, d, expr_label(expr, &label, d->records));
    switch (expr->kind) {
    case EXPR_CALL:
        for (size_t i = 0; i < expr->as.call.args.len; i++) {
            dump_expr(expr->as.call.args.items[i], depth + 1, d);
        }
        break;
    case EXPR_LENGTH:
    case EXPR_FILE_CONTENTS: dump_children(expr->as.operand, NULL, depth, d); break;
    case EXPR_UNARY: dump_children(expr->as.unary.operand, NULL, depth, d); break;
    case EXPR_BINARY: dump_children(expr->as.binary.left, expr->as.binary.right, depth, d); break;
    case EXPR_CONVERT: dump_children(expr->as.convert.operand, NULL, depth, d); break;
    case EXPR_LIST:
    case EXPR_MAP:  // a map: each key, then its value
        for (size_t i = 0; i < expr->as.items.len; i++) dump_expr(expr->as.items.items[i], depth + 1, d);
        break;
    case EXPR_MAP_GET:
        dump_children(expr->as.lookup.key, expr->as.lookup.map, depth, d);
        dump_children(expr->as.lookup.fallback, NULL, depth, d);
        break;
    case EXPR_ITEM: dump_children(expr->as.item.position, expr->as.item.list, depth, d); break;
    case EXPR_LIST_OP: dump_children(expr->as.list_op.operand, NULL, depth, d); break;
    case EXPR_EVERY: dump_children(expr->as.every.list, expr->as.every.body, depth, d); break;
    case EXPR_ONLY: dump_children(expr->as.only.list, expr->as.only.condition, depth, d); break;
    case EXPR_SLICE:  // in the order they're written, except postfix "without": the list first
        dump_children(expr->as.slice.from, expr->as.slice.to, depth, d);
        dump_expr(expr->as.slice.list, depth + 1, d);
        break;
    case EXPR_FIELD: dump_children(expr->as.field.record, NULL, depth, d); break;
    case EXPR_NEW:
        for (size_t i = 0; i < expr->as.new_record.values.len; i++) {
            const FieldValue *value = &expr->as.new_record.values.items[i];
            StrBuf label;
            sb_init(&label, d->out->arena);
            sb_appendf(&label, "with %s", value->field.text);
            dump_line(&value->field.pos, depth + 1, d, label.data);
            dump_expr(value->value, depth + 2, d);
        }
        break;
    case EXPR_TEXT_OP:  // in the order they're written, except "the position of X in T": X first
        if (expr->as.text_op.op == TEXT_POSITION) {
            dump_children(expr->as.text_op.arg, expr->as.text_op.operand, depth, d);
        } else {
            dump_children(expr->as.text_op.operand, expr->as.text_op.arg, depth, d);
            dump_children(expr->as.text_op.arg2, NULL, depth, d);
        }
        break;
    default: break;
    }
}

static const char *loop_label(const Stmt *stmt, StrBuf *scratch) {
    switch (stmt->as.loop.kind) {
    case LOOP_COUNT:
        sb_appendf(scratch, "count%s, call each number %s", stmt->as.loop.down ? " down" : "", stmt->as.loop.var.text);
        return scratch->data;
    case LOOP_TIMES: return "repeat times";
    case LOOP_WHILE: return "repeat while";
    case LOOP_UNTIL: return "repeat until";
    case LOOP_FOREVER: return "repeat forever";
    case LOOP_EACH:
        sb_appendf(scratch, "for each %s", stmt->as.loop.var.text);
        if (stmt->as.loop.pair) sb_appendf(scratch, " and %s", stmt->as.loop.value_name.text);
        sb_append(scratch, " in");
        return scratch->data;
    }
    return "loop";
}

static const char *list_label(const Stmt *stmt, StrBuf *scratch) {
    static const char *const where[] = {"item", "first item", "last item"};
    switch (stmt->as.list.op) {
    case LIST_ADD_END: return "add to the end of";
    case LIST_INSERT: return "insert at position";
    case LIST_REMOVE: sb_appendf(scratch, "remove %s", where[stmt->as.list.where]); break;
    case LIST_SET_ITEM: sb_appendf(scratch, "set %s", where[stmt->as.list.where]); break;
    case LIST_SORT:
        sb_append(scratch, stmt->as.list.by_length ? "sort by length" : "sort");
        if (stmt->as.list.descending) sb_append(scratch, stmt->as.list.by_length ? ", longest first" : ", biggest first");
        break;
    case LIST_REMOVE_KEY: return "remove from map";
    }
    return scratch->data;
}

// The first line of a statement, e.g. "let total" or "read file, call it notes".
static const char *stmt_label(const Stmt *stmt, StrBuf *scratch, const Records *records) {
    switch (stmt->kind) {
    case STMT_LET: sb_appendf(scratch, "let %s", stmt->as.assign.name.text); break;
    case STMT_SET:
        if (stmt->as.assign.field) return stmt->as.assign.field->kind == EXPR_MAP_GET ? "set value in map" : "set field";
        sb_appendf(scratch, "set %s", stmt->as.assign.name.text);
        break;
    case STMT_CHANGE: {
        const Expr *field = stmt->as.change.field;
        sb_appendf(scratch, "%s %s", ast_change_op_name(stmt->as.change.op),
                   !field ? stmt->as.change.target.text : field->kind == EXPR_MAP_GET ? "value in map" : "field");
        break;
    }
    case STMT_SAY: return "say";
    case STMT_ASK: sb_appendf(scratch, "ask, call the answer %s", stmt->as.ask.answer.text); break;
    case STMT_WRITE_FILE: return "write to file";
    case STMT_APPEND_FILE: return "append to file";
    case STMT_READ_FILE: sb_appendf(scratch, "read file, call it %s", stmt->as.read_file.name.text); break;
    case STMT_STOP: return "stop the program";
    case STMT_IF: return stmt->as.if_stmt.one_line ? "if (one line)" : "if";
    case STMT_LOOP: return loop_label(stmt, scratch);
    case STMT_BREAK: return "stop the loop";
    case STMT_CONTINUE: return "skip this one";
    case STMT_FUNCTION:
        sb_appendf(scratch, "define %s (", stmt->as.function.name.text);
        for (size_t i = 0; i < stmt->as.function.params.len; i++) {
            sb_appendf(scratch, "%s%s", i ? ", " : "", stmt->as.function.params.items[i].text);
            Type declared = stmt->as.function.declared.items[i];
            if (declared != TYPE_UNKNOWN) sb_appendf(scratch, ": %s", ast_type_name(scratch->arena, records, declared));
        }
        sb_append_char(scratch, ')');
        if (stmt->as.function.declared_result != TYPE_UNKNOWN) {
            sb_appendf(scratch, " giving back %s", ast_type_name(scratch->arena, records, stmt->as.function.declared_result));
        }
        break;
    case STMT_RETURN: return "give back";
    case STMT_CALL: return "call";
    case STMT_CONSTANT: sb_appendf(scratch, "keep %s", stmt->as.constant.name.text); break;
    case STMT_LIST: return list_label(stmt, scratch);
    case STMT_RECORD: sb_appendf(scratch, "record %s", stmt->as.record.name.text); break;
    }
    return scratch->data;
}

static void dump_stmt(const Stmt *stmt, size_t depth, Dump *d);

static void dump_label(size_t depth, Dump *d, const char *label) {
    sb_append_repeat(d->out, ' ', depth * 2);
    sb_append(d->out, label);
    sb_append_char(d->out, '\n');
}

// branch if / branch otherwise if / branch otherwise, then "condition" and
// "then" sections.
static void dump_if(const Stmt *stmt, size_t depth, Dump *d) {
    for (size_t i = 0; i < stmt->as.if_stmt.branches.len; i++) {
        const IfBranch *branch = &stmt->as.if_stmt.branches.items[i];
        const char *label = i == 0 ? "branch if" : branch->condition ? "branch otherwise if" : "branch otherwise";
        dump_line(&branch->pos, depth + 1, d, label);
        if (branch->condition) {
            dump_label(depth + 2, d, "condition");
            dump_expr(branch->condition, depth + 3, d);
        }
        dump_label(depth + 2, d, "then");
        for (size_t j = 0; j < branch->body.len; j++) {
            dump_stmt(branch->body.items[j], depth + 3, d);
        }
    }
}

static void dump_section(const char *label, const Expr *expr, size_t depth, Dump *d) {
    if (!expr) return;
    dump_label(depth, d, label);
    dump_expr(expr, depth + 1, d);
}

// Sections "from", "to", "by", "times", "condition", then "do" with the body.
static void dump_loop(const Stmt *stmt, size_t depth, Dump *d) {
    dump_section("from", stmt->as.loop.from, depth + 1, d);
    dump_section("to", stmt->as.loop.to, depth + 1, d);
    dump_section("by", stmt->as.loop.step, depth + 1, d);
    dump_section("times", stmt->as.loop.times, depth + 1, d);
    dump_section("condition", stmt->as.loop.condition, depth + 1, d);
    dump_section("in", stmt->as.loop.list, depth + 1, d);
    dump_label(depth + 1, d, "do");
    for (size_t i = 0; i < stmt->as.loop.body.len; i++) {
        dump_stmt(stmt->as.loop.body.items[i], depth + 2, d);
    }
}

// The section a sentence's target goes in: a record's "field", or "target" for a value in a map.
static const char *target_label(const Expr *target) {
    return target && target->kind == EXPR_MAP_GET ? "target" : "field";
}

static void dump_stmt(const Stmt *stmt, size_t depth, Dump *d) {
    StrBuf label;
    sb_init(&label, d->out->arena);
    dump_line(&stmt->pos, depth, d, stmt_label(stmt, &label, d->records));
    switch (stmt->kind) {
    case STMT_LET:
    case STMT_SET:
        dump_section(target_label(stmt->as.assign.field), stmt->as.assign.field, depth + 1, d);
        dump_expr(stmt->as.assign.value, depth + 1, d);
        break;
    case STMT_CHANGE: {  // in the order they're written
        const Expr *field = stmt->as.change.field;
        if (field && stmt->as.change.amount->pos.span.offset < field->pos.span.offset) {
            dump_expr(stmt->as.change.amount, depth + 1, d);
            dump_section(target_label(field), field, depth + 1, d);
        } else {
            dump_section(target_label(field), field, depth + 1, d);
            dump_expr(stmt->as.change.amount, depth + 1, d);
        }
        break;
    }
    case STMT_SAY: dump_expr(stmt->as.value, depth + 1, d); break;
    case STMT_ASK: dump_expr(stmt->as.ask.prompt, depth + 1, d); break;
    case STMT_WRITE_FILE:
    case STMT_APPEND_FILE:
        dump_children(stmt->as.file_write.text, stmt->as.file_write.path, depth, d);
        break;
    case STMT_READ_FILE: dump_expr(stmt->as.read_file.path, depth + 1, d); break;
    case STMT_STOP: break;
    case STMT_IF: dump_if(stmt, depth, d); break;
    case STMT_LOOP: dump_loop(stmt, depth, d); break;
    case STMT_BREAK:
    case STMT_CONTINUE: break;
    case STMT_FUNCTION:
        dump_label(depth + 1, d, "do");
        for (size_t i = 0; i < stmt->as.function.body.len; i++) {
            dump_stmt(stmt->as.function.body.items[i], depth + 2, d);
        }
        break;
    case STMT_RETURN:
        if (stmt->as.returned) dump_expr(stmt->as.returned, depth + 1, d);
        break;
    case STMT_CALL: dump_expr(stmt->as.call, depth + 1, d); break;
    case STMT_CONSTANT: dump_expr(stmt->as.constant.value, depth + 1, d); break;
    case STMT_LIST:
        // In the order they're written: "set item P of L to V" ends with its
        // value, the others start with it.
        if (stmt->as.list.op != LIST_SET_ITEM) {
            dump_section(stmt->as.list.op == LIST_REMOVE_KEY ? "key" : "value", stmt->as.list.value, depth + 1, d);
        }
        dump_section("position", stmt->as.list.position, depth + 1, d);
        dump_section(stmt->as.list.op == LIST_REMOVE_KEY ? "map" : "list", stmt->as.list.list, depth + 1, d);
        if (stmt->as.list.op == LIST_SET_ITEM) dump_section("value", stmt->as.list.value, depth + 1, d);
        break;
    case STMT_RECORD:
        for (size_t i = 0; i < stmt->as.record.fields.len; i++) {
            const Field *field = &stmt->as.record.fields.items[i];
            StrBuf label;
            sb_init(&label, d->out->arena);
            sb_appendf(&label, "field %s: %s", field->name.text, ast_type_name(d->out->arena, d->records, field->type));
            dump_line(&field->pos, depth + 1, d, label.data);
            dump_section("starting at", field->initial, depth + 2, d);
        }
        break;
    }
}

void ast_dump_program(const Program *program, StrBuf *out) {
    Dump d = {out, &program->records};
    for (size_t i = 0; i < program->body.len; i++) {
        dump_stmt(program->body.items[i], 0, &d);
    }
}
