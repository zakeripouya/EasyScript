# 0008: kinds of text with a rule

**Status:** Built (reads-like-text, 2026-10-12)
**Kind:** Addition

## The phrase

```
KIND is text that must contain|start with|end with|be ... .
X as KIND          (an email)          a list of emails
```

A kind of text is checked like text (it can be used anywhere text can), but only text that has passed its rule becomes one: kept in a field of that kind (a field called `email`, or written `(an email)`), or written `X as an email`. The rule is the same as `an email must contain "@"`, applied to every field of the kind and to `as an email`. Codegen keeps it as text.

## Examples

```
before: a customer's email must contain "@".
after:  an email is text that must contain "@".
        let typed be "bob@example.com" as an email.
```

## Checked against the design rules

- Distinct trigger words: `NAME is text that must` at a line start.
- Exact meaning in the core: the rule's check where the text becomes the kind; nothing anywhere else.
- Errors: "This has to be an email, but this is text." (with a note to write `as an email`), "Only text can become an email", and the rule's own message while running.

## Cognitive Dimensions notes

- Closeness of mapping: "an email is text that must contain @" is how a form describes it.
- Error-proneness: plain text can't slip in where an email is needed.

## Open questions

- Kinds of number with rules (`an age is a number that must be between 0 and 150`).
- `X is an email` as a test (does it pass the rule?).
