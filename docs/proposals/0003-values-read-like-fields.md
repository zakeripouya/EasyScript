# 0003: adding with a condition, and values read like fields

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition

## The phrase

```
X, plus N if C        X, minus N if C        (built with 0002)
its NAME   the NAME of R   R's NAME   L sorted by NAME   the ITEM with the highest NAME
```

`X, plus N if C` is `X plus (N if C, otherwise 0)`. A value defined with `KIND's NAME is ...` or `the NAME of a KIND depends on ...:` is read like a field in every place a field is: it's a call of the definition with the record.

## Examples

```
before: a parcel's price is its base price plus (the extra of it) plus (the care of it).
after:  a parcel's price is its base price, plus 12 if it's abroad, plus 4 if it's fragile.
after:  say the parcels sorted by price.
```

## Checked against the design rules

- Distinct trigger words: the comma before `plus` separates it from arithmetic and from a sentence's trailing `if`; without `if` after N, nothing changes.
- Exact meaning in the core: a C conditional, and a call of the value's definition.
- One meaning per word: `plus` still adds.

## Cognitive Dimensions notes

- Closeness of mapping: "its price, plus 12 if it's abroad" is how a price list reads.
- Hidden dependencies: a value is worked out where it's used, so it's never out of date.
- Consistency: fields and values worked out from a record are used the same way.

## Open questions

None.
