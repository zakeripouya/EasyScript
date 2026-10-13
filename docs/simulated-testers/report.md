# Simulated testers: the report

120 attempts (24 tasks × 5 kinds of people), written by agents that had never seen EasyScript, each with up to 3 tries; the plan and the method are in [plan.md](plan.md), every try is under [attempts/](attempts/). The compiler was master at `cd03c6a` (after the shell-and-money fixes), 2026-10-12.

**The short version: no attempt worked.** Not one of the 120 programs ran within 3 tries. The error messages helped once (errors fell from 1,259 on the first tries to 475 on the second) and then stopped helping (440 on the third). People who don't know EasyScript can't guess enough of it to finish a whole program, and the messages don't teach them the missing pieces. Most of the gap comes from a few phrasings, listed below; what was built from this report is in [english-pass-log.md](../english-pass-log.md#v02-prep).

## (d) Ran, but did something other than what was meant

**None during the run**, because nothing ran: an attempt has to get past every error before it can do the wrong thing. The nearest thing: `read the table in "listings.xlsx"` reads the workbook's bytes as if they were a csv file, so it stops with "listings.xlsx has no column called "price"." (or, for a workbook whose first line happens to look like a header, shows rows of unreadable bytes), and never says that it's an Excel file. Fixed in A4 ([proposal 0021](../proposals/0021-cant-do-yet.md)).

**One found afterwards.** Rerunning every third try with the A4 changes (see "After the fixes" below), six programs ran, and one of them gave a wrong answer without any error:

```
ask "price?" and call the answer price
ask "deposit?" and call the answer deposit
ask "rate?" and call the answer rate
ask "years?" and call the answer years
let loan be price minus deposit
let r be rate / 100 / 12
let n be years * 12
let payment be loan * r / (1 - (1 + r) to the power of -n) rounded to 2 decimal places
say loan
say payment
say payment * n
```

- **Meant:** the monthly payment, rounded to cents: 1804.04 (with 350000, 70000, 6% and 25 years).
- **Happened:** 1794.87. `rounded to 2 decimal places` rounded only the bracketed part (0.776... became 0.78), because a describing word after a value applies to that value.
- **Fixed** ([proposal 0022](../proposals/0022-rounding-after-arithmetic.md)): arithmetic whose right side is `... rounded ...` without brackets is now an error showing both readings (`..., rounded to 2 decimal places` for the whole value, or brackets for the part). The density calculator `mass / volume rounded to 2 decimal places` had the same shape and was right only because the volume was a whole number.

## The counts

| | (a) worked | (b) fixed by a suggestion | (c) stuck | (d) did something else |
|---|---|---|---|---|
| **All 120** | 0 | 0 | 120 | 0 |
| Has never coded (24) | 0 | 0 | 24 | 0 |
| Lives in spreadsheets (24) | 0 | 0 | 24 | 0 |
| A teenager (24) | 0 | 0 | 24 | 0 |
| Non-native English speaker (24) | 0 | 0 | 24 | 0 |
| Programmer from Python or JavaScript (24) | 0 | 0 | 24 | 0 |

**How far they got** (errors per try, summed; the checker only runs once a program has no parse errors, so these undercount):

| | Try 1 | Try 2 | Try 3 |
|---|---|---|---|
| Has never coded | 206 | 96 | 75 |
| Lives in spreadsheets | 233 | 93 | 64 |
| A teenager | 139 | 76 | 58 |
| Non-native English speaker | 167 | 72 | 91 |
| Programmer | 514 | 138 | 152 |
| **All** | **1,259** | **475** | **440** |

On the third try, 30 programs had one error left and 27 had two.

**What each kind of person did:**

- **Has never coded** wrote polite letters: "Hello! Please open the file guests.csv. ... Thank you!" Greetings, `!`, "then please", "ask me how many ...", and whole paragraphs as one sentence.
- **Lives in spreadsheets** wrote formulas: `SUM(...)`, `IF(service = "good", 20%, ...)`, `&` to join, `VLOOKUP`, "make a pivot table", "add a column".
- **A teenager** wrote the shortest thing: `read grades.csv`, `show average score for each student`, `who has best average?`. Closest to working, and stopped by file names and grouping.
- **Non-native speaker** wrote short sentences like the teenager's, plus `Total is 0.` and `Money is money plus add.` to make and change names.
- **Programmer** wrote Python: `=`, `f(x)`, `{}`, `[]`, `while True:`. Many errors per program, almost none about wording.

## The most common rejected phrasings

Counted by matching each attempt's programs and errors against a pattern (any try), so the numbers are close, not exact.

| # | What they meant | Attempts | What they wrote | Suggested wording | Kind |
|---|---|---|---|---|---|
| 1 | Use a typed answer as a number | 31 | `ask "Bill?" and call it bill` then `bill times 0.2` ("I can't multiply text by a number.") | Nothing new to write: an answer used as a number is asked for as a number, like `ask for a number` (asking again until it is one) | Addition (it was always an error) |
| 2 | Name a file | 58 | `read grades.csv`, `Open file guests.csv.` | `"grades.csv"` in quotes. The error should say so in one message, instead of "Something is missing after "grades"" and "I don't know a sentence that starts with "csv"" | Error rewrite |
| 3 | Open a csv file | 58 | `open`, `load`, `import`, `read file "x.csv"`, `read "x.csv"` | `read the table in "grades.csv" and call it grades`: the errors should show this sentence | Error rewrite |
| 4 | Work something out per group | 48 | `total hours for each employee`, `average score by student`, `grouped by`, "make a pivot table" | `the total hours of timesheet per employee`: the errors should show `per`. (`by` keeps its meaning: sorting, steps, dividing) | Error rewrite; `for each X` after a value is an open question |
| 5 | Use brackets and formulas | 37 | `SUM(hours)`, `round(x, 2)`, `prices["mug"]`, `{...}` | The EasyScript words: `the total of`, `the average of`, `X rounded to 2 decimal places`, `the value for "mug" in prices`. A spreadsheet formula's name should be answered with its EasyScript words | Error rewrite |
| 6 | Make or change a name | 16+ | `Total is 0.`, `score = 0`, `Money is money plus add.` | `let total be 0.` / `set total to ...` (the hint exists; `the name is "Ada"` is already an open question) | Open question (exists) |
| 7 | End a sentence with `!` | 22 | `Thank you!`, `say "Well done!"!` | `!` ends a sentence, like `.` and `?` | Addition |
| 8 | Be polite | 21 + 16 | `Hello!`, `Thank you very much.`, `Then please ask ...` | `please` after a step word is skipped like at the start; a greeting gets "Every sentence is something to do, ..." | Addition (`then please`); error rewrite (greetings) |
| 9 | Round | 22 | `rounded to 2 decimals`, `round density to 2 decimal places`, `round(x, 2)` | `rounded to 2 decimals` (a synonym of `decimal places`); the sentence `round X to 2 decimal places` (the value form already exists) | Additions |
| 10 | Repeat | 15 | `repeat:`, `loop:`, `keep asking until ...`, `while true:` | `forever:` / `keep doing this until ...:`; the error should say so (it suggested a name: "like "asking until guess right"") | Error rewrite |
| 11 | Percent, powers, joining | 18 | `20%`, `x ^ 2`, `x ** 2`, `"Total: " & total` | `20 percent`, `x to the power of 2`, `say "Total:", total` | Error rewrites |
| 12 | Ask in the person's own words | 9 | `Please ask me how many gloves we have.` | `ask "How many gloves do we have?" and call the answer gloves`: the error should show it ("I expected a value here, but found "me"" didn't help) | Error rewrite |

## The 10 worst error messages

These left people stuck: they either pointed at the wrong thing or suggested something that doesn't help.

| # | What they wrote | What they saw | How it should read |
|---|---|---|---|
| 1 | `Then please ask me how many people are sharing the bill.` | `I don't know a sentence that starts with "please".` / `Did you mean "please"?` | Not an error: `please` after `then` should be skipped like at the start |
| 2 | `Read file grades.csv.` | `Something is missing after "grades".` / `Write it like "read file "notes.txt" and call it notes".`, then `I don't know a sentence that starts with "csv".` | `A file's name goes in quotes, like "grades.csv".` / `To read a table, write "read the table in "grades.csv" and call it grades".` (one error) |
| 3 | `add yearly to money` (both answers from `ask`) | `I can't add text to text.` / `To join text, use "followed by".` | No error: an answer used as a number is asked for as a number. Where the answer is also used as text: `"yearly" is an answer from "ask" on line 2, so it's text. To get a number, write "ask for a number ..."` |
| 4 | `Keep asking until the guess is right.` | `"is" can't be part of a name, because it already means something in EasyScript.` / `Try a name without it, like "asking until guess right".` | `I don't know a sentence that starts with "keep asking".` / `To do something again and again, write "keep doing this until guess is secret:" with the sentences indented below.` |
| 5 | `read "orders.csv"` | `I expected "file" here, but found the text "orders.csv".` / `Write it like "read file "notes.txt" and call it notes".` (which then gives text, and `for each row in it` fails) | `To read a table, write "read the table in "orders.csv" and call it orders".` |
| 6 | `for each row in data:` (after `read file "x.csv" and call it data`) | `A "for each" loop needs a list to go through, but this is text.` | ... plus `"data" is the text of a file. To go through its rows, read it with "read the table in "x.csv" and call it data".` |
| 7 | `let tip be 20%` | `Something is missing after "%".` / `Add a value there, like a number, some text in quotes, or a name.` | `"%" here means the remainder after dividing. For a percent, write "20 percent".` |
| 8 | `Let TipRate be IF(Service = "good", 20%, ...)` | `I don't understand "(" here.` / `After a value I expected an operator like "plus" or "is", ...` | `EasyScript doesn't use "IF(...)". Write "20 if service is "good", otherwise 15".` (and the same for SUM, AVERAGE, COUNT, ROUND, MAX, MIN) |
| 9 | `Open file guests.csv.` | `I don't know a sentence that starts with "Open".` / `Sentences like these work: "make a new file called "notes.txt"", ...` | `To read a table, write "read the table in "guests.csv" and call it guests".` |
| 10 | `every monday at 9am email "bookings.csv" to "team@ourcompany.com"` | `I expected a colon after this event, but found "email".` | `EasyScript can't send email yet.` (and the same for web pages and the internet; see below) |

Close behind: `Thank you very much!` ("I don't understand "!"." / "To check that two things are different, write "!="."), `rounded to 2 decimals` ("I expected "decimal places" here"), `Please ask me how many ...` ("I expected a value here, but found "me"."), and `guest's plus ones` for a column called "plus ones" ("Something is missing after "'s"": `plus` is an EasyScript word, so that column can't be named).

## How the "can't do yet" tasks went

20 attempts (email, an Excel file, the internet, a web page). **None got an error saying EasyScript can't do it**, so all count as (c). What they saw instead:

- **Email:** `I don't understand the character "@".` (an address outside quotes), `I expected a colon after this event, but found "email".`, `I don't know a sentence that starts with "email"`.
- **An Excel file:** `Something is missing after the text "listings.xlsx".`, `I expected "file" here, but found the text "listings.xlsx".`. Worse, `read the table in "listings.xlsx"` would have read the workbook's bytes as a csv file (see (d)).
- **The internet:** `I don't understand the text "https://track.example-carrier.com/?id=" here.`, `I don't know a sentence that starts with "Look"`.
- **A web page:** `I don't know a sentence that starts with "make"`, `I don't know anything called "clicked"`.

**Suggested fix:** sentences about sending email, making a web page or website, getting a web address, and reading an Excel file should say `EasyScript can't ... yet.`, with what it can do instead (for Excel: save the sheet as a csv file).

## What this means

- **Discovery is the problem, not the grammar's size.** Each kind of person reached for the same handful of things (reading a file, a total per group, using an answer as a number), and EasyScript has all of them, under words nobody guessed (`read the table in`, `per`, `ask for a number`). The fix is in the messages: an error should show the sentence that does what the person was trying to do.
- **Three changes cover most of it:** answers used as numbers, file names and opening files, and grouping. What was built from this report is in the log's v0.2 prep section (A4).
- **The first non-coder test still matters.** Agents imitating people are a rough stand-in: real people read the docs, copy examples and ask for help. This run is about what someone types before any of that.

## After the fixes

The same 120 third tries, run again with the compiler after A4 (no new tries were written):

| | Ran | Errors |
|---|---|---|
| Master (`cd03c6a`) | 0 | 440 |
| After A4 | 6 | 376 |
| After the whole v0.2 prep | 3 | 386 |

The six that ran: the savings calculator and the budget check of the person who had never coded, both density calculators of the teenager and the non-native speaker, and both mortgage calculators of the person who had never coded and the teenager. Five did what was asked; the teenager's mortgage calculator is the (d) case above. (The budget check printed 12.5 months where the task wanted 13; that's the program's own arithmetic, `5000 divided by savings`, not a misreading.) None of them would have run without "an answer used as a number is asked for as a number". The rest still stop at things A4 didn't change, mostly whole programs written as letters or in Python, and unquoted words inside sentences.

After the whole v0.2 prep (A5, and the fixes the cookbook led to), three of the six no longer run, on purpose: the density calculators and the teenager's mortgage calculator round after a division without a comma, which is now the "could mean two things" error that shows both readings (the mortgage one was the wrong answer above). The count of errors went up a little because programs now get further before stopping: for example, the column `plus ones` is known now, so a program that used it reaches its next mistake.
