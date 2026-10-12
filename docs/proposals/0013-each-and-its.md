# 0013: for each K and its V

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition

## The phrase

```
for each KEY and its VALUE in M:
```

The same loop as `for each KEY and VALUE in M:`. `its` is left out of the value's name, unless the program writes `its VALUE` anywhere else (then the name stays `its VALUE`, its old meaning).

## Examples

```
before: for each class and average in the average score of students per class:
after:  for each class and its average in the average score of students per class:
```

## Checked against the design rules

- Deterministic: decided from the program's words, never from what the names mean.
- No change of meaning: programs that used `its VALUE` as the name keep it.

## Cognitive Dimensions notes

- Closeness of mapping: "each class and its average" is how it's said.

## Open questions

- `for each student with their score in ...` for lists of records.
