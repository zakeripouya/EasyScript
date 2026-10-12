// Events (STMT_EVENT) run after the program's other sentences, in one loop
// written at the end of main():
//
//   for (;;) {
//       whenever/when rules: a rule's sentences run when its condition is yes
//           (for "whenever", only when it was no the time before)
//       stop if there are no schedules and no keys
//       wait for a key until the next schedule is due; run the key's sentences
//       wait until the next schedule is due; run the ones that are
//   }
//
// "stop the program" leaves it, like anywhere in main().

#include "back/codegen_internal.h"

static void line(Codegen *g, size_t depth, const char *fmt, ...) PRINTF_LIKE(3, 4);

static void line(Codegen *g, size_t depth, const char *fmt, ...) {
    sb_append_repeat(&g->body, ' ', 4 * (depth + 1));
    va_list args;
    va_start(args, fmt);
    sb_vappendf(&g->body, fmt, args);
    va_end(args);
    sb_append_char(&g->body, '\n');
}

// An event's sentences, in a scope of their own (`depth` is the enclosing level).
static void body(Codegen *g, const Stmt *stmt, size_t depth) {
    size_t saved = g->depth;
    g->depth = depth;
    cg_emit_block(g, &stmt->as.event.body);
    g->depth = saved;
}

// The whenever/when rules, each checked once per round.
static void emit_rules(Codegen *g, const StmtList *events, size_t depth) {
    for (size_t i = 0; i < events->len; i++) {
        const Stmt *stmt = events->items[i];
        if (stmt->kind != STMT_EVENT) continue;
        if (stmt->as.event.kind == EVENT_WHENEVER) {
            line(g, depth, "/* line %zu: whenever */", stmt->pos.line);
            line(g, depth, "{");
            line(g, depth + 1, "bool es_now_yes = %s;", cg_operand(g, stmt->as.event.condition, false));
            line(g, depth + 1, "bool es_fire = es_now_yes && !es_ev_was%zu;", i);
            line(g, depth + 1, "es_ev_was%zu = es_now_yes;", i);
            line(g, depth + 1, "if (es_fire) {");
            body(g, stmt, depth + 1);
            line(g, depth + 1, "}");
            line(g, depth, "}");
        } else if (stmt->as.event.kind == EVENT_WHEN) {
            line(g, depth, "/* line %zu: when */", stmt->pos.line);
            line(g, depth, "if (!es_ev_done%zu && %s) {", i, cg_operand(g, stmt->as.event.condition, false));
            line(g, depth + 1, "es_ev_done%zu = true;", i);
            body(g, stmt, depth);
            line(g, depth, "}");
        }
    }
}

void cg_emit_events(Codegen *g) {
    const StmtList *events = &g->events;
    if (events->len == 0) return;
    bool schedules = false, keys = false;
    for (size_t i = 0; i < events->len; i++) {
        EventKind kind = events->items[i]->as.event.kind;
        schedules |= kind == EVENT_EVERY || kind == EVENT_AT;
        keys |= kind == EVENT_KEY;
    }
    cg_plan(g, events->items[0]->pos, "events: one loop after the rest of the program, %s%s.",
            schedules ? "waiting for the next schedule" : "checking the rules once",
            keys ? " and reading keys as they come" : "");
    line(g, 0, "/* events: once the rest of the program has run */");
    line(g, 0, "{");
    for (size_t i = 0; i < events->len; i++) {
        const Stmt *stmt = events->items[i];
        switch (stmt->as.event.kind) {
        case EVENT_EVERY: {
            const Expr *every = stmt->as.event.every;
            size_t site = cg_site(g, every ? every->pos : stmt->pos);
            const char *amount = every ? cg_operand(g, every, false) : "1.0";
            line(g, 1, "double es_ev_every%zu = es_event_interval(%zu, %s * %.17g);", i, site, amount,
                 stmt->as.event.scale);
            line(g, 1, "double es_ev_next%zu = es_event_clock() + es_ev_every%zu;", i, i);
            break;
        }
        case EVENT_AT:
            line(g, 1, "double es_ev_next%zu = es_next_at(es_now(), %d, %.17g);", i, stmt->as.event.weekday,
                 stmt->as.event.time);
            break;
        case EVENT_WHENEVER: line(g, 1, "bool es_ev_was%zu = false;", i); break;
        case EVENT_WHEN: line(g, 1, "bool es_ev_done%zu = false;", i); break;
        case EVENT_KEY: break;
        }
    }
    line(g, 1, "for (;;) {");
    emit_rules(g, events, 2);
    if (!schedules && !keys) {
        line(g, 2, "break;");
    } else {
        line(g, 2, "double es_ev_wait = INFINITY;  /* until the next schedule is due */");
        for (size_t i = 0; i < events->len; i++) {
            EventKind kind = events->items[i]->as.event.kind;
            if (kind == EVENT_EVERY) line(g, 2, "es_ev_wait = fmin(es_ev_wait, es_ev_next%zu - es_event_clock());", i);
            if (kind == EVENT_AT) line(g, 2, "es_ev_wait = fmin(es_ev_wait, es_ev_next%zu - es_now());", i);
        }
    }
    if (keys) {
        line(g, 2, "bool es_ev_ended;");
        line(g, 2, "EsText *es_ev_key = es_event_key(es_ev_wait, %s, &es_ev_ended);", schedules ? "true" : "false");
        line(g, 2, "if (es_ev_key) {");
        cg_declare_name(g, "0key", TYPE_TEXT);
        StrBuf store;
        sb_init(&store, g->arena);
        sb_append(&store, "es_set(&");
        cg_append_var(g, &store, "0key", TYPE_TEXT);
        sb_append(&store, ", es_ev_key);");
        line(g, 3, "%s", store.data);
        LoopCode it = {NULL, 0, 0, "0key", TYPE_TEXT};
        vec_push(g->arena, &g->loops, it);
        for (size_t i = 0; i < events->len; i++) {
            const Stmt *stmt = events->items[i];
            if (stmt->as.event.kind != EVENT_KEY) continue;
            line(g, 3, "/* line %zu: whenever a key is pressed */", stmt->pos.line);
            body(g, stmt, 2);
        }
        g->loops.len--;
        line(g, 3, "continue;");
        line(g, 2, "}");
        if (!schedules) line(g, 2, "if (es_ev_ended) break;");
        if (!schedules) line(g, 2, "continue;");
        if (schedules) line(g, 2, "(void)es_ev_ended;");
    }
    if (schedules) {
        line(g, 2, "es_event_wait(es_ev_wait);");
        for (size_t i = 0; i < events->len; i++) {
            const Stmt *stmt = events->items[i];
            if (stmt->as.event.kind == EVENT_EVERY) {
                line(g, 2, "/* line %zu: every ... */", stmt->pos.line);
                line(g, 2, "if (es_event_clock() >= es_ev_next%zu - 1e-9) {", i);
                line(g, 3, "es_ev_next%zu += es_ev_every%zu;", i, i);
            } else if (stmt->as.event.kind == EVENT_AT) {
                line(g, 2, "/* line %zu: every ... at ... */", stmt->pos.line);
                line(g, 2, "if (es_now() >= es_ev_next%zu) {", i);
                line(g, 3, "es_ev_next%zu = es_next_at(es_ev_next%zu, %d, %.17g);", i, i, stmt->as.event.weekday,
                     stmt->as.event.time);
            } else {
                continue;
            }
            body(g, stmt, 2);
            line(g, 2, "}");
        }
    }
    line(g, 1, "}");
    line(g, 0, "}");
}
