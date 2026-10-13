# 0020: "rounded to 2 decimals" and "round X to 2 decimal places"

**Status:** Built
**Kind:** Addition. `rounded to N decimals` was an error ("I expected "decimal places" here"), and no sentence started with `round`.

## The phrase

```
say density rounded to 2 decimals.
round density to 2 decimal places.
round total.
round price up.
round price down to 1 decimal.
```

- `decimals` (and `decimal` after 1) means the same as `decimal places` / `decimal place` in `rounded to N ...`.
- `round X [up | down] [to N decimal places | decimals]` is a sentence that changes X in place: it's `set X to X rounded [up | down] [to N ...]`. X is a name (or a field, like the other change sentences).

What it means in core-language terms: `set X to X rounded to N decimal places`.

## Examples

From the simulated testers ([report](../simulated-testers/report.md)): 22 attempts rounded, as `rounded to 2 decimals`, `round density to 2 decimal places` and `round(x, 2)` (the last gets the note from [0019](0019-errors-for-other-languages-words.md)).

```
before: round density to 2 decimal places.
        Line 1: I don't know a sentence that starts with "round".
after:  (density is rounded)
```

## Checked against the design rules

- One meaning per word: `round` only rounds; `decimals` only counts decimal places.
- Statements vs values: `round X` (a sentence, changes X) and `X rounded` (a value, gives a new number) are the same pair as `sort L` / `L sorted`, as design-principles.md asks.
- Distinct trigger words: `round` starts the sentence. It starts sentences only when the program doesn't define its own `round` (like the library's other verbs: `rename`, `copy`, `move`), so a program with `to round with x:` keeps its meaning.
- Common words only, readable aloud: yes.
- Errors: `round` with nothing after it is "Something is missing after "round"."; rounding text is the usual kind error.

## Cognitive Dimensions notes

- Closeness of mapping: "round it to 2 decimals" is how people say it.
- Consistency: matches `sort` / `sorted`, and the change sentences (`increase X by N`).

## Open questions

- None.
