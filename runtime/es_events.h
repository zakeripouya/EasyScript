/* EasyScript runtime, part 18: events ("every 2 seconds:", "every Monday at
 * 9am:", "whenever a key is pressed:"), run by a loop the compiler writes at
 * the end of main(), once the program's other sentences have run.
 *
 * With ES_NOW (tests), the clock only moves when the program waits, so a
 * program with events runs the same way every time. */
#ifndef ES_EVENTS_H
#define ES_EVENTS_H

#ifndef ES_TOTALS_H
#include "es_totals.h"
#endif

/* Seconds that only ever go forward, for "every N seconds" (the fixed clock in tests). */
static double es_event_clock(void) {
    es_now(); /* reads ES_NOW the first time */
    if (es_clock_fixed) return es_clock_now;
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)t.tv_sec + (double)t.tv_nsec / 1e9;
}

/* "every N seconds": N, which has to be more than 0. */
static double es_event_interval(int site, double seconds) {
    if (!(seconds > 0) || !isfinite(seconds)) {
        es_fail(site, NULL, "How often has to be more than 0 seconds, but it's %s.", es_shown(seconds));
    }
    return seconds;
}

/* "every Monday at 9am": the first such time after `after` (on the local
 * calendar); `weekday` is 0 for Sunday ... 6, or -1 for every day. */
static double es_next_at(double after, int weekday, double time) {
    double when = floor(after / ES_DAY) * ES_DAY + time;
    while (when <= after || (weekday >= 0 && es_weekday(when) != weekday)) when += ES_DAY;
    return when;
}

/* Waits `seconds` (moving the fixed clock in tests). */
static void es_event_wait(double seconds) {
    fflush(stdout);
    if (!(seconds > 0)) return;
    if (seconds > 31536000) seconds = 31536000;
    es_now();
    if (es_clock_fixed) {
        es_clock_now += seconds;
        return;
    }
    struct timespec rest = {(time_t)seconds, (long)((seconds - floor(seconds)) * 1e9)};
    while (nanosleep(&rest, &rest) != 0 && errno == EINTR) {
    }
}

/* A key pressed within `seconds` (INFINITY: waiting as long as it takes), or
 * NULL; *ended is set when no more keys can come. Keys that come from a file
 * (not a terminal) are taken one before each round of the schedules, so a
 * program reads them the same way every time; with no schedules, one after another. */
static EsText *es_event_key(double seconds, bool schedules, bool *ended) {
    static bool taken; /* a key was taken since the last round of the schedules */
    fflush(stdout);
    *ended = false;
    if (!isatty(STDIN_FILENO)) {
        if (schedules && taken) {
            taken = false;
            return NULL;
        }
        EsText *key = es_read_key(true);
        taken = key != NULL;
        *ended = key == NULL;
        return key;
    }
    if (seconds <= 0) return es_read_key(false);
    if (isinf(seconds)) return es_read_key(true);
    int tenths = (int)ceil(seconds * 10);
    es_terminal_keys(0, tenths > 255 ? 255 : tenths < 1 ? 1 : tenths);
    int c = getc(stdin);
    if (c == EOF) {
        clearerr(stdin);
        return NULL;
    }
    ungetc(c, stdin);
    return es_read_key(false);
}

#endif
