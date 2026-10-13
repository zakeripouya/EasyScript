# 0018: errors for file names without quotes and for opening a table

**Status:** Built
**Kind:** Error messages only. Every sentence below was an error before and is still one; only what the error says changes.

## The phrase

No new phrase. The sentence the messages lead to already exists:

```
read the table in "grades.csv" and call it grades.
```

## What changes

From the simulated testers ([report](../simulated-testers/report.md)): 58 of 120 attempts wrote a file's name without quotes, and 58 tried to open a csv file with words EasyScript doesn't have. None of them found `read the table in`.

1. **A file's name without quotes** (`grades.csv`, `bank.csv`, `listings.xlsx`: a word, a period and a word with no spaces between them, where the last word is a file ending EasyScript knows: csv, txt, svg, xlsx, xls, json, md, html, es) is one error at the whole name, instead of "Something is missing after "grades"." and "I don't know a sentence that starts with "csv".":

   ```
   Line 1: A file's name goes in quotes, like "grades.csv".
       read grades.csv.
            ^^^^^^^^^^
   To read a table, write "read the table in "grades.csv" and call it grades".
   ```

   The note about tables is given for csv files (except in `read the table in grades.csv`, where only the quotes are missing); Excel files get the note from [0021](0021-cant-do-yet.md); other files get the message only. The rest of the line is read with the name in quotes, and the parser says nothing more about that line, so the one error is the whole story.

2. **`open`, `load`, `import` and `get` at the start of a sentence** with a csv file get the table sentence as their note (with another file in quotes: `To read a file, write "read file "notes.txt" and call it notes".`), and **`read "grades.csv"`** is "I expected "file" or "the table in" here, but found the text "grades.csv"." with the same note:

   ```
   Line 1: I don't know a sentence that starts with "open".
       open the file "guests.csv".
       ^^^^
   To read a table, write "read the table in "guests.csv" and call it guests".
   ```

3. **Going through a whole csv file read as text** (`read file "x.csv" and call it data`, then `for each row in data:`) keeps "A "for each" loop needs a list to go through, but this is text." and adds: `"data" is the text of a file. To go through its rows, write "read the table in "x.csv" and call it data".`

The name suggested after `and call it` is the file's name without its ending, made into a name: lowercase, spaces for `_` and `-` (`best months.csv` gives `best months`). When that isn't a valid name (it starts with a digit, or is an EasyScript word), it's `rows`.

## Checked against the design rules

- One meaning per word: no new words; a period still ends a sentence (filenames go in quotes, as CLAUDE.md requires).
- Errors: first person, the exact spot, one sentence to type instead; grade 8 or below; no programmer words.
- Deterministic: the error shows one sentence; nothing is read differently.

## Cognitive Dimensions notes

- Closeness of mapping: people name files the way they see them in a folder; the error answers in the same terms.
- Error-proneness: one clear error instead of two that point at the wrong words.

## Open questions

- `open the table in "x.csv"` (or `load`) as a synonym of `read the table in`: not built. `read` works, and the error now shows it; a synonym can be added later if people keep typing it.
