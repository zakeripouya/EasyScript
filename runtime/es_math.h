/* EasyScript runtime, part 11: math and random numbers.
 *
 * Math on numbers ("the square root of", "to the power of", "rounded", ...)
 * fails with a plain message where C would give "not a number". Random
 * numbers come from xoshiro256**, seeded from the clock on first use, or
 * with a fixed seed after "use the same random numbers every time" (and,
 * for tests, with ES_RANDOM_SEED under ES_HOOKS). */
#ifndef ES_MATH_H
#define ES_MATH_H

#ifndef ES_CLOSURE_H
#include "es_closure.h"
#endif

#include <time.h>

/* --- Math ----------------------------------------------------------------- */

static double es_square_root(int site, double x) {
    if (x < 0) {
        es_fail(site, "Only numbers from 0 up have a square root.", "I can't take the square root of %s, because it's "
                "below zero.", es_shown(x));
    }
    return sqrt(x);
}

static bool es_whole(double x) {
    return isfinite(x) && x == floor(x);
}

static double es_power(int site, double x, double y) {
    if (x < 0 && !es_whole(y)) {
        es_fail(site, NULL, "I can't raise %s to the power of %s: a number below zero can only be raised to a whole "
                "power.", es_shown(x), es_shown(y));
    }
    if (x == 0 && y < 0) {
        es_fail(site, NULL, "I can't raise 0 to the power of %s, because that divides by zero.", es_shown(y));
    }
    return pow(x, y);
}

/* Half away from zero, like "2.5 rounded" is 3. A number a rounding error
 * away from a half (1.005 * 100 is 100.49999999999999) counts as the half. */
static double es_round_half(double x) {
    double down = floor(x), part = x - down;
    if (fabs(part - 0.5) <= 1e-9 * fmax(1, fabs(x))) return x < 0 ? down : down + 1;
    return round(x);
}

static double es_round_places(int site, double x, double places) {
    if (!es_whole(places) || places < 0 || places > 15) {
        es_fail(site, NULL, "The number of decimal places has to be a whole number from 0 to 15, but it's %s.",
                es_shown(places));
    }
    if (!isfinite(x)) return x;
    double scale = pow(10, places);
    return es_round_half(x * scale) / scale;
}

static inline double es_bigger(double a, double b) { return a >= b || isnan(b) ? a : b; }
static inline double es_smaller(double a, double b) { return a <= b || isnan(b) ? a : b; }

/* --- Random numbers ------------------------------------------------------- */

static uint64_t es_rng[4];
static bool es_rng_ready;

static uint64_t es_splitmix(uint64_t *x) {
    uint64_t z = (*x += 0x9E3779B97F4A7C15ull);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

static void es_random_seed(uint64_t seed) {
    for (int i = 0; i < 4; i++) es_rng[i] = es_splitmix(&seed);
    es_rng_ready = true;
}

/* "use the same random numbers every time" */
static void es_random_fix(void) {
    es_random_seed(20261011);
}

static uint64_t es_random_bits(void) {
    if (!es_rng_ready) {
        uint64_t seed = (uint64_t)time(NULL) ^ ((uint64_t)clock() << 32) ^ (uint64_t)(uintptr_t)&seed;
#ifdef ES_HOOKS
        const char *fixed = getenv("ES_RANDOM_SEED");
        if (fixed) seed = strtoull(fixed, NULL, 10);
#endif
        es_random_seed(seed);
    }
    uint64_t *s = es_rng;
    uint64_t result = ((s[1] * 5) << 7 | (s[1] * 5) >> 57) * 9;
    uint64_t t = s[1] << 17;
    s[2] ^= s[0];
    s[3] ^= s[1];
    s[1] ^= s[2];
    s[0] ^= s[3];
    s[2] ^= t;
    s[3] = (s[3] << 45) | (s[3] >> 19);
    return result;
}

/* From 0 (included) to 1 (not included), in steps of 2^-53. */
static double es_random_unit(void) {
    return (double)(es_random_bits() >> 11) * (1.0 / 9007199254740992.0);
}

/* A whole number below n (n at most 2^53), every one as likely. */
static double es_random_below(double n) {
    uint64_t limit = (uint64_t)n, bits;
    uint64_t top = UINT64_MAX - UINT64_MAX % limit; /* so no number is likelier than another */
    do {
        bits = es_random_bits();
    } while (bits >= top);
    return (double)(bits % limit);
}

/* "a random number from A to B": a whole number, A and B included. */
static double es_random_whole(int site, double a, double b) {
    if (!es_whole(a) || !es_whole(b)) {
        es_fail(site, NULL, "A random number from %s to %s can't be worked out: both have to be whole numbers.",
                es_shown(a), es_shown(b));
    }
    if (a > b) {
        es_fail(site, NULL, "I can't pick a random number from %s to %s: the first number has to be the smaller one.",
                es_shown(a), es_shown(b));
    }
    if (b - a >= 9007199254740992.0) {
        es_fail(site, NULL, "The numbers from %s to %s are too many to pick from.", es_shown(a), es_shown(b));
    }
    return a + es_random_below(b - a + 1);
}

/* "a random decimal between A and B": from A (included) to B (not included). */
static double es_random_decimal(int site, double a, double b) {
    if (!isfinite(a) || !isfinite(b) || a > b) {
        es_fail(site, NULL, "I can't pick a random decimal between %s and %s: the first number has to be the smaller "
                "one.", es_shown(a), es_shown(b));
    }
    return a + es_random_unit() * (b - a);
}

/* "pick one of L": a position in L (from 1), every one as likely. Only looks at L. */
static double es_random_position(int site, const EsList *l) {
    if (l->len == 0) es_fail(site, NULL, "I can't pick one of the items of an empty list.");
    return 1 + es_random_below((double)l->len);
}

/* "shuffle L": the items in a random order (every order as likely). The list itself changes. */
static void es_shuffle(EsList *l) {
    size_t size = es_item_size(l->items);
    char *items = (char *)l->at.o;
    for (size_t i = l->len; i > 1; i--) {
        size_t j = (size_t)es_random_below((double)i);
        EsValue a, b; /* any item fits in one */
        memcpy(&a, items + (i - 1) * size, size);
        memcpy(&b, items + j * size, size);
        memcpy(items + (i - 1) * size, &b, size);
        memcpy(items + j * size, &a, size);
    }
    es_list_release(l);
}

/* "L shuffled": a new list (reusing L when nobody else holds it), like "a copy of" in a random order. */
static EsList *es_list_shuffled(EsList *l) {
    EsList *r = es_list_copy(l);
    es_shuffle(es_list_retain(r));
    return r;
}

#endif
