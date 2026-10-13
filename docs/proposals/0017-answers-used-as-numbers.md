# 0017: an answer used as a number is asked for as a number

**Status:** Built
**Kind:** Addition. Using the answer of a plain `ask` as a number (or an amount, or yes or no) was always an error ("I can't multiply text by a number."); programs that compiled before keep their meaning, because they used the answer as text.

## The phrase

No new words. `ask P and call it NAME` (or `and call the answer NAME`), as before:

```
ask "What's the bill? " and call it bill.
ask "How many people? " and call it people.
say bill divided by people.
```

The answer's kind comes from how the program uses it, like any name's:

- used as a number: the sentence asks for a number, exactly like `ask for a number P and call it NAME` (asking again, with "Please type a number, like 42.", until the answer is one);
- used as an amount: like `ask for an amount`; used as yes or no: like `ask yes or no`;
- used only as text, or nothing decides: text, as before.

When the name already exists (`ask ... and call it total` where `total` is a number), the answer is asked for as that kind, where before it was an error.

What it means in core-language terms: the same as the `ask for a number` / `ask for an amount` / `ask yes or no` sentence the kind calls for.

## Examples

From the simulated testers ([report](../simulated-testers/report.md)): 31 of 120 attempts asked a question and did arithmetic with the answer. Every one was stopped by "I can't add text to text." (with "To join text, use "followed by"."), "I can't compare text with a number." or "... needs a number, but this is text.", and some then tried `Make balance a number.`

```
before: ask "Bill amount?" and call it bill.
        let tip be bill times 20 divided by 100.
        Line 2: I can't multiply text by a number.
after:  Bill amount? 86.40
        (tip is 17.28)
```

## Checked against the design rules

- One meaning per word: `ask` keeps its one meaning (ask a question, keep the answer); what's kept is decided by use, like every other name.
- Common words only, readable aloud: no new words.
- Distinct trigger words: no new pattern.
- Deterministic: the kind comes from the checker's usual rules; there's no guessing from the answer typed. An answer used as both text and a number is a kind mistake, reported as one.
- Exact meaning in the core: `ask for a number` (etc.), which already exist.
- Errors: unchanged wording; a kind mistake about an answer says where the kind was decided, as for any name.

## Cognitive Dimensions notes

- Closeness of mapping: a person who asks "How many people?" means a number.
- Hidden dependencies: the question asks again if the answer isn't a number because of a line further down. The message when it asks again ("Please type a number, like 42.") says what's wanted, and `ask for a number` remains for anyone who wants it written down.
- Consistency: every other name already gets its kind from use.

## Open questions

- None.
