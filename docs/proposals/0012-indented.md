# 0012: say X, indented

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition

## The phrase

```
say PIECES, indented.          say PIECES, indented by N.
```

What's said, with 2 spaces (or N) before each of its lines.

## Examples

```
before: say "  ", student's name, student's score.
after:  say student's name, student's score, indented.
```

## Checked against the design rules

- One meaning per word: `indented` after the comma of a `say`; in a program that has a name `indented`, the bare form stays a piece (its old meaning).
- Errors: "How far to indent has to be a number", and while running "How far to indent has to be a whole number of spaces from 0 to 1000".

## Cognitive Dimensions notes

- Closeness of mapping: says what the spaces were for.

## Open questions

- Indenting a whole block of sentences (`indented:` with sentences under it).
