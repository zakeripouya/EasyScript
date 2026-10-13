# 0016: "!" ends a sentence, and "please" after a step word

**Status:** Built
**Kind:** Addition. `!` on its own was always an error, and so was `please` after `then`, `first`, `next`, `finally` or `after that`.

## The phrase

```
say "Well done!"!
then please ask "How many people?" and call it people.
```

- `!` ends a sentence, exactly like `.` and `?` (`!=` is still "is not"). Unlike `?`, it doesn't make the sentence a question.
- Polite words and step words can come in either order, any number of times, before the sentence: `then please ...`, `please, first ...`, `first, can you ...`. All of them are skipped, as each one already was on its own.

What it means in core-language terms: nothing new; the sentence is the same as with a `.`, and without the step and polite words.

## Examples

From the simulated testers ([report](../simulated-testers/report.md)): 22 attempts ended sentences with `!` (`Thank you!`, `Hello!`), and 16 wrote `Then please ...`.

```
before: Thank you very much!
        Line 1: I don't understand "!". / To check that two things are different, write "!=".
after:  Line 1: I don't know a sentence that starts with "Thank". / Every sentence is something for the program to do, ...

before: Then please ask "How many people?" and call it people.
        Line 1: I don't know a sentence that starts with "please". / Did you mean "please"?
after:  (asks)
```

A greeting or a thank-you on its own is still an error (it isn't something to do), now with a note saying so: "Every sentence is something for the program to do, so leave out greetings and thanks." It's given for sentences starting with `hello`, `hi`, `dear`, `thanks` or `thank`.

## Checked against the design rules

- One meaning per word: `!` only ends a sentence; `!=` is lexed first, so it keeps its meaning.
- Common words only, readable aloud: `!` is ordinary punctuation; nothing new to say.
- Distinct trigger words: polite and step words were already skipped at a sentence's start; only their order is freed.
- Statements vs values: unchanged.
- Exact meaning in the core: the same sentence.
- Errors: `then please.` with nothing after it is "Something is missing after "please"." (as `please.` already was).

## Cognitive Dimensions notes

- Closeness of mapping: people end sentences with `!` and write "then please".
- Error-proneness: `if x ! y` (meaning `!=`) is now two sentences, `if x` and `y`, which is an error at the `if` ("I expected ":", "," or "then" after the condition"); that mistake was already an error.
- Consistency: `!` works like `?` and `.`.

## Open questions

- A whole sentence of thanks could be skipped silently instead of an error. It isn't: a sentence that does nothing is more likely a mistake than a courtesy, and the rule is that every sentence means something.
