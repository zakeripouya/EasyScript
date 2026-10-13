# 0022: rounding after arithmetic says which part it rounds

**Status:** Built
**Kind:** Change. `A op B rounded ...` (with `plus`, `minus`, `times`, `divided by`, `mod` or their symbols, and no brackets or comma) used to round only B; now it's an error that shows both readings. No program in the repository's tests, examples or library wrote it.

## The problem

Found by the simulated testers ([report](../simulated-testers/report.md)): once their answers could be used as numbers, a teenager's mortgage calculator ran and printed the wrong payment.

```
let payment be loan * r / (1 - (1 + r) to the power of -n) rounded to 2 decimal places
```

The person meant the payment rounded to cents (1804.04). EasyScript rounded only the bracketed part, as its grammar says (a describing word after a value applies to that value), and printed 1794.87. A density calculator written the same way (`mass / volume rounded to 2 decimal places`) gave the right answer only because the volume was a whole number.

## The phrase

```
let payment be loan * r / (1 - (1 + r) to the power of -n) rounded to 2 decimal places.

Line 1: "loan * r / (1 - (1 + r) to the power of -n) rounded to 2 decimal places" could mean two things.
To round the whole value, write "loan * r / (1 - (1 + r) to the power of -n), rounded to 2 decimal places".
To round only the last part, write "loan * r / ((1 - (1 + r) to the power of -n) rounded to 2 decimal places)".
```

The rule: when the right side of an arithmetic operator is `X rounded`, `X rounded up`, `X rounded down` or `X rounded to N decimal places` and isn't in brackets, it's an error with both readings. Both readings already worked: a comma before `rounded` rounds the whole value (since part 2), brackets round only the part inside them. `say 2.5 rounded`, `(a / b) rounded` and `a times (b rounded)` are unchanged.

## Checked against the design rules

- Deterministic: "A sentence means one thing; ambiguity is an error with both rewrites." This was the one place where English and the grammar disagreed silently.
- Errors: the "could mean two things" shape from voice.md, with both sentences written out (and as choices in `easyscript errors`).
- Exact meaning in the core: unchanged for the two accepted forms.

## Cognitive Dimensions notes

- Error-proneness: removes a silent wrong answer; the cost is a comma or brackets.
- Closeness of mapping: "work it out, rounded to cents" is said with a pause, which the comma writes down.

## Open questions

- Other describing words after arithmetic (`a plus b as text`, `a plus b in uppercase`) have the same shape. They weren't changed: they're rarely wrong in a way that runs (text from `a plus b as text` is caught as a kind mistake when it's added to), and changing them would turn working programs into errors. Logged under Still open in english-pass.md.
