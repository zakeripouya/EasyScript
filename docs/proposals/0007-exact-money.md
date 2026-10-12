# 0007: exact money

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition, plus one change: `$5` used to be an error and `5 dollars` meant nothing.

## The phrase

```
$1,200   €4.50   £3   4.50 dollars   12 euros   1 pound
A plus|minus B   A times|divided by N   N percent of A   half of A   twice A   A divided by B
ask for an amount [in euros|pounds] Q and call it NAME
(dollars)  (euros)  (pounds)  a list of dollars  amounts in euros
```

Money is a kind per currency (`TYPE_DOLLARS`, `TYPE_EUROS`, `TYPE_POUNDS`), kept as whole cents in a 64-bit integer while the program runs, with an overflow check at 2^53 cents (90 trillion), so lists, maps and records keep amounts exactly as numbers of cents. Multiplying or dividing by a number rounds to the cent, half away from zero.

## Examples

```
before: let tip be (bill times tip rate) rounded to 2 decimal places.   say tip as money.
after:  let tip be bill times tip rate.                                 say tip.
```

## Checked against the design rules

- One meaning per word: `dollars` after a number is the currency; elsewhere it's an ordinary name.
- Exact meaning in the core: cents and C integers; the rounding is written down and shown by `easyscript plan`.
- Errors: adding a number to an amount, mixing currencies, multiplying two amounts, dividing a number by an amount, the remainder of an amount, too many decimals, amounts too big.

## Cognitive Dimensions notes

- Closeness of mapping: "$1,200" is how people write money; results print the same way.
- Error-proneness: `rent plus 3` is an error (did you mean $3?) instead of a silent mix.
- Hidden dependencies: a definition's input in arithmetic is a number unless written `(dollars)`.

## Open questions

- Changing between currencies (rates), and more currencies.
- Money as a map's key, and `a date or nothing`'s cousin `an amount or nothing`.
- `X as dollars` to turn a plain number into money.
