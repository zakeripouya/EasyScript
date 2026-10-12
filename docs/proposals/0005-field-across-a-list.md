# 0005: a field across a list, and counting a list field

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition

## The phrase

```
the FIELDs of L          each ITEM's FIELD
R has|have more than|at least|fewer than|less than|at most|exactly N FIELD
they   (in a definition: the record described)
```

`the names of L` and `each drink's name` are `each NAME in L's name` (a new list). `R has more than 5 orders` is `the number of items in R's orders is above 5`.

## Examples

```
before: say each drink in drinks's name.
after:  say the names of the drinks.
before: a customer is loyal if the number of items in their orders is above 5.
after:  a customer is loyal if they have more than 5 orders.
```

## Checked against the design rules

- Distinct trigger words: a plural that isn't a field, whose singular is one, then `of`; `each` + an item + `'s`; `has|have` + `more than` and the rest (the map test `M has K` never has these words).
- One meaning per word: `has` tests a map, defines a kind at a line start, and now counts a list field before `more than`: all mean "has" in English.
- Exact meaning in the core: a map over the list (`each ... in L`), and a length compared with a number.

## Cognitive Dimensions notes

- Closeness of mapping: these are the sentences people say.
- Error-proneness: a field whose plural is irregular ("person"/"people") isn't recognized; `each person's name` works.

## Open questions

- Plurals that don't follow the spelling rule (`people`, `children`).
