# 0009: wordings from the "Still not English" list

**Status:** Built (reads-like-text, 2026-10-12), with the rest under Open questions in english-pass.md
**Kind:** Additions

## The phrases

```
the FIELD of the ITEM with the highest FIELD2      (was an error: the item's name ended at "of")
R has a FIELD of [at least|more than|...] N         how many students have a score of at least 60
X, minus N percent         X, plus N percent
the most common ITEM [in L]
```

`has a score of at least 60` is `R's score is at least 60`; `X, minus 10 percent` is `X times (100 minus 10) divided by 100`; `the most common drink` is the first of `the 1 most common drinks`.

## Examples

```
before: say "Best:", the student with the highest score's name.
after:  say "Best:", the name of the student with the highest score.
before: how many students whose score is at least 60        after: how many students have a score of at least 60
before: times (100 minus its discount) divided by 100        after: , minus its discount percent
before: the 1 most common names                              after: the most common name
```

## Checked against the design rules

- Distinct trigger words: `has a FIELD of` needs a known field with its `a`/`an` and `of` (a map's `has K` never has them); `, minus N percent` needs the comma and `percent`; `most common` without a number before it.
- Exact meaning in the core: a field comparison, arithmetic, and the existing most-common question.

## Cognitive Dimensions notes

- Closeness of mapping: these are the lines as a person would write them.

## Open questions

See english-pass.md's Still open: tables of records, indented output, two names in a loop, prompts' spaces, `the name is`, quotes inside brackets, text blocks in file sentences.
