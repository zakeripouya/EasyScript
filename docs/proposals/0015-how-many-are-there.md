# 0015: how many Ls are there?

**Status:** Built
**Kind:** Addition: `how many Ls are there` was always an error unless the program made a name or a describing word called `there`, and those keep their meaning.

## The phrase

```
how many L are there
how many L is there
```

As a value, with nothing after it (`say how many purchases are there.`, `"[how many purchases are there]"`), or as a question with its question mark (`how many purchases are there?`). It means exactly `how many L there are`: the number of items in L (`the number of items in L`, kept as the `how many` question so it's counted in one pass).

`there` keeps any meaning the program gives it: when the program makes a name `there` (`let there be 5`, then `how many scores are there` counts the scores that are 5), keeps a constant or a definition by that name, or defines a describing word `there` (`a purchase is there if ...`) or a yes/no field `there`, the old reading wins, as describing words do today. The parser marks the form; the checker decides, since only it knows every name.

## Examples

```
before: say how many purchases there are.
after:  how many purchases are there?
```

## Checked against the design rules

- One meaning per word: `there` means "exists" in both orders; a program's own `there` wins (a known exception, like `is a letter`).
- Common words only, readable aloud: the question people actually type.
- Distinct trigger words: `how many` starts it, as before.
- Exact meaning in the core: the number of items in L.
- Errors: unchanged. `how many purchases are big?` with no `big` still says "I don't know what "big" means for a purchase."

## Cognitive Dimensions notes

- Closeness of mapping: "how many purchases are there?" is the question form of "how many purchases there are".
- Hidden dependencies: a describing word or name called `there` changes it; that's rare, and it's the rule describing words already follow.
- Consistency: matches `how many Ls are TEST`.

## Open questions

- **A question without its question mark** (`how many purchases are there` on its own line, as people type in the shell): only a sentence ending in `?` is a question, so it's "I don't know a sentence that starts with "how"." (the same as `how many purchases there are` on its own). Suggested: a sentence starting with `how many` is a question with or without the mark, since no other sentence starts with `how`.

- `how many tasks are left`: "left" also means the side (`how many socks are left`), which a program can define as a describing word or a field. Not built: it's two meanings in plain English, not just in a program. Suggested: build it with the same rule as `there` if user testing shows people type it for "the items still in the list".
