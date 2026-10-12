# 0011: put the text below in a file

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition

## The phrase

```
put the text below in file F:        write the text below to file F:
    line one
    line two
```

A `put` or `write` sentence with `the text below` in it and a colon at its end takes the indented lines as its text, like `let X be the text below:`.

## Examples

```
before: let rows be the text below:          after: put the text below in file "spending.csv":
            date,store,amount                           date,store,amount
        put rows in file "spending.csv".
```

## Checked against the design rules

- Deterministic: only `put`/`write` lines, only with `text below` and a final colon; such a line was an error before.

## Cognitive Dimensions notes

- Closeness of mapping: the file's contents are written where they go.

## Open questions

- `add the text below to the end of file F:`.
