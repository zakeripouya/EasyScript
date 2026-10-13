# 0019: errors that answer words from spreadsheets and other languages

**Status:** Built
**Kind:** Error messages only. Every sentence below was an error before and is still one; only the note changes.

## What changes

From the simulated testers ([report](../simulated-testers/report.md)): people who live in spreadsheets wrote formulas, programmers wrote Python, and almost everyone grouped with "for each" or "by". The errors answered with "After a value I expected an operator like "plus" or "is"." Now each gets the EasyScript words for what it was trying to do.

| They wrote | Note |
|---|---|
| `SUM(hours)`, `AVERAGE(x)`, `MEAN`, `MEDIAN`, `COUNT`, `COUNTIF`, `SUMIF`, `MAX`, `MIN`, `ROUND(x, 2)`, `IF(c, a, b)`, `VLOOKUP`, `XLOOKUP`, `LEN` (any capitals, with the bracket straight after the word) | `EasyScript doesn't use "SUM(...)". Write "the total of ..." instead.` (`the average of ...`, `the median of ...`, `how many ... there are`, `how many ... are ...`, `the total of the ... whose ...`, `the biggest of ...`, `the smallest of ...`, `... rounded to 2 decimal places`, `A if C, otherwise B`, `the value for K in M`, `the number of items in ...`) |
| `name(x)` for any other word | `To use a definition, write "name with x" (or "name of x"), without brackets.` |
| `20%` at the end of a value | `"%" means the remainder after dividing. For a percent, write "20 percent".` |
| `x ^ 2`, `x ** 2` | `For a power, write "x to the power of 2".` |
| `"Total: " & total` | `To put text together, write "say "Total:", total" or use "followed by".` |
| `total hours for each employee`, `... by student`, `... grouped by day`, where the value ends | `To work it out for each employee, write "per employee" there.` |
| `repeat:` | `Something is missing after "repeat".` / `Say how many times, like "repeat 3 times:". To do something again and again, write "forever:" or "keep doing this until ...:", with the sentences indented below.` |
| `loop:` (and `loop over scores`: `To go through a list, write "for each item in scores:", ...`), `keep asking until ...` (a `keep` sentence with no `as` or `at`, whose next word ends in "ing" or which has `until` or `while`; `keep rate 0.2` keeps its constant error) | `I don't know a sentence that starts with "keep asking".` / `To do something again and again, write "forever:" or "keep doing this until ...:", with the sentences indented below.` |
| `ask me ...`, `ask the user ...`, `ask for ...` (but not `ask for a number`), where `and` doesn't follow (`ask user and call it name` still asks the text in the name `user`) | `I expected the question in quotes here, but found "me".` / `Write it like "ask "How many people?" and call the answer people".` |

`^` and `&` are characters EasyScript doesn't use, so they're the lexer's errors, with the note; the parser may still report the rest of that line.

## Checked against the design rules

- Errors: first person, the exact spot, one sentence to type instead (grade 8 or below, no programmer words; "definition" for `to` definitions, as voice.md says).
- Deterministic: nothing is read differently; `by` keeps its meanings (sorting, steps, dividing) and `for each` keeps starting loops.

## Cognitive Dimensions notes

- Closeness of mapping: each note answers in the terms the person used ("SUM" → "the total of").
- Habitability: the research behind the English pass says people rephrase blindly when an error doesn't say what works; these notes say what works.

## Open questions

- `X for each FIELD` (`the total hours for each employee`) as a synonym of `per`: 48 attempts grouped this way. Not built, because `for each` starts loops and "one meaning per word" would need an exception; logged under Still open in english-pass.md.
