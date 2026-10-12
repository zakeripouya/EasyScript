# 0002: yes/no fields as describing words, and one-line choices

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition, plus one change: a yes/no field's name after `is` reads the field.

## The phrase

```
KIND has ..., and can be WORD.          KIND can be WORD.
a new WORD KIND with ...                mark X as [not] WORD
X is WORD   the WORD Ls   how many Ls are WORD   every WORD ITEM
A if C, otherwise B                     X, plus N if C      X, minus N if C
```

`can be WORD` is a field `WORD (yes or no, starting at no)`. `a new WORD KIND` gives it yes; `mark X as WORD` is `set X's WORD to yes`. `X is WORD` is `X's WORD`. `A if C, otherwise B` is a choice with no subject (the same code as a one-line table); `X, plus N if C` is `X plus (N if C, otherwise 0)`.

## Examples

```
before: a parcel has ... a fragile (yes or no) ...     after: a parcel has ..., and can be fragile.
before: ... fragile yes and note "glass"               after: a new fragile parcel with ...
before: set box's fragile to no.                       after: mark box as not fragile.
before: its base price plus (the extra of it) plus (the care of it)
after:  its base price, plus 12 if it's abroad, plus 4 if it's fragile
```

## Checked against the design rules

- One meaning per word: `can` + `be` (a field) and `can` + a verb + `:` (a method) start differently; `otherwise` keeps meaning "in the other case".
- Distinct trigger words: `mark` starts its own sentence; a choice needs `if` and `, otherwise` in that order; the comma separates `, plus N if C` from a sentence's trailing `if`.
- Exact meaning in the core: fields, `set`, and a C conditional, as written by hand.
- Errors: "The condition after "if" has to be yes or no", "Both values of a choice have to be the same kind", "Say what a parcel has before what it can be."

## Cognitive Dimensions notes

- Closeness of mapping: "a fragile parcel", "mark it as fragile", "4 if it's fragile, otherwise 0" are how people talk.
- Error-proneness: forgetting the comma before `otherwise` or `plus` gives the older meaning (a trailing condition or arithmetic), which usually fails to check.
- Consistency: yes/no fields and describing words a kind defines are used the same way.

## Open questions

- Fields whose kind is worked out from use (no `(yes or no)`) don't read as describing words, since the parser can't know they're yes/no.
