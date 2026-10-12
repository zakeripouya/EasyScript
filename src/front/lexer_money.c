// Amounts of money written with a currency sign: $1,200, $19.99, €4.50, £3.
// The token (TOK_MONEY) has the currency's word and the amount in cents as
// its text: "dollars 120000". Commas between groups of three digits are part
// of the amount (a comma followed by anything else isn't: "a list of $1, $2").
// At most two decimals; "4.50 dollars" (a number, then the word) is the parser's.

#include <string.h>
#include "front/lexer_internal.h"

#define MONEY_LIMIT 9007199254740992.0  // 2^53 cents: every amount is exact as a number too

static bool digit_at(const Lexer *lx, size_t i) {
    char c = lexer_peek(lx, i);
    return c >= '0' && c <= '9';
}

// The currency sign at lx->pos and its length in bytes, or 0.
static size_t currency_at(const Lexer *lx, const char **word) {
    const unsigned char *s = (const unsigned char *)lx->src + lx->pos;
    size_t avail = lx->len - lx->pos;
    if (avail >= 1 && s[0] == '$') return *word = "dollars", 1;
    if (avail >= 3 && s[0] == 0xE2 && s[1] == 0x82 && s[2] == 0xAC) return *word = "euros", 3;
    if (avail >= 2 && s[0] == 0xC2 && s[1] == 0xA3) return *word = "pounds", 2;
    return 0;
}

bool lexer_money(Lexer *lx) {
    const char *word = NULL;
    size_t sign = currency_at(lx, &word);
    if (!sign || !digit_at(lx, lx->pos + sign)) return false;
    size_t start = lx->pos, i = lx->pos + sign;
    double units = 0;
    for (;;) {
        while (digit_at(lx, i)) units = units * 10 + (lexer_peek(lx, i++) - '0');
        bool group = lexer_peek(lx, i) == ',' && digit_at(lx, i + 1) && digit_at(lx, i + 2) && digit_at(lx, i + 3) &&
                     !digit_at(lx, i + 4);
        if (!group) break;
        i++;  // "$1,200": the comma between thousands
    }
    double cents = units * 100;
    size_t decimals = 0;
    if (lexer_peek(lx, i) == '.' && digit_at(lx, i + 1)) {
        i++;
        while (digit_at(lx, i)) {
            if (decimals < 2) cents += (lexer_peek(lx, i) - '0') * (decimals == 0 ? 10 : 1);
            decimals++;
            i++;
        }
    }
    lx->pos = i;
    Span span = lexer_span(start, i);
    if (decimals > 2) {
        diag_error(lx->diag, span, "An amount of money has at most two decimals, for the cents.");
        diag_note(lx->diag, "Round it first, like \"%.*s\".", (int)(i - start - decimals + 2), lx->src + start);
        diag_info(lx->diag, DIAG_NOT_ALLOWED, NULL, NULL);
    } else if (cents >= MONEY_LIMIT) {
        diag_error(lx->diag, span, "This amount of money is too big for me to keep exactly.");
        diag_note(lx->diag, "Amounts can go up to 90 trillion.");
        diag_info(lx->diag, DIAG_NOT_ALLOWED, NULL, NULL);
        cents = 0;
    }
    if (decimals > 2) cents = 0;
    const char *text = arena_sprintf(lx->arena, "%s %.0f", word, cents);
    lexer_emit_content(lx, TOK_MONEY, start, i, text, strlen(text));
    return true;
}
