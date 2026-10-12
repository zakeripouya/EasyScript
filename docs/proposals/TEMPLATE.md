# NNNN: the phrase in a few words

**Status:** Proposed (Proposed / Accepted / Declined / Built)
**Kind:** Addition (a new phrase) / Change (to an existing phrase; say which, and what breaks)

## The phrase

The exact words, with the parts that vary in capitals:

```
if file F doesn't exist:
```

What it means in core-language terms (the plain loops, ifs and maps it stands for).

## Examples

Two or three lines from real programs (the examples, the guide, "Still not English"), before and after:

```
before: unless file "sales.csv" exists:
after:  if file "sales.csv" doesn't exist:
```

## Checked against the design rules

From [design-principles.md](../design-principles.md) (including "Rules for new phrases" and "Known exceptions") and the design rules in [english-pass.md](../english-pass.md). One line each; say how it passes, or why it's an exception:

- One meaning per word:
- Common words only, readable aloud:
- Distinct trigger words (no overlap with an existing pattern):
- Statements vs values, `of`/`in`, optional and "not found" results:
- Exact meaning in the core; intent kept as its own node where it matters:
- Errors: what a mistake with it says (grade 6 to 8, no programmer words):

## Cognitive Dimensions notes

Short notes, only on the ones that matter for this phrase ([the dimensions](https://en.wikipedia.org/wiki/Cognitive_dimensions_of_notations)):

- Closeness of mapping (is it how a person would say it?):
- Error-proneness (what will people get wrong?):
- Hidden dependencies:
- Role-expressiveness (can you tell what each word does?):
- Consistency (does it look like the phrases next to it?):
- Viscosity (is it easy to change later?):

## Open questions

Anything not settled.
