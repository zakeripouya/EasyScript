# 0001: inputs in the middle of a definition's first line

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition, plus one change: `to adopt a dog:` (a kind after `a`, before any input word) now has the input `dog`.

## The phrase

```
to WORDS a KIND WORDS another KIND WORDS some NAME ...:
WORDS VALUE WORDS VALUE ...
reduce|increase|decrease X by N percent
```

`a KIND` is an input of that kind, called `the KIND` inside; `another KIND` a second one, `the other KIND`; `some NAME` an input called `the NAME`, its kind from use. Every other word is fixed. A call is a `to` function call with the values in order; `every [WORD] ITEM` in one place is a for-each loop over the list named like the item's plural around that call. `increase X by N percent` is `multiply X by (100 plus N) divided by 100`.

## Examples

```
before: to give discount to customer with percent:     give discount to ann with 10.
after:  to give a customer a discount of some percent:  give ann a discount of 10 percent.
before: for each customer in customers: if customer is loyal, give discount to customer with 10.
after:  give every loyal customer a discount of 10 percent.
```

## Checked against the design rules

- One meaning per word: `a` still means nothing in a sentence; in a first line, before a kind, it marks an input (a known exception, like `is a letter`). `another` and `some` mean what they mean in English.
- Common words, readable aloud: the call is the sentence a person would say.
- Distinct trigger words: a call is found by its fixed words, all of them, in order; two definitions fitting one sentence is an error naming both.
- Statements vs values: these are actions (sentences) only.
- Exact meaning in the core: a call of a `to` definition; a group is the same `for each` a person would write.
- Errors: a sentence that almost fits says which word is missing and quotes the definition.

## Cognitive Dimensions notes

- Closeness of mapping: the definition reads like the call, with "a customer" where the customer goes.
- Error-proneness: a fixed word that's a kind is an error (it might be meant as an input); two inputs in a row can't be told apart, so it's an error.
- Hidden dependencies: a call depends on every fixed word; a near miss names the definition.
- Role-expressiveness: `the customer`, `the other person` say which input is which.
- Consistency: `the percent percent` reads oddly but follows the rules (the input, then the word).
- Viscosity: changing a fixed word means changing every call; the near-miss error finds them.

## Open questions

- Values with inputs in the middle (`the tax on some amount`), not just actions.
- `some NAME` takes one word; names of several words need `some` before each? (Not supported yet.)
