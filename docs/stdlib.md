# The standard library

Every phrase here works without `use`: they're sentence forms with their own words, not names a program could clash with. The few that are names (`pi`, `today`, `tomorrow`, `yesterday`) mean the library's value only where the program hasn't made that name itself. All of it is **Available**. The exact meaning of each phrase is given in core-language terms where it has one.

The library is written in C inside the runtime (`runtime/es_math.h`, `es_time.h`, `es_files.h`, `es_csv.h`): random numbers, the clock, files and folders need the system, and the math and csv parts are a few lines of C each that would be much slower written in EasyScript (the csv benchmark reads 100,000 rows; see [performance](performance.md)).

## Math

| Phrase | Gives | Example | Meaning |
|---|---|---|---|
| `the square root of X` | a number | `the square root of 16` is 4 | An error for a number below zero ("I can't take the square root of -4, because it's below zero.") |
| `X to the power of Y` | a number | `2 to the power of 10` is 1024 | `-2 to the power of 2` is -4 (the minus applies to the whole power, as in math). A number below zero to a power that isn't whole, and 0 to a power below zero, are errors. Two in a row need parentheses |
| `the absolute value of X` | a number | `the absolute value of -3.5` is 3.5 | X without its minus sign |
| `X rounded` | a number | `2.5 rounded` is 3, `-2.5 rounded` is -3 | To the nearest whole number; a half goes away from zero |
| `X rounded up` / `X rounded down` | a number | `2.1 rounded up` is 3, `-2.1 rounded up` is -2 | The next whole number up (or down), or X itself if it's whole |
| `X rounded to N decimal places` (or `1 decimal place`) | a number | `pi rounded to 2 decimal places` is 3.14 | Like `rounded`, at the Nth decimal place; N is a whole number from 0 to 15. `1.005 rounded to 2 decimal places` is 1.01 |
| `the bigger of X and Y` / `the smaller of X and Y` | a number | `the bigger of 3 and 4` is 4 | `if X is at least Y, X, otherwise Y` |
| `pi` | a number | `pi times 2 times radius` | 3.141592653589793. A constant can use it: `keep turn at pi times 2` |

`the square root of X`, `the absolute value of X` and the other prefixes take a single value, like `the length of`: `the square root of x plus 1` is `(the square root of x) plus 1`.

## Random numbers

| Phrase | Gives | Example | Meaning |
|---|---|---|---|
| `a random number from A to B` | a whole number | `a random number from 1 to 6` | Any whole number from A to B, both included, each as likely. A and B have to be whole, and A at most B |
| `a random decimal between A and B` | a number | `a random decimal between 0 and 1` | From A (included) to B (not included) |
| `pick one of L` / `a random item of L` (or `in L`) | an item | `pick one of cards` | `item (a random number from 1 to the number of items in L) of L`; an error for an empty list |
| `L shuffled` | a new list | `cards shuffled` | `a copy of L` with its items in a random order, every order as likely |
| `shuffle L.` | (a sentence) | `shuffle cards.` | The same, changing L itself |
| `roll a die` | a whole number | `say roll a die` | `a random number from 1 to 6` |
| `use the same random numbers every time.` | (a sentence) | `use the same random numbers every time.` | From here on, the random numbers are the same on every run: for testing a game, or a quiz that should repeat |

Each run starts with different random numbers, from the clock. When the tests run a program (with `ES_HOOKS`, see [SECURITY.md](../SECURITY.md)), `ES_RANDOM_SEED` fixes them too; programs built with `easyscript build` never read it.

## Dates and time

A **date** is a day, and sometimes a time of day: `today` is a day; `the time now` has the time too. Dates print readably: `Sunday, October 11, 2026`, and `Sunday, October 11, 2026, 2:05 PM` for one with a time of day. Dates can be compared with `is`, `is before` and `is after`, turned into text (`today as text`, `"Due " followed by tomorrow`), given to functions (`(a date)`) and given back, and kept in lists (`a list of dates`), maps (`a map from text to dates`) and records (`day, which is a date.`). Lists of dates can be sorted (earliest first), and `the biggest of` / `the smallest of` give the latest and earliest. Inside a list, map or record a date prints in parentheses, because it has commas of its own: `(Sunday, October 11, 2026) and (Monday, October 12, 2026)`. A date can't be a map's key, and can't be "or nothing".

| Phrase | Gives | Example | Meaning |
|---|---|---|---|
| `the time now` | a date | `say the time now` | Today, with the time of day (to the second) |
| `today`, `tomorrow`, `yesterday` | a date | `say tomorrow` | The day, without a time of day |
| `N days ago` / `N days from now` (or `1 day ...`) | a date | `3 days ago`, `2 days from now` | The day N days before (after) today; N is a whole number |
| `the year of D` (or `month`, `day`, `weekday`, `hour`, `minute`), or `D's year` | a number (the weekday: text) | `today's weekday` is `Sunday`, `the month of today` is 10 | The month is 1 to 12, the hour 0 to 23 |
| `D is before E` / `D is after E` | yes/no | `if yesterday is before today:` | Earlier or later, to the second |
| `D is in this month` (or `last month`, `next month`, `this year`, `last year`, `next year`) | yes/no | `if paid is in last month:` | D's month (or year) is that one, counted from today |
| `the number of days between D and E` | a number | `the number of days between today and holiday` | Whole days from one to the other, never below zero |
| `wait N seconds.` (or `milliseconds`, `minutes`, `a second`, `a minute`) | (a sentence) | `wait 1 second.` | Shows what's been printed, then pauses. From 0 seconds to a year |

When the tests run a program, `ES_NOW` (like `2026-10-11 14:05:00`) fixes the clock, and `wait` moves it on instead of pausing, so their output doesn't depend on the day.

## Files and folders

Every path is text, used as written: relative to the folder the program runs in (`"notes.txt"`, `"photos/cat.jpg"`), or absolute (`"/tmp/x"`). The words `file` and `folder` say what a sentence acts on, so the same sentences never mean something else for text or lists. Nothing is ever replaced: making, renaming, copying or moving onto a name that's taken is an error ("There's already a file called "b.txt"."), so delete it first.

| Phrase | Gives | Example | Meaning |
|---|---|---|---|
| `file F exists` / `folder F exists` | yes/no | `if file "settings.txt" exists:` | There's a file (a folder) at F |
| `put X in file F.` / `write X in file F.` / `write X to file F.` | (a sentence) | `put "hello" in file "notes.txt".` | The file holds X and a new line (made, or replaced) |
| `add X to the end of file F.` / `append X to file F.` | (a sentence) | `add "more" to the end of file "notes.txt".` | X and a new line go at the end (the file is made if it's not there) |
| `the contents of file F` | text | `say the contents of file "notes.txt"` | The whole file, without its last new line |
| `the lines of file F` | a list of text | `for each line in the lines of file "todo.txt":` | `the lines of (the contents of file F)` |
| `make a new file called F.` / `make a folder called F.` (or `a new folder`) | (a sentence) | `make a folder called "photos".` | An empty file (folder); an error if something's there, or the folder it goes in isn't |
| `rename the file F to G.` / `rename the folder F to G.` | (a sentence) | `rename the file "a.txt" to "b.txt".` | F is called G now (G can be in another folder) |
| `copy the file F to G.` | (a sentence) | `copy the file "a.txt" to "backup/a.txt".` | G is a new file with F's contents |
| `move the file F to the folder D.` (or `the folder F`) | (a sentence) | `move the file "a.txt" to the folder "old".` | F goes into D, keeping its name |
| `delete the file F.` | (a sentence) | `delete the file "old.txt".` | An error if F isn't a file |
| `delete the folder D.` | (a sentence) | `delete the folder "empty".` | Only an empty folder ("The folder "photos" isn't empty.") |
| `delete the folder D and everything in it.` | (a sentence) | `delete the folder "scratch" and everything in it.` | D and all the files and folders inside it. Links inside are removed, never followed. Paths that mean more than one folder (`"/"`, `"."`, `".."`, `"~"`, `""`) are refused |
| `the files in D` / `the folders in D` (or `in the folder D`) | a list of text | `for each name in the files in "photos":` | The names (not paths) of the files (folders) in D, in alphabetical order |
| `the size of file F` | a number | `the size of file "notes.txt"` | In bytes |
| `the last change of file F` | a date | `if the last change of file "log.txt" is in this month:` | When F was last changed, with the time of day |

Errors say what's wrong in plain words: "I couldn't find the folder "photos".", ""notes.txt" is a file, not a folder.", "I couldn't make the file "a/b.txt": the folder it should go in doesn't exist."

## CSV files

| Phrase | Example | Meaning |
|---|---|---|
| `read csv file F and call it NAME.` | `read csv file "sales.csv" and call it rows.` | NAME is a list of maps from text to text: one map per row, from each column's name (in the first row, the header) to that row's text, in the header's order |
| `save L as csv file F.` | `save report as csv file "report.csv".` | L is a list of maps from text to numbers, text or yes/no. The columns are the first map's keys, in order; every map needs the same keys. The file is replaced |

Reading follows the usual csv rules: a field in double quotes can hold commas, line breaks and quotes (written twice: `"say ""hi"""`); lines can end with `\n` or `\r\n`; blank lines are skipped. Saving quotes a field when it has a comma, a quote or a line break in it, or starts or ends with a space, and ends each line with `\n`. Values are text, so `(the value for "amount" in row) as a number` turns one into a number.

Mistakes name the file and its line: "In sales.csv, line 14 has 3 values, but the header has 4.", "In sales.csv, the quote on line 7 never ends.", "The header of sales.csv has the column "name" twice.", "sales.csv is empty, so it has no header row."; and when saving, "Row 3 of the list has no value for "region", but the first row does."

## The screen

| Phrase | Example | Meaning |
|---|---|---|
| `clear the screen.` | `clear the screen.` | Clears the terminal and puts the cursor at the top. When the output goes to a file or another program instead, it does nothing |
