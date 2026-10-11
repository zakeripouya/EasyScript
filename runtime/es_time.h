/* EasyScript runtime, part 12: dates, and waiting.
 *
 * A date is a number of seconds from midnight at the start of January 1,
 * 1970, counted on the local calendar (as if every day had 86400 seconds),
 * so days are exact steps and a date's parts never depend on time zones
 * once it's made. "today" is midnight of the current day; "the time now"
 * keeps the time of day too. Under ES_HOOKS, ES_NOW ("2026-10-11 14:05:00")
 * fixes the clock, and waiting moves it on instead of sleeping, so tests
 * don't depend on the day they run. */
#ifndef ES_TIME_H
#define ES_TIME_H

#ifndef ES_MATH_H
#include "es_math.h"
#endif

#define ES_DAY 86400.0

static bool es_clock_fixed;
static double es_clock_now; /* the fixed clock, in seconds as above */

/* Days from 1970-01-01 to y-m-d (proleptic Gregorian calendar). */
static double es_days_from_civil(long y, int m, int d) {
    y -= m <= 2;
    long era = (y >= 0 ? y : y - 399) / 400;
    long yoe = y - era * 400;
    long doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    long doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (double)(era * 146097 + doe - 719468);
}

static void es_civil_from_days(double days, long *year, int *month, int *day) {
    long z = (long)days + 719468;
    long era = (z >= 0 ? z : z - 146096) / 146097;
    long doe = z - era * 146097;
    long yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    long doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    long mp = (5 * doy + 2) / 153;
    *day = (int)(doy - (153 * mp + 2) / 5 + 1);
    *month = (int)(mp < 10 ? mp + 3 : mp - 9);
    *year = yoe + era * 400 + (*month <= 2);
}

static double es_now(void) {
#ifdef ES_HOOKS
    static bool checked;
    if (!checked) {
        checked = true;
        const char *fixed = getenv("ES_NOW");
        int y, mo, d, h = 0, mi = 0, s = 0;
        if (fixed && sscanf(fixed, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) >= 3) {
            es_clock_fixed = true;
            es_clock_now = es_days_from_civil(y, mo, d) * ES_DAY + h * 3600 + mi * 60 + s;
        }
    }
#endif
    if (es_clock_fixed) return es_clock_now;
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    return es_days_from_civil(local.tm_year + 1900L, local.tm_mon + 1, local.tm_mday) * ES_DAY + local.tm_hour * 3600 +
           local.tm_min * 60 + local.tm_sec;
}

static double es_today(int offset) {
    return (floor(es_now() / ES_DAY) + offset) * ES_DAY;
}

/* "3 days ago" (sign -1), "2 days from now" (sign 1): a day counted from today. */
static double es_days_away(int site, double days, int sign) {
    if (!es_whole(days)) es_fail(site, NULL, "The number of days has to be a whole number, but it's %s.", es_shown(days));
    return es_today(0) + sign * days * ES_DAY;
}

static const char *const es_weekdays[] = {"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday", "Saturday"};
static const char *const es_months[] = {"January", "February", "March", "April", "May", "June", "July", "August",
                                        "September", "October", "November", "December"};

static int es_weekday(double date) {
    long days = (long)floor(date / ES_DAY);
    return (int)(((days + 4) % 7 + 7) % 7); /* 1970-01-01 was a Thursday */
}

/* A date's year, month (1 to 12), day, hour (0 to 23) or minute: `part` as in the compiler's DatePart. */
static double es_date_part(double date, int part) {
    long year;
    int month, day;
    es_civil_from_days(floor(date / ES_DAY), &year, &month, &day);
    double seconds = date - floor(date / ES_DAY) * ES_DAY;
    switch (part) {
    case 0: return (double)year;
    case 1: return month;
    case 2: return day;
    case 4: return floor(seconds / 3600);
    default: return floor(fmod(seconds, 3600) / 60);
    }
}

static EsText *es_weekday_text(double date) {
    const char *name = es_weekdays[es_weekday(date)];
    return es_text_copy(name, strlen(name));
}

/* "D is in this month" and the others: `period` as in the compiler's Period. */
static bool es_in_period(double date, int period) {
    long year, now_year;
    int month, day, now_month;
    es_civil_from_days(floor(date / ES_DAY), &year, &month, &day);
    es_civil_from_days(floor(es_now() / ES_DAY), &now_year, &now_month, &day);
    long months = year * 12 + month - 1, now_months = now_year * 12 + now_month - 1;
    switch (period) {
    case 0: return months == now_months;
    case 1: return months == now_months - 1;
    case 2: return months == now_months + 1;
    case 3: return year == now_year;
    case 4: return year == now_year - 1;
    default: return year == now_year + 1;
    }
}

/* "the number of days between D and E": whole days, never below zero. */
static double es_days_between(double a, double b) {
    return fabs(floor(b / ES_DAY) - floor(a / ES_DAY));
}

/* "Sunday, October 11, 2026", and ", 2:05 PM" when it has a time of day. */
static EsText *es_date_text(double date) {
    if (!isfinite(date)) return es_number_text(date);
    long year;
    int month, day;
    double days = floor(date / ES_DAY);
    es_civil_from_days(days, &year, &month, &day);
    long seconds = (long)(date - days * ES_DAY);
    int hour = (int)(seconds / 3600), minute = (int)(seconds % 3600 / 60);
    const char *weekday = es_weekdays[es_weekday(date)], *name = es_months[month - 1];
    int n = seconds ? snprintf(NULL, 0, "%s, %s %d, %ld, %d:%02d %s", weekday, name, day, year,
                               hour % 12 ? hour % 12 : 12, minute, hour < 12 ? "AM" : "PM")
                    : snprintf(NULL, 0, "%s, %s %d, %ld", weekday, name, day, year);
    char *chars;
    EsText *t = es_text_new((size_t)n, &chars);
    if (seconds) {
        snprintf(chars, (size_t)n + 1, "%s, %s %d, %ld, %d:%02d %s", weekday, name, day, year,
                 hour % 12 ? hour % 12 : 12, minute, hour < 12 ? "AM" : "PM");
    } else {
        snprintf(chars, (size_t)n + 1, "%s, %s %d, %ld", weekday, name, day, year);
    }
    return t;
}

/* "wait 2 seconds": what's printed so far is shown first. */
static void es_wait(int site, double seconds) {
    if (!(seconds >= 0) || seconds > 31536000) {
        es_fail(site, NULL, "I can only wait from 0 seconds to a year, but this is %s seconds.", es_shown(seconds));
    }
    fflush(stdout);
    es_now(); /* reads ES_NOW the first time */
    if (es_clock_fixed) {
        es_clock_now += seconds;
        return;
    }
    struct timespec rest = {(time_t)seconds, (long)((seconds - floor(seconds)) * 1e9)};
    while (nanosleep(&rest, &rest) != 0 && errno == EINTR) {
    }
}

#endif
