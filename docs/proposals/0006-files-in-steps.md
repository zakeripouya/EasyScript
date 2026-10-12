# 0006: files in steps in a row, and "doesn't exist"

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition (a known exception to "file sentences say `file`", inside steps only)

## The phrase

```
..., then write X in it         ..., then add X to it [, one per line]       ..., then put X in it
..., then add X to the end of it|the file     ..., then show me what's in the file
file F doesn't exist     file F does not exist     (also folder)
```

Right after a step that made or wrote to a file, `it` and `the file` (no name) are that file: the step before's result, as for any `it` in steps in a row. `file F doesn't exist` is `not (file F exists)`.

## Examples

```
before: then write "milk" in file it, then add "eggs" to the end of file it, one per line
after:  then write "milk" in it, then add "eggs" to it, one per line
before: unless file "sales.csv" exists:       after: if file "sales.csv" doesn't exist:
```

## Checked against the design rules

- One meaning per word: `it` still means the step before's result; it's only accepted as a file where the step before was about one.
- Distinct trigger words: `add X to it` is a file sentence only right after a file step; elsewhere `it` keeps its loop meaning.
- Errors: "I don't know which file "the file" means here."

## Cognitive Dimensions notes

- Closeness of mapping: the groceries request reads as people say it.
- Hidden dependencies: `it` reaches back one step only.

## Open questions

- `the file` when the step before named two files (copying, renaming): those steps aren't file steps for `it`, so `the file` there is an error; it could name the new one.
