# 0004: the best group

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition

## The phrase

```
the FIELD with the highest|lowest total|average|median FIELD2 [of L]
the FIELD with the highest|lowest number of L
the top|lowest N FIELDs by total|average|median FIELD2 [of L]     ... by number of L
```

When FIELD is a field of the items (not a kind), it's `the first item of the keys of (the total FIELD2 of L per FIELD, sorted biggest first)`, or the first N keys. The parser writes exactly that (the question per group stays its own node). Without `of L`, L is the one list in reach whose items have FIELD; none or several is an error asking for `... of L`.

## Examples

```
before: let totals per store be the total amount of purchases per store, sorted biggest first.
        say "Top store:", the first item of the keys of totals per store.
after:  say "Top store:", the store with the highest total amount.
```

## Checked against the design rules

- Distinct trigger words: a field's name, `with the highest`, then `total`/`average`/`median`/`number of`; a kind's name keeps the old meaning (the item with the highest FIELD).
- Exact meaning in the core: per-group totals, a stable sort, the first key; ties go to the group that came first.
- Intent stays visible: the question per group is the same node as `per`.
- Errors: "There's no list whose items have a "store" to group by.", ""store" could come from more than one list here."

## Cognitive Dimensions notes

- Closeness of mapping: "which store did I spend the most at" is this question.
- Hidden dependencies: the list is found by the field, so adding a second list with the field makes it an error (it asks which).

## Open questions

- `the store I spent the most at` (a relative clause) isn't supported.
