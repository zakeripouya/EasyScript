# 0021: "EasyScript can't do that yet" for email, web pages, the internet and Excel files

**Status:** Built
**Kind:** Error messages, and one runtime check. Every sentence below was an error before; reading an Excel file as a table read its bytes as if they were a csv file.

## What changes

From the simulated testers ([report](../simulated-testers/report.md)): 20 attempts asked for something EasyScript can't do yet. None got an error saying so; they got "I don't understand the character "@".", "I expected a colon after this event, but found "email".", or a missing column in a workbook read as text.

| They wrote | Error |
|---|---|
| A sentence starting with `send` (with an email, mail or an address in it), `email` or `mail` (not `an email is ...`), or an event that sends one (`every monday at 9am email ...`); an email address outside quotes (`team@ourcompany.com`) | `EasyScript can't send email yet.` / `It can write the text into a file for you to send, like "write report in file "report.txt"".` (for an address outside quotes: `An email address goes in quotes, like "team@ourcompany.com".` / `EasyScript can't send email yet, but it can keep an address as text.`) |
| A sentence starting with `make`, `create`, `build`, `start`, `host`, `serve` or `publish` that has a web page, website, web server or web form in it (`make a web page ...`, `start the website ...`) | `EasyScript can't make web pages yet.` / `It can ask questions in the terminal, like "ask "What's your name?" and call it name".` |
| A web address in a program (`https://...` or `http://...`, in quotes or not) used to get or read something | `EasyScript can't use the internet yet.` / `It can read files on this computer, like "read the table in "data.csv" and call it data".` |
| `read the table in "x.xlsx"` (or `.xls`), and `open`, `load` or `import` with one | `EasyScript can't read Excel files yet.` / `Save the sheet as a csv file in Excel ("Save As", then "CSV"), and read that.` (`read file "x.xlsx"` still reads any file as text, as before) |
| While the program runs: a table whose file starts like an Excel workbook (the bytes `PK`, then 3 and 4), whatever its name | `EasyScript can't read Excel files yet, and book.dat is one, not a csv file.` (with the same note) |

A web address in quotes is otherwise just text: `say "https://example.com"` still prints it. The internet error is given when a sentence starting with `read`, `open`, `get`, `fetch`, `visit`, `download`, `load`, `look`, `go`, `check` or `browse` has a web address in quotes, and for a web address written without quotes anywhere. A sentence that starts with one of the program's own definitions is never one of these.

## Checked against the design rules

- Errors: say plainly what EasyScript can't do, then what it can; first person; no programmer words.
- Deterministic: these sentences were errors already, or always stopped when they ran (a web address read as a file is never there); the only exception is a workbook read with `read the table in`, which could print its bytes as rows, and is now an error. A word the program made into a name (`website`, a definition called `send`) is never read as one of these.
- Honest: the [English pass](../english-pass.md) plans email, web pages and the internet for Phase 4; "yet" is true.

## Cognitive Dimensions notes

- Closeness of mapping: a person asking to send an email learns that it can't, instead of trying to fix a stray "@".

## Open questions

- None.
