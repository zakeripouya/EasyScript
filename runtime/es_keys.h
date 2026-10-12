/* EasyScript runtime, part 15: the keyboard, a key at a time.
 *
 * "wait for a key and call it k" and "the key being pressed" read keys
 * without Enter and without showing them. On a terminal that needs the
 * terminal's key-at-a-time mode: it's turned on by the first key read and
 * stays on (so keys pressed between reads aren't shown, and wait for the
 * next read) until the program ends, asks for a line ("ask"), or runs
 * another program. However the program ends (normally, with an error, or
 * with Ctrl+C), the terminal is put back the way it was.
 *
 * Keys are named in plain words: "up", "down", "left", "right", "enter",
 * "escape", "space", "tab", "backspace", "delete", "home", "end", "page up",
 * "page down", "insert"; any other key is the character it types ("a", "A",
 * "7", "?", "é"). When the keys come from a file or another program instead
 * of a terminal, they're read from it the same way, one at a time, and the
 * key being pressed is simply the next one (nothing once they run out). */
#ifndef ES_KEYS_H
#define ES_KEYS_H

#ifndef ES_CSV_H
#include "es_csv.h"
#endif

#include <signal.h>
#include <termios.h>
#include <unistd.h>

static struct termios es_terminal_saved; /* as the terminal was before the first key */
static bool es_terminal_known;           /* es_terminal_saved holds it */
static bool es_terminal_raw;             /* key-at-a-time mode is on */

/* Back to the terminal's own mode (lines, shown as typed). */
static void es_terminal_normal(void) {
    if (!es_terminal_raw) return;
    tcsetattr(STDIN_FILENO, TCSANOW, &es_terminal_saved);
    es_terminal_raw = false;
}

/* Ctrl+C and the like: put the terminal back, then stop the way the signal would have. */
static void es_terminal_signal(int sig) {
    if (es_terminal_raw) tcsetattr(STDIN_FILENO, TCSANOW, &es_terminal_saved);
    signal(sig, SIG_DFL);
    raise(sig);
}

/* Key-at-a-time mode, waiting for at least `least` bytes (0: not waiting),
 * or for `tenths` of a second. Only on a terminal. */
static void es_terminal_keys(int least, int tenths) {
    if (!isatty(STDIN_FILENO)) return;
    if (!es_terminal_known) {
        if (tcgetattr(STDIN_FILENO, &es_terminal_saved) != 0) return;
        es_terminal_known = true;
        atexit(es_terminal_normal);
        static const int signals[] = {SIGINT, SIGTERM, SIGHUP, SIGQUIT};
        for (size_t i = 0; i < sizeof signals / sizeof signals[0]; i++) {
            if (signal(signals[i], es_terminal_signal) == SIG_IGN) signal(signals[i], SIG_IGN); /* still ignored */
        }
    }
    struct termios raw = es_terminal_saved;
    raw.c_lflag &= ~(tcflag_t)(ICANON | ECHO);
    raw.c_iflag &= ~(tcflag_t)(ICRNL | IXON);
    raw.c_cc[VMIN] = (cc_t)least;
    raw.c_cc[VTIME] = (cc_t)tenths;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    es_terminal_raw = true;
}

/* The next byte, or EOF: waiting for one, or (wait false) only if one is there already. */
static int es_key_byte(bool wait) {
    es_terminal_keys(wait ? 1 : 0, 0);
    int c = getc(stdin);
    if (c == EOF) clearerr(stdin);
    return c;
}

/* After an escape: the next byte of the sequence, if it comes at once (a
 * tenth of a second on a terminal), otherwise EOF: the key was Escape. */
static int es_key_follow(void) {
    es_terminal_keys(0, 1);
    int c = getc(stdin);
    if (c == EOF) clearerr(stdin);
    return c;
}

static EsText *es_key_word(const char *name) {
    return es_text_copy(name, strlen(name));
}

/* What follows "escape [": an arrow key or another named key, or NULL for one without a name. */
static const char *es_key_sequence(void) {
    int c = es_key_follow();
    if (c >= '0' && c <= '9') { /* "3~" is delete, ... */
        int number = 0;
        while (c >= '0' && c <= '9') {
            number = number * 10 + (c - '0');
            c = es_key_follow();
        }
        while (c != EOF && !(c >= 0x40 && c <= 0x7E)) c = es_key_follow(); /* to the end of the sequence */
        if (c != '~') return NULL;
        static const char *const numbered[] = {NULL, "home", "insert", "delete", "end", "page up", "page down",
                                               "home", "end"};
        return number >= 1 && number <= 8 ? numbered[number] : NULL;
    }
    switch (c) {
    case 'A': return "up";
    case 'B': return "down";
    case 'C': return "right";
    case 'D': return "left";
    case 'H': return "home";
    case 'F': return "end";
    default: return NULL;
    }
}

/* One key, or NULL: none there (wait false), or the input has ended. */
static EsText *es_read_key(bool wait) {
    for (;;) {
        int c = es_key_byte(wait);
        if (c == EOF) return NULL;
        if (c == 27) {
            int next = es_key_follow();
            if (next == '[' || next == 'O') {
                const char *name = es_key_sequence();
                if (name) return es_key_word(name);
                continue; /* a key without a name (F5, ...): the next one */
            }
            if (next != EOF) ungetc(next, stdin); /* Escape, then another key */
            return es_key_word("escape");
        }
        if (c == '\r' || c == '\n') return es_key_word("enter");
        if (c == '\t') return es_key_word("tab");
        if (c == ' ') return es_key_word("space");
        if (c == 127 || c == 8) return es_key_word("backspace");
        if (c < 32) continue; /* Ctrl and a letter: not a key the program sees */
        if (c < 128) return es_ascii_text((unsigned char)c);
        char bytes[4] = {(char)c, 0, 0, 0}; /* a character of several bytes (UTF-8) */
        size_t n = c >= 0xF0 ? 4 : c >= 0xE0 ? 3 : c >= 0xC0 ? 2 : 1;
        size_t got = 1;
        while (got < n) {
            int more = es_key_byte(true);
            if (more == EOF) break;
            bytes[got++] = (char)more;
        }
        return es_text_copy(bytes, got);
    }
}

/* "wait for a key and call it k". */
static EsText *es_wait_key(int site) {
    fflush(stdout); /* show what's been printed before waiting */
    EsText *key = es_read_key(true);
    if (!key) es_fail(site, NULL, "There's no key to wait for: the keys sent to the program have run out.");
    return key;
}

/* "the key being pressed": a key if one is there, otherwise nothing (NULL). */
static EsRecord *es_key_pressed(void) {
    fflush(stdout);
    EsText *key = es_read_key(false);
    return key ? es_box_t(key) : NULL;
}

#endif
