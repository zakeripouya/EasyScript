# 0010: text in quotes inside brackets

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition (it was always an error before)

## The phrase

```
"... [VALUE with "text" in it] ..."
```

Inside a value in brackets, text in quotes is its own text when it ends, and a `]` comes after it, on the same line. Otherwise the quote ends the outer text, as before, and the `[` is reported as missing its `]`.

## Examples

```
before: let remark be parcel's note, or "no note".   say ..., "([remark])".
after:  say ..., "([parcel's note, or "no note"])".
```

## Checked against the design rules

- Deterministic: the lexer looks ahead on the line for the closing quote and the `]`; no guessing.
- Errors: a forgotten `]` keeps its message ("This "[" starts a value inside the text, but there's no "]" to end it.").

## Cognitive Dimensions notes

- Closeness of mapping: the text reads as it prints.
- Error-proneness: quotes inside quotes are harder to scan; the comma form (`"(", note, ")"`) still works.

## Open questions

None.
