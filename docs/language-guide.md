# The EasyScript Language Guide

This guide teaches EasyScript from zero, one chapter at a time. Each chapter is marked with its status:

- **Available:** you can write and run everything in the chapter today.
- **Coming soon:** the chapter describes the *planned* language. Its examples don't compile yet, and the exact wording may change. They're here so you can see where the language is going and give feedback.

The full list of words and patterns is in the [vocabulary](vocabulary.md).

| Chapter | Status |
|---|---|
| [0. Getting set up](#0-getting-set-up) | Available |
| [1. How EasyScript reads your sentences](#1-how-easyscript-reads-your-sentences) | Available |
| [2. Values and expressions](#2-values-and-expressions) | Available |
| [3. Variables](#3-variables) | Available |
| [4. Output and input](#4-output-and-input) | Available |
| [5. Decisions](#5-decisions) | Available |
| [6. Loops](#6-loops) | Available |
| [7. Functions](#7-functions) | Available |
| [8. Files](#8-files) | Available |
| [9. Types](#9-types) | Available |
| [10. Lists](#10-lists) | Available |
| [11. Text](#11-text) | Available |
| [12. Records](#12-records) | Available |
| [13. Maps](#13-maps) | Available |
| [14. Kinds with variants](#14-kinds-with-variants) | Available |
| [15. Functions as values](#15-functions-as-values) | Available |
| [16. Interfaces](#16-interfaces) | Available |
| [17. Programs in several files](#17-programs-in-several-files) | Available |
| [18. The standard library](#18-the-standard-library) | Available |
| [19. Programs and the system](#19-programs-and-the-system) | Available |
| [20. The keyboard](#20-the-keyboard) | Available |
| [21. Questions and totals](#21-questions-and-totals) | Available |
| [22. Choices without if](#22-choices-without-if) | Available |
| [23. Talking to it](#23-talking-to-it) | Available |

---

## 0. Getting set up

**Status: Available**

Build the compiler (you need `cc`, `make`, and macOS or Linux):

```bash
git clone https://github.com/zakeripouya/EasyScript.git
cd EasyScript
make
```

EasyScript programs are plain text files ending in `.es`. Write this into `hello.es`:

```
say "Hello, world!".
```

and run it:

```bash
./easyscript run hello.es
```

`easyscript build hello.es -o hello` makes a program you can run on its own (`./hello`). Running `./easyscript` with nothing after it starts the interactive shell, where you can type sentences and see the results straight away; end a line with `:` to type a block, and finish the block with a blank line. Two more commands show what the compiler sees: `easyscript tokens` (the words and symbols) and `easyscript ast` (the meaning, as a tree).

---

## 1. How EasyScript reads your sentences

**Status: Available.** This chapter is about how sentences are read: words, numbers, text, comments, and blocks. You can watch it happen with `easyscript tokens`. What the sentences *mean* comes in the later chapters.

### Sentences

A program is a list of sentences. A sentence ends with a period or at the end of the line:

```
Set the total to 0.
add 5 to total
Say the total.
```

Two sentences can be joined with `and` (or `and then`), and they run one after the other: `add 5 to total and say total.` Between two values `and` still means "both" (`if ready and total is 5`); it starts a new sentence only when a sentence word like `say`, `add` or `stop` follows it, with a value after that word. A one-line `if` keeps both: `if done, say "bye" and stop the loop.` To call a function of your own as the second sentence, write `and then`: `greet with "Ann" and then greet with "Bo".`

A long sentence can go on over several lines: when a line ends with a comma, `then` or `and`, the sentence carries on to the next line, however that line is indented:

```
let colors be a list of "red", "green",
    "blue" and "yellow".
if ready then
    say "Off we go.".
```

Capital letters don't matter for words, so `Set` and `set` are the same word. The words `the`, `a`, and `an` are ignored, so you can write naturally: `set the total to 0.` and `set total to 0.` are the same sentence. Because they're ignored, `the`, `a`, and `an` can't be used as names. (One phrase needs its `a`: `is a letter`, and `is a digit` and `is a space`, so that `x is letter` can still compare `x` with a variable called `letter`; see [Text](#checking-characters).)

Try it: [`examples/lexer/sentences.es`](../examples/lexer/sentences.es). The output lists every word the compiler saw, and `the` isn't among them.

### Names

A name starts with a letter and can contain letters, digits, and underscores: `total`, `player2`, `clientName`. You never need underscores, though: a name can be several words (see below), and error messages always suggest names in plain words. An apostrophe can sit between letters, so `guest's` and `isn't` are single words.

A name can also be **several words**: `let late fee be 25.`, `keep hourly rate as 150.`, `count from 1 to 3 as session number:`. Once the program makes a name like that (with `let`, `keep`, `to`, `as`, `call`, or `for each`), EasyScript recognizes it everywhere, always taking the longest name it knows: if the program has both `total` and `total cost`, then `total cost plus 1` uses `total cost`. Words that already mean something in a sentence (like `plus`, `is`, `and`, `of`, `with`, `to`, `in`, `be`) can't be part of a name of several words; trying it is an error that suggests another name:

```
Line 1: "plus" can't be part of a name, because it already means something in EasyScript.
    let price plus tax be 5
              ^^^^
Try a name without it, like "price tax".
```

Names are matched without caring about capitals, so `clientName`, `clientname` and `CLIENTNAME` are the same name; messages show a name the way you first wrote it.

There are no reserved words for single-word names. Whether `count` is a command or a variable depends on where it appears in the sentence, so you can name a variable `count`.

### Numbers

Whole numbers (`42`) and decimals (`19.99`). A period after a number ends the sentence; it's only a decimal point when a digit follows it:

```
set price to 19.99.
set count to 3.
```

A number and a word need a space between them: `3times` is an error, `3 times` is fine. The one exception is an ordinal like `3rd` or `21st`, used in `every 3rd item of scores` ([Lists](#10-lists)); its ending has to match the number, so `2th` is an error that suggests `2nd`.

Try it: [`examples/lexer/numbers.es`](../examples/lexer/numbers.es)

### Text

Text goes in double quotes, and must end on the same line. Inside text, these escapes are available:

| Write | To get |
|---|---|
| `\n` | a new line |
| `\t` | a tab |
| `\"` | a double quote |
| `\\` | a backslash |
| `\r` | back to the start of the line |
| `\u{e9}` or `\u00e9` | the character with that code, in hex (here `é`) |

```
say "Two lines:\nfirst\nsecond".
say "She said \"hi\"".
```

For text of several lines, end a sentence with `the text below:` and indent the lines under it. They're kept exactly as written: no escapes, no values in brackets, and the indentation they share is taken off.

```
let page be the text below:
    {"name": "Ann", "path": "C:\notes"}
    [this is just text]
say page.
```

Try it: [`examples/lexer/text.es`](../examples/lexer/text.es)

### Comments

Anything after `#` is ignored, and so is any line that starts with `note:`:

```
note: this whole line is a comment.
say "hello". # and so is this part
```

Try it: [`examples/lexer/comments.es`](../examples/lexer/comments.es)

### Blocks

A sentence ending in a colon opens a **block**: the indented lines below it. The block ends when the indentation goes back:

```
if total is greater than 10:
    say "big".
    if total is greater than 100:
        say "huge".
otherwise:
    say "small".
```

Use spaces or tabs (a tab counts as 4 spaces). When a block ends, the next line has to line up exactly with an enclosing block. Anything else is an error, and the message tells you which indentations would work.

Try it: [`examples/lexer/blocks.es`](../examples/lexer/blocks.es) and the error example [`examples/lexer/err_bad_indentation.es`](../examples/lexer/err_bad_indentation.es)

### Symbols

EasyScript also understands a few symbols: `+ - * / %` for arithmetic, `( )` for grouping, `= == != < > <= >=` for comparisons, and `,` for lists of things. See [`examples/lexer/comparisons.es`](../examples/lexer/comparisons.es).

---

## 2. Values and expressions

**Status: Available.** The examples show expressions on their own; in a program they appear inside sentences, such as `say price plus tax` (chapter 4).

### Values

| Kind | Examples |
|---|---|
| Numbers | `42`, `19.99` |
| Text | `"Hello, world!"` |
| Yes and no | `yes`, `no` (or `true`, `false`) |
| Lists | `a list of 72, 95 and 88`, `an empty list of text` ([chapter 10](#10-lists)) |
| Names | `total`, `player_2`, `guest's` |

Every value has one of these kinds: a number, text, or yes/no. The compiler works out the kind of every value before the program runs ([chapter 9](#9-types)). `nothing` is only what a function gives back when it doesn't give back a value; it can't be used as a value itself.

### Arithmetic

You can use words or symbols:

```
price plus tax
total minus discount
width times height
total divided by count
minutes mod 60
-temperature
```

`plus`/`+`, `minus`/`-`, `times`/`multiplied by`/`*`, `divided by`/`/`, and `mod`/`%` (the remainder). They work on numbers; `say 7 divided by 2` prints `3.5`, and whole numbers print exactly, without decimals, up to 9007199254740992 (the biggest a computer number holds every whole number up to), so `1000000 times 1000000 times 1000` prints `1000000000000000`. Bigger ones print like other numbers. Other numbers print with at most 10 significant digits, so tiny rounding errors never show: `0.1 plus 0.2` prints `0.3`, and `1 divided by 3` prints `0.3333333333`. A number that rounds to a whole number prints as one (`0.99999999999` prints `1`). Only the printing rounds: the number itself keeps its full precision. Using text in arithmetic is caught before the program runs:

```
Line 1: I can't add text to a number.
    say 1 plus "2"
        ^^^^^^^^^^
To join text, use "and" or "followed by".
```

Dividing by zero can only be seen while the program runs, so it stops the program with a message that shows which line went wrong, like a compile error:

```
Line 3: You divided by zero.
    say 12 divided by people
        ^^^^^^^^^^^^^^^^^^^^
``` As in maths, multiplying and dividing come first: `price plus tax times 2` means `price plus (tax times 2)`. Use parentheses to change that: `(price plus tax) times 2`.

Try it: `./easyscript ast examples/parser/arithmetic.es`

### Comparisons

A comparison is yes or no. There are several ways to say each one, so you can pick whichever reads best:

```
age is at least 18
score is greater than or equal to target
name isn't ""
lives is below 1
points reaches 100
```

The full list is in the [vocabulary](vocabulary.md#comparisons). Numbers that differ only by tiny rounding errors count as equal, so `0.1 plus 0.2 is 0.3` is `yes`. A sentence can hold one comparison at a time: instead of `1 < x < 10`, write `1 < x and x < 10`. The compiler tells you this if you forget.

### Combining yes and no

`and`, `or`, and `not` combine comparisons. `not` comes first, then `and`, then `or`:

```
age is at least 13 and age is under 20
day is "sat" or day is "sun"
not done and ready
```

### Working with text

There are three ways to put values into text, and numbers (or lists) are turned into text for you:

- **Commas in `say`:** `say "Hello,", name, "!"` prints the pieces with a space between them, except next to punctuation, so it prints `Hello, Ann!`. `show`, `print`, `display` and `complain` take pieces too.
- **Brackets in text:** `"Hello, [name]! You have [tasks plus 1] tasks."` works out each value in brackets and puts it exactly there. Write `[[` and `]]` for a bracket in the text itself.
- **`followed by`** joins two values exactly as they are, anywhere: `"Total: " followed by total`.

`and` doesn't join text: it means "both", for yes or no values. Joining happens after the maths, so `"Sum: " followed by 2 plus 3` gives `Sum: 5`. A value can be turned into text with `as text`, and text into a number with `as a number`:

```
"Hello, [name]!"
"Total: " followed by total as text
"42" as a number plus 1
length of name
contents of file "notes.txt"
```

`as a number` also reads a power of ten written with `e`: `"1e5" as a number` is 100000. Numbers print with at most 10 significant digits, so `0.1 plus 0.2` shows `0.3`; when every digit matters (a number written to a file and read back), use `as exact text`, which gives the shortest digits that read back as exactly the same number (`(0.1 plus 0.2) as exact text` is `0.30000000000000004`, and `100000000000000000000 as exact text` is `1e20`).

Taking text apart, searching it, splitting it into words and changing its case have [a chapter of their own](#11-text).

Try it: `./easyscript ast examples/parser/text.es`

### Calling functions

A function (chapter 7) is called by its name, with `of` (or `with`) and its inputs separated by `and`:

```
area of 3 and 4
greet with "Ada"
```

Each input is a single value. If arithmetic comes straight after, EasyScript can't tell whether it belongs to the last input or to the result, so it asks:

```
Line 2: "double using 21 plus 1" could mean two things.
    say double using 21 plus 1
        ^^^^^^^^^^^^^^^^^^^^^^
Use parentheses to say which: "(double using 21) plus 1" or "double using (21 plus 1)".
```

This is the "no guessing" rule in action. See [`examples/parser/`](../examples/parser/) for more.

---

### Memory

There's nothing to manage. Text made while the program runs, like a join or a number turned into text, is freed as soon as nothing uses it any more, so a loop that makes a million texts uses no more memory than one that makes one. See [How EasyScript manages memory](memory.md).

## 3. Variables

**Status: Available.**

A variable is a named value. `let` creates one, and `set` changes it:

```
let total be 0.
let name be "Ada".
set total to 10.
```

`let total equal 0` means the same as `let total be 0`, and `change total to 10` the same as `set total to 10`. As everywhere, `the` is ignored, so `let the total be 0` reads naturally.

Changing a number has its own sentences:

```
add 5 to total.
subtract 1 from total.
multiply total by 2.
divide total by 4.
```

`increase total by 5` and `decrease total by 1` mean the same as `add` and `subtract`.

A name is one word. It can't be `a`, `an`, or `the`, because EasyScript ignores those words, and the compiler explains this if you try:

```
Line 2: "a" can't be used as a name.
    let a be 5
        ^
EasyScript ignores the words "a", "an" and "the" wherever they appear, so they can't be names.
Try a name that says what it holds, like "total" or "answer".
```

Words that already mean something, like `yes`, `nothing`, or `it`, can't be names either.

A variable has to be made with `let` before it's used, and only once. Using a name that doesn't exist is an error that suggests the closest name you did make:

```
Line 4: I don't know anything called "totl".
    add 5 to totl.
             ^^^^
Did you mean "total"? You made it on line 1.
```

A variable keeps the kind of value it was made with. Storing a different kind in it is an error, caught before the program runs:

```
Line 2: "total" is a number (made on line 1), but this is text.
    set total to "zero"
                 ^^^^^^
A name keeps the kind of value it was made with. To keep text, make a new name for it.
```

### Values that never change

A **constant** is a name for a value that stays the same for the whole program. Make one with `keep`:

```
keep tax rate as 0.2.
keep shop at "Corner Shop".
keep welcome as "Welcome to " followed by shop.
say welcome.
```

A constant's value has to be known before the program starts, so it can use numbers, text, yes and no, lists of those written out (`keep weekdays as a list of "Mon", "Tue" and "Wed".`, a fixed list that can't change; see [Lists](#10-lists)), and other constants, but not variables or anything the program works out while it runs. Trying to change a constant is an error that suggests `let` instead. Constants can be used everywhere, including inside functions (chapter 7).

Try it: `./easyscript run examples/programs/variables.es`

---

## 4. Output and input

**Status: Available.**

`say` prints a value on its own line. `print`, `show`, `display`, and `write` mean the same thing:

```
say "Hello, world!".
print total.
show "Total: " followed by total as text.
```

`ask` prints a question and keeps the line of text the user types (if there's nothing more to read, the answer is empty text). The answer is always text; turn it into a number with `as a number`:

```
ask "What's your name? " and call the answer name.
say "Nice to meet you,", name, ".".
ask "How old are you? " and call the answer age.
say "Next year you'll be", age as a number plus 1, ".".
```

`and call it name` works too, as in `read file`.

Any sentence can begin with `please`, which is skipped: `please say "hi"`.

`stop the program` ends the program straight away.

If the first word of a sentence isn't one EasyScript knows, the compiler suggests the closest one:

```
Line 2: I don't know a sentence that starts with "sya".
    sya "hello"
    ^^^
Did you mean "say"?
```

Try it: `./easyscript run examples/programs/ask_name.es`

---

## 5. Decisions

**Status: Available.**

```
let temperature be 24.
if temperature is greater than 30:
    say "It's hot.".
otherwise if temperature is less than 10:
    say "It's cold.".
otherwise:
    say "It's nice.".
```

End the `if` line with a colon, and indent the sentences that belong to it (4 spaces is usual). The block ends where the indentation goes back. `otherwise if` adds another choice, checked only if everything before it was no; a plain `otherwise` (or `else`) comes last and catches everything else. An `otherwise` has to start in the same column as its `if`, and blocks can hold more `if`s.

For a single sentence, you can stay on one line with a comma or `then`:

```
if temperature is 24, say "Exactly 24.".
if temperature is at least 20 then say "Maybe go for a walk.".
```

`unless` is the opposite of `if`: its sentences run when the condition is no. It can't have an `otherwise` (turn it into an `if` instead):

```
unless temperature is above 30:
    say "No need for the fan.".
unless temperature is below 0, say "No ice today.".
```

The conditions are the comparisons from [chapter 2](#2-values-and-expressions), and they must be yes or no. `if temperature:` on its own is an error, caught before the program runs, because a number isn't yes or no.

A name made inside a block only exists inside it. If you need it afterwards, make it before the `if` and change it inside with `set`.

Try it: `./easyscript run examples/programs/decisions.es`

If a sentence could be read two ways, EasyScript doesn't guess. It stops and shows you the possible readings so you can say which one you meant.

---

## 6. Loops

**Status: Available.**

A loop repeats the indented sentences below it. To count:

```
count from 1 to 3:
    say number.
```

says 1, 2 and 3. The number is called `number`, or `it`; give it your own name with `as` (or `and call each number`), and count in bigger steps with `by` or `in steps of`:

```
count from 0 to 100 by 25 as percent:
    say percent followed by "%".
```

Counting goes up: if the start is bigger than the end, the count runs zero times, so `count from 1 to the number of items in scores` does nothing for an empty list. To count down, say so: `count down from 3 to 1` says 3, 2, 1. (A count written with two numbers that can never run, like `count from 10 to 1:`, gets a warning suggesting `count down`.) `for each i from 1 to 10:` and `go from 0 to 1 in steps of 0.25:` are other ways to say the same thing. `for each score in scores:` goes through a list instead (see [Lists](#10-lists)); there, `it` is the item.

To repeat something a number of times, or while something is true:

```
repeat 3 times:
    say "hip hip hooray!".

let total be 1.
while total is less than 100:
    multiply total by 2.
say total.
```

There's also `as long as` (the same as `while`), `keep doing this until` (the opposite), and `forever`. Inside a loop, `stop the loop` leaves it straight away, and `skip this one` jumps to the next round:

```
forever:
    ask "Guess? " and call the answer guess.
    if guess is "42", stop the loop.
    say "Try again.".
say "You got it!".
```

Names made inside a loop, and the loop's number, only exist inside the loop. Nested counting loops each need their own name for the number, like `count from 1 to 3 as row:` and `count from 1 to 3 as column:`.

Try it: `./easyscript run examples/programs/loops.es`

---

## 7. Functions

**Status: Available.**

A function is a named set of steps. It's defined with `to`, like a recipe ("to make tea: ..."), and its sentences are indented below. Its name is every word up to `with`, `of`, `using`, `to`, `for`, `from` or `in`, which introduce its inputs:

```
to make tea:
    say "Boil the water.".

to greet with someone:
    say "Hello,", someone, "!".

make tea.
greet "Ada".
greet with "Alan".
call greet with "Grace".
```

So `to make tea:` is a function called `make tea` with no inputs, and `to greet with someone:` is `greet` with the input `someone`. (Writing `to greet someone:` makes a function called `greet someone`; the error you get when you then use `someone` explains how to make it an input.) A function's name can be several words, and if its inputs are introduced with `to`, `for`, `from` or `in`, it's called with that word too (or with `with`, `of` or `using`, which work for every function):

```
to add tax to amount:
    give back amount plus amount times 0.25.

to pause for seconds:
    say "Waiting " followed by seconds followed by " seconds".

to busiest artist from names:
    give back the first item of names.

say add tax to 100.
add tax to 100.
pause for 3.
say busiest artist from a list of "Nina" and "Ray".
```

Calling a function with another of these words (`pause from 3`) is an error that says which words it takes. `for` followed by a kind of record makes a method instead (see [Methods](#methods)), and `for` always ends a function's name, so `to print receipt for customer:` is `print receipt` with the input `customer`.

A sentence that starts with a function's name calls it, even if its first word usually starts other sentences (like `add`).

A function can take several inputs, separated by `and`, and give back a result. Use it like a value with `of` (or `with`):

```
to area with width and height:
    give back width times height.

say area of 3 and 4.
if area of 2 and 5 is 10, say "ten".
```

A function can leave early: `give back X if CONDITION` (or `unless`) hands X back only when the condition says so, and otherwise goes on to the next sentence. In a function that gives back nothing, `stop here if CONDITION` does the same:

```
to first word of line, giving back text:
    give back "" if line is empty.
    give back item 1 of the words of line.

to warn with amount:
    stop here unless amount is above 100.
    say "That's a lot: " followed by amount.
```

A function can call itself. Here the factorial of a number is the number times the factorial of the number below it:

```
to factorial of n:
    if n is at most 1, give back 1.
    give back n times factorial of (n minus 1).

say factorial of 5.
```

Functions are defined at the top level of the program, and you can use one before its definition. A function only sees its own inputs, the names it makes, and constants (`keep`, chapter 3), so pass in anything else it needs:

```
keep tax rate as 0.2.
to add tax to amount:
    give back amount plus amount times tax rate.
say add tax to 50.
``` Giving it the wrong number of inputs is an error that says how many it needs and where it's defined.

You don't write down what kind of value each input is: the compiler works it out from how the function uses it and what it's given, and tells you before the program runs if something doesn't fit. When it can't tell, you can say, like `to area with width (a number) and height (a number), giving back a number:`. See [chapter 9](#9-types).

### When something goes wrong inside a function

A runtime error inside a function also says how the program got there: which function it was working out (with the values it was given), which function called it and from which line, and so on back to the main program:

```
Line 4: You divided by zero.
        give back marks added together divided by count
                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
while working out "average" (label was "x", loud was no)
called from "report" on line 6 (title was "Quarterly figures for the whol...", region was "North")
called from "start" on line 8
called from line 10: start
```

Numbers, yes/no values and text are shown (long text is cut short); lists aren't. A function that calls itself many times is shown once, with a count:

```
Line 3: You divided by zero.
        if n is 0, say 1 divided by n
                       ^^^^^^^^^^^^^^
while working out "countdown" (n was 0)
called from "countdown" on line 4 (n was 1)
...it called itself 4,998 more times...
called from line 5: countdown with 4999
```

Try it: `./easyscript run examples/programs/factorial.es` and `examples/programs/fibonacci.es`

---

## 8. Files

**Status: Available.**

```
write "first line" to file "notes.txt".
append "second line" to file "notes.txt".
read file "notes.txt" and call it notes.
say notes.
```

`and call the answer notes` works too, as in `ask`.

`write X to file F` replaces the file's contents, and `append` adds to the end; both end what they write with a new line, so each one is a line of the file. Reading gives back the whole file without its last new line, so the example prints the two lines. If a file can't be read, the program stops with a message like `Line 3: I couldn't read the file "notes.txt": it doesn't exist.` Inside an expression, `contents of file "notes.txt"` gives the whole file as text.

`put "hello" in file "notes.txt"` is the same as `write ... to file`, and `add "more" to the end of file "notes.txt"` the same as `append`. To check first:

```
if file "settings.txt" exists:
    say "Found the settings.".
```

Folders, and making, renaming, copying, moving and deleting files, are in the [standard library](#18-the-standard-library).

Try it: `./easyscript run examples/programs/files.es`

---

## 9. Types

**Status: Available.**

Every value has a **kind** (its type): a number, text, yes/no, a list of one of these (see [Lists](#10-lists)), or one that can be nothing (`a number or nothing`). The compiler works out the kind of every value before the program runs, so mistakes like adding text to a number are reported with the line and a caret, the same way as a misspelled name, instead of stopping the program halfway. The program then runs faster too, because it never has to check kinds while it runs.

Most programs never mention kinds. They come from:

- **Values:** `42` is a number, `"hi"` is text, `yes` is yes/no. `ask` and reading a file give text, and `length of` gives a number.
- **Variables:** a variable takes its kind from its `let` and keeps it. `ask ... and call the answer X` and `read file ... and call it X` make text.
- **Constants:** a constant's kind is the kind of its value.
- **Operators:** arithmetic needs numbers and gives a number; comparisons give yes/no; `followed by` and `as text` give text. `and` is logical between yes/no values and joins when there's text, decided before the program runs.
- **Functions:** an input's kind comes from how the function uses it (`x divided by 2` makes `x` a number) and from what it's given when it's called. What a function gives back comes from its `give back`s. A function with no `give back` gives back nothing, so it can only be called as a sentence of its own.

```
to halve with x:
    give back x divided by 2
say halve of 4
say halve of "four"
```

```
Line 4: "halve" needs a number for "x", but this is text.
    say halve of "four"
                 ^^^^^^
"x" is a number because of line 2. An input has one kind of value everywhere; to make it clear, write it in the definition, like "to halve with x (a number):".
```

An input has one kind everywhere, so calling the same function with a number in one place and text in another is an error. Turn the value into text first with `as text` if that's what you meant.

### Saying the kind

When nothing in the program decides a kind, for example a function whose input is only printed and that's never called, the compiler asks you to say which:

```
Line 1: I can't tell what kind of value "x" is.
    to frame with x:
                  ^
Nothing in the program decides it. Say which kind in the definition, like "to frame with x (a number):".
```

Write the kind in parentheses after an input, and what the function gives back with `giving back` before the colon:

```
to area with width (a number) and height (a number), giving back a number:
    give back width times height.
to greet with who (text), giving back nothing:
    say "Hello,", who.
to both with first (yes or no) and second (yes or no):
    give back first and second.
```

The kinds are `a number`, `text`, `yes or no` (or `yes/no`), lists like `a list of numbers`, and any of these except lists and maps followed by `or nothing` ([below](#values-that-can-be-nothing)); `giving back nothing` says the function gives back no value. Kinds are optional, and you can write some and leave out others; whatever you write is checked like everything else.

**Why this wording?** It had to read like English and not look like the `as a number` / `as text` conversions, which change a value. Other choices that were considered:

| Considered | Why not |
|---|---|
| `to area with width as a number:` | Reads like the conversion `width as a number`, which means something different |
| `to area with number width:` | Puts the kind before the name, which reads awkwardly with "with" |
| `to area with width: number` | The colon already opens the function's block |
| `to area with width which is a number:` | Long, and "is" already compares values |
| `to area ... gives back a number:` / `returns a number` | "to area ... gives back" doesn't read as a sentence; `giving back` matches `give back` |

### Values that can be nothing

A kind can end in `or nothing`: `a number or nothing`, `text or nothing`, `yes or no or nothing`, and also `a dog or nothing` ([records](#fields-that-can-be-nothing)) and `a shape or nothing` ([kinds with variants](#14-kinds-with-variants)). Such a value is either a value of that kind or nothing. `the position of X in T` gives one (nothing when X isn't there), and fields, inputs and results can be one:

```
a person is a kind of thing with:
    a name, which is text.
    a nickname, which is text or nothing.

to half of n (a number or nothing), giving back a number:
    if n is nothing, give back 0.
    give back n divided by 2.
```

**It has to be checked before it's used as its kind.** Using it unchecked is an error before the program runs:

```
Line 4: I can't add a number to a number or nothing.
    say at plus 1.
        ^^^^^^^^^
It could be nothing. Check it first, like "if at is not nothing:".
```

The checks are the same as for records: `if X is not nothing:`, `while X is not nothing:`, the right side of `X is not nothing and ...`, the rest of a block after `if X is nothing, give back 0.`, and `check which kind X is:` with `when it's nothing:` and `otherwise:` (where `it` is the number). A number, text or yes/no value can go wherever one that can be nothing is kept (`set nickname to "Annie"`, `half of 10`), and `nothing` too. Printing one shows its value, or `nothing`.

A function that gives back a value and also nothing needs to say so in its definition (`giving back a number or nothing`) if it gives back the value first; giving back nothing first works it out.

### What's still checked while the program runs

Kinds can't tell you everything. These are still found while the program runs, with the line they happened on: asking for a list item or a map key that isn't there, dividing by zero, counting with a step that isn't more than zero, a number of times that isn't a whole number, text that isn't a number in `as a number`, files that can't be read or written, a function that gives back a value but reaches its end without `give back`, and functions calling each other too deeply.

Try it: `./easyscript run examples/programs/kinds.es`
---

## 10. Lists

**Status: Available.**

A list holds several values in order, like the scores of a class or the names of the players. Write one out with `a list of`, separating the items with commas and `and`:

```
let scores be a list of 72, 95 and 88.
let names be a list of "Ann" and "Bob".
say scores.
```

says `72, 95 and 88`. A list holds **one kind** of value: a list of numbers, a list of text, a list of yes/no values, or a list of lists. To start with nothing in it, say which kind it will hold: `let names be an empty list of text.` (or `an empty list of numbers`, `an empty list of yes/no values`, `an empty list of lists of numbers`).

### Items

Items are numbered from 1, as in English:

```
say item 2 of scores.        # 95
say the first item in scores.
say the last item of scores.
```

`of` and `in` mean the same here. The position is a number, a name, or something in parentheses: `item n of scores`, `item (n plus 1) of scores`. Asking for an item that isn't there stops the program, saying which item and how long the list is:

```
Line 3: There's no item 4 in the list: it has 3 items.
```

### Parts of a list

These give back a **new list** with some of the items; changing it never changes the original (lists inside it are copied too):

```
let scores be a list of 10, 20, 30, 40, 50 and 60.
say items 2 to 4 of scores.             # 20, 30 and 40
say items 5 to the end of scores.       # 50 and 60
say the first 3 items in scores.        # 10, 20 and 30
say the last 2 items in scores.         # 50 and 60
say scores without the first item.      # 20, 30, 40, 50 and 60
say scores without the last item.       # 10, 20, 30, 40 and 50
say scores without item 3.              # 10, 20, 40, 50 and 60
say every second item of scores.        # 20, 40 and 60
say every other item of scores.         # 10, 30 and 50
```

`every other item of scores` takes the first item and every second one after it (10, 30 and 50), while `every second item` starts with the second (20, 40 and 60). Ranges include both ends, counted from 1, and both positions must be in the list: `items 2 to 9` of a six-item list stops the program with "There's no item 9 in the list: it has 6 items.". `the first N items` and `the last N items` give back the whole list when it's shorter than N. `every second item` means items 2, 4, 6 and so on; any ordinal works: `every third item`, `every tenth item`, `every 12th item`.

### Asking about a list

- `the length of scores` or `the number of items in scores`: how many items it has
- `scores contains 95`, `scores does not contain 95` (or `doesn't contain`): yes or no
- `scores is empty`, `scores is not empty` (or `isn't empty`); these work for text too (`name is empty`)
- `scores is a list of 72, 95 and 88`: two lists are equal when they have equal items in the same order

### Changing a list

```
add 60 to the end of scores.
insert 7 at position 1 of scores.
remove item 2 from scores.
remove the last item from scores.
set item 1 of scores to 100.
sort scores.
```

`remove` and `set` also take `the first item` and `the last item`. `insert` can put an item anywhere from position 1 to one past the end.

**`add X to Y`** adds X to the end of Y when Y is a list and that's the only thing it can mean: `add "Bob" to names` adds a name. With a list of numbers it could also mean adding to every number, so EasyScript asks you to say which:

```
Line 2: "scores" is a list of numbers, so this could mean two things.
    add 5 to scores
    ^^^^^^^^^^^^^^^
To put it at the end of the list, write "add ... to the end of scores". To add it to every item, write "set scores to every n in scores plus ...".
```

**Sorting** puts numbers smallest first ("not a number", which only comes from sums like infinity minus infinity, goes last) and text in alphabetical order (by character codes, so capital letters come first). Add `, biggest first` (or `smallest first`) to choose the order, and sort text, or lists of lists, by their length with `sort names by their length` (`, longest first` or `shortest first`). Items that are equal stay in the order they were in. Sorting changes the list itself; `scores sorted` gives a sorted copy instead (see [New lists from old ones](#new-lists-from-old-ones)).

### Going through a list

```
for each score in scores:
    say score.
```

Inside the loop, `it` is the item too. `stop the loop` and `skip this one` work as in every loop. The loop goes through the items the list had when it started, so adding to or removing from the list inside the loop doesn't change which items it visits.

When the list's name is the plural of the item's, `for each score:` is enough: it goes through `scores` (the plural adds `s`, or `es` after `s`, `x`, `z`, `ch` and `sh`, and `y` after a consonant becomes `ies`). If there's no list with that name, it's an error that asks which list you mean.

### New lists from old ones

`each NAME in L` (or `every NAME in L`) works out the rest of the value for each item and gives the results as a new list:

```
say each score in scores plus 5.           # 77, 100 and 93
say each name in names followed by "!".
```

`the scores that are ...` keeps the items for which a comparison is yes. The comparisons are the usual ones with `are` instead of `is` (`are at least`, `are less than`, `are not`, `are empty`, ...). `whose` does the same with a field of each record, and `where` takes any condition about `it`, the item:

```
let passing be the scores that are at least 70.
let older be the dogs whose age is over 3.
let low be the scores where it is below 50 or it is 100.
```

A filter goes after a single value, so the count of a filtered list needs parentheses: `the number of items in (the dogs whose age is over 3)`. (`only the scores that are at least 70` is the older form, and still works.)

And to sum up a list:

- `scores added together`: the total of a list of numbers
- `the biggest of scores`, `the smallest of scores`: for numbers or text
- `scores reversed`: a new list, last item first
- `scores sorted`: a new list, sorted like `sort scores` would (`scores sorted biggest first`, `names sorted by their length`, `names sorted by their length longest first`), leaving `scores` as it was
- `morning slots followed by evening slots`: a new list with the first list's items, then the second's. Both must be lists of the same kind; with anything else, `followed by` joins text, as always (`"Slots: " followed by morning slots`).

### Sharing and copying

A list is **shared**: `let same be scores` gives the same list a second name, so a change through either name shows through both. A function you give a list to gets that same list, so it can change it. To get a separate list, make a copy:

```
let backup be a copy of scores.
add 1 to the end of scores.        # backup doesn't change
```

`a copy of` copies the lists inside a list too, so nothing in the copy is shared with the original.

### Lists of lists

A list can hold lists. Write each inner list in parentheses, because its own commas and `and`s would otherwise be ambiguous:

```
let grid be a list of (a list of 1 and 2) and (a list of 3).
say grid.                          # (1 and 2) and (3)
add 5 to the end of item 2 of grid.
say item 1 of item 1 of grid.      # 1
```

### Lists and kinds

A list's kind is worked out like any other ([Types](#9-types)): a list of numbers can't hold text, and the compiler says so before the program runs:

```
Line 5: "scores" is a list of numbers, so it can't hold text.
    add "ninety" to the end of scores
        ^^^^^^^^
A list holds one kind of value. To keep text too, make a separate list for it.
```

Write list kinds in definitions as `(a list of numbers)`, `(a list of text)`, `(a list of yes/no values)` or `(a list of lists of numbers)`:

```
to average of marks (a list of numbers), giving back a number:
    give back marks added together divided by the length of marks.
```

### Things to watch

- **The items of `a list of` are single values up to the next comma or `and`**, so `a list of 1, 2 plus 3` has two items, 1 and 5. To use `and` inside an item, or to write anything after a list, put it in parentheses: `every name in (a list of "a" and "b") followed by "!"`.
- **`every` takes the rest of the value**, so it has to start it; inside a longer value, put it in parentheses: `say "Curved: " followed by (every score in scores plus 5)`.
- A list kept with `keep` is fixed: `keep weekdays as a list of "Mon", "Tue" and "Wed".` Its items are fixed values (not lists), and adding, removing, setting, inserting or sorting is an error that suggests `let`. `let mine be weekdays` gives `mine` its own copy, which can change.

### What's checked while the program runs

Asking for an item that isn't there (or item 0, or item 1.5), a range whose positions aren't in the list or go backwards, a negative or fractional number of items, inserting past the end, and asking for the first, last, biggest or smallest item of an empty list (or taking it away with `without`) stop the program with the line and what went wrong. Everything else about lists, including every kind mistake, is checked before the program runs.

Try it: `./easyscript run examples/programs/grade_statistics.es` and `./easyscript run examples/programs/top_scores.es`

---

## 11. Text

**Status: Available.**

Text is a row of **characters**. A character is one Unicode character, whatever language it's from, so `"café"` has 4 characters and `"日本"` has 2. (Inside the program each is stored as UTF-8, one to four bytes; you never see that, and text that's plain English is handled byte by byte, as fast as it can be.) Text never changes: everything in this chapter makes new text, or a new list, and leaves the text it started from as it was.

### Characters and pieces

Characters are numbered from 1, and you take them with exactly the words you use for the items of a list, with `character` for `item`:

```
let word be "café au lait".
say character 4 of word.                  # é
say the first character of word.          # c
say the last character in word.           # t
say characters 2 to 4 of word.            # afé
say characters 6 to the end of word.      # au lait
say the first 4 characters of word.       # café
say the last 4 characters of word.        # lait
say word without the first character.     # afé au lait
say word without character 5.             # caféau lait
say every other character of "abcdefg".   # aceg
say every second character of "abcdefg".  # bdf
say the characters of "café".             # c, a, f and é
say the number of characters in word.     # 12, like the length of word
```

The rules are the lists' rules too: positions are numbers, names, `it`, or something in parentheses (`character (n plus 1) of word`); asking for a character that isn't there stops the program with the length ("There's no character 13 in the text: it has 12 characters."); `the first 20 characters` of shorter text is all of it. `the characters of word` is a list of text, so it works with `for each`:

```
for each letter in the characters of "naïve":
    say letter.
```

### Searching

```
let word be "unbreakable".
say word contains "break".            # yes
say word starts with "un".            # yes
say word ends with "ing".             # no
say word doesn't start with "re".     # yes
say the position of "break" in word.  # 3
say the position of "x" in word.      # nothing
```

`contains`, `starts with` and `ends with` (and their `does not` / `doesn't` forms) are comparisons, so they combine with `and`, `or` and `not`. **`the position of` is `a number or nothing`:** nothing when the text isn't there. Check it before using it as a number (see [values that can be nothing](#values-that-can-be-nothing)):

```
let at be the position of "@" in address.
if at is nothing, say "That's not an email address."
if at is not nothing, say "The name ends at " followed by (at minus 1).
```

It works for lists too: `the position of 95 in scores` is the position of the first item equal to 95, or nothing.

### Splitting and joining

```
say the words of "  the quick  brown fox ".       # the, quick, brown and fox
say the lines of "one\ntwo\n".                   # one and two
say "2024-10-10" split by "-".                    # 2024, 10 and 10
let names be a list of "Ada", "Grace" and "Linus".
say names joined with ", ".                       # Ada, Grace, Linus
```

`the words of` splits at spaces, tabs and line breaks, however many there are; `the lines of` splits at line breaks (a line break at the very end doesn't start another line). `split by` keeps empty pieces, so `"a,,b" split by ","` has three items, the middle one empty. `joined with` puts its text between the items of a list of text, numbers or yes/no values, and is the opposite of `split by`: `(text split by ",") joined with ","` gives back `text`.

### Changing text

```
say "Hello, World" in uppercase.           # HELLO, WORLD
say "Hello, World" in lowercase.           # hello, world
say "  padded  " without extra spaces.     # padded
say "banana" with "a" replaced by "o".     # bonono
```

`without extra spaces` takes away the spaces, tabs and line breaks at the start and the end, and leaves the ones in the middle. `with X replaced by Y` replaces every X, from the left. Letters change case in English, in the accented Latin letters (`é` becomes `É`), in Greek and in Cyrillic; other characters stay as they are.

### Checking characters

```
for each letter in the characters of "R2-D2":
    if letter is a letter:
        say letter followed by " is a letter".
    otherwise if letter is a digit:
        say letter followed by " is a digit".
say " " is a space.                  # yes
say the code of "A".                 # 65
say the character with code 233.     # é
```

`is a letter`, `is a digit` and `is a space` are yes only for text of exactly one such character (so `"ab" is a letter` is no). **They need the `a`:** `letter is letter`, without it, compares with a variable called `letter`, so you can still use `letter`, `digit` and `space` as names. The code of a character is its Unicode number; `the character with code N` goes the other way.

### Building text

Adding to the end of text in a loop is the usual way to build it:

```
let line be "".
repeat 5 times:
    set line to line followed by it followed by " ".
say line.                                  # 1 2 3 4 5
```

`set line to line followed by ...` is fast however long the text gets: when nothing else holds the text, it grows where it is instead of being copied each time, so a million pieces take a fraction of a second. (Anything else that held the old text still sees the old text.)

### Things to watch

- **The forms that take one value take only one:** `the words of`, `the code of`, `the position of X in T`, and the values after `split by`, `joined with`, `with` and `replaced by` are single values, like `length of`. `the number of items in text split by ","` means `(the number of items in text) split by ","`, which is an error; write `the number of items in (text split by ",")`.
- **After a function's name, `with` starts its inputs.** `word with "a" replaced by "b"` replaces text when `word` is a variable; if `word` were a function, it would be called with `"a"`.
- **Text constants stay as written.** A constant (`keep`) can't take text apart or change it ("A constant can't take text apart or look inside it."); use `let`.

### What's checked while the program runs

A character that isn't there (or character 0, or 1.5), a range of characters that goes backwards, a negative or fractional number of characters, `without the first character` of empty text, splitting by empty text, replacing empty text, the code of anything but one character, a character code that isn't one, and `as a number` on text that isn't a number stop the program with the line and what went wrong. Everything else, including every kind mistake (`the words of 42`), is checked before the program runs.

Try it: `./easyscript run examples/programs/word_counter.es`

## 12. Records

**Available.** A record keeps several named values together: a dog has a name and an age, an order has a customer and a total. You define a **kind of record** once, then make as many records of that kind as you like.

```
a dog has a name (text) and an age (a number, starting at 0).

let rex be a new dog with name "Rex" and age 3.
say rex.
say rex's name.
```

```
a dog with name "Rex" and age 3
Rex
```

### Defining a kind of record

A definition is one sentence at the start of a line (never inside an `if`, a loop or a function): `NAME has` and its **fields**, separated by commas and `and`, each followed by its kind in parentheses: `a dog has a name (text) and an age (a number).` The kind is any kind of value: `a number`, `text`, `yes or no`, a list like `a list of numbers`, or another kind of record. A long list of fields can go on over several lines (end a line with a comma or `and`). Names of several words work for kinds and fields alike: `a tree node`, `a start time`. `some` before a field's name is left out like `a`: `an order has a customer and some drinks.`

The kinds can be left out: `a cat has a name and a favorite toy.` Then each field's kind is worked out from how the program uses it (`a new cat with name "Tom"` makes `name` text), and it's the same everywhere. If nothing decides it, that's an error asking you to write it, like `favorite toy (text)`.

The older form does the same, with one field per line: `a dog is a kind of thing with:` and, indented below, `a name, which is text.` and `an age, which is a number, starting at 0.`

A field can have a **starting value** (`an age (a number, starting at 0)`, or `an age (starting at 0)`, whose kind is the value's). It's fixed before the program runs, like a constant: a number, text, yes or no, a constant, a fixed list, or an empty list (each record gets its own copy of a list).

### Making records

`a new dog with name "Rex" and age 3` makes a dog. Give every field a value unless it has a starting value or can be nothing; a missing one is an error before the program runs:

```
Line 10: A new dog needs a value for "age".
    let pup be a new dog with name "Pup".
               ^^^^^^^^^^^^^^^^^^^^^^^^^
```

Fields can come in any order, separated by `and` or commas. A `new` inside another one needs parentheses, so it's clear which record the fields after it belong to: `a new person with name "Ann" and pet (a new dog with name "Rex" and age 3)`.

### Fields

`rex's age` is a field. It works anywhere a value does, and chains: `the order's customer's name`. `the age of rex` means the same, and needs no parentheses: `the name of item 1 of dogs`, `the name of the owner of rex`. (If a function has the same name as a field, `the size of rex` could mean either, so it's an error that asks you to write `rex's size` or `size with rex`.) Inside a `for each` loop, `it's name` is the item's field, and `(item 1 of dogs)'s name` takes a field of a value in parentheses. `'s` binds tightest of all, so `rex's age plus 1` is `(rex's age) plus 1`.

Change fields with the same sentences as variables:

```
set rex's age to 4.
set the age of rex to 4.
add 1 to rex's age.
increase rex's age by 2.
add "ball" to the end of rex's toys.
```

### Records are shared

Like lists, a record is **shared**: `let same dog be rex` gives the same record another name, and a function given a record can change it. `is` asks whether two values are **the same record**:

```
let same dog be rex.
set same dog's name to "Rexy".
say rex's name.                    # Rexy
if same dog is rex, say "same".    # same
```

`a copy of rex` makes a new record with the same field values. The copy is **shallow**: text, lists and records inside it are shared with the original. Two records with the same fields are still two records, so `is` says no for a copy. To compare what's inside, compare the fields, or write a method:

```
to matches, for a dog, with other (a dog), giving back yes or no:
    give back the dog's name is other's name and the dog's age is other's age
```

### Fields that can be nothing

A field whose kind ends in `or nothing` may hold nothing. That's how a linked list ends, or a tree has no child:

```
a node is a kind of thing with:
    a value, which is a number.
    a next, which is a node or nothing.
```

Such a field starts as nothing, and `set first's next to nothing` empties it again. Before using it as a record, **check it**. Using it unchecked is an error before the program runs:

```
Line 13: "first's next" could be nothing, so check it first.
    say first's next's value.
        ^^^^^^^^^^^^
Use it inside "if first's next is not nothing:".
```

Inside the check, it's a node:

```
if first's next is not nothing:
    say first's next's value.

let current be first.
while current is not nothing:
    say current's value.
    set current to current's next.
```

These count as checks: `if X is not nothing:` (and the `otherwise` of `if X is nothing:`), `while X is not nothing:`, `repeat until X is nothing:`, the right side of `X is not nothing and ...` or `X is nothing or ...`, and the rest of a block after `if X is nothing, give back 0.` (an `if` that always leaves). A check lasts until the block ends or the program sets X. While the program runs it's checked once more, so if another name for the same record set it to nothing in between, the program stops with "This was checked not to be nothing, but something has set it to nothing since then." instead of crashing.

Functions can take and give back records that may be nothing. Their kinds are worked out like any other, or written down: `(a node or nothing)`, `giving back a node or nothing`, and `give back nothing`.

```
to sum of start:
    if start is nothing, give back 0.
    give back start's value plus (sum of start's next)
```

Numbers, text and yes/no values can be nothing too: see [values that can be nothing](#values-that-can-be-nothing). A list or map can't; an empty one says there's nothing in it.

### Methods

A **method** is something a kind of record can do: `a dog can bark:`. Inside it, the record is called like its kind (`the dog`). The method's name is every word after `can` (the `a` in `have a birthday` is ignored), and more inputs come after `with`, as in any function:

```
a dog can bark:
    say the dog's name, "says woof".

a dog can have a birthday:
    add 1 to the dog's age.

a dog can rename with new name:
    set the dog's name to new name.

bark rex.
have a birthday for rex.
rename rex with "Max".
say (describe rex) followed by "!".
```

A method is called with the record straight after its name (`bark rex`) or after `for`, as a sentence or inside a value. The older form `to bark, for a dog:` means the same as `a dog can bark:`; `can` matches how interfaces are written (`something that can speak`).

**Different kinds of record can have methods with the same name.** The call picks the one for the kind of its record:

```
to speak, for a dog:
    say the dog's name followed by " says woof".

to speak, for a cat:
    say the cat's name followed by " says meow".

speak rex.     # Rex says woof
speak tom.     # Tom says meow
```

The record's kind can be worked out from anywhere in the program, as usual (`to introduce with pet:` calling `speak pet` works once `introduce` is given a dog). A kind of record can have only one method with each name, a plain function can't share a name with anything, and calling `speak` on a kind that has no `speak` is an error that lists the kinds that do. When nothing decides which kind the record is, the error asks you to say it, like `(a dog)` after an input.

### Records in lists, and lists in records

A list can hold records (`a list of dogs`), and a field can hold a list (`the toys, which is a list of text, starting at an empty list of text.`). Records in a list are shared, never copied, even by `a copy of` the list. `contains` and `the position of` look for the same record. Sorting and `biggest of` don't work on records.

### Memory

A record is freed as soon as nothing holds it, with what its fields hold. Records that point to each other in a circle (two people who are each other's friend) keep each other alive, so they're never freed while the program runs. The memory check (`ES_DEBUG_MEMORY=1`) reports them at the end, following lists and maps too. Before the program ends, break the circle by setting one of its fields to nothing, or by taking a record out of the list or map that holds it. See [memory](memory.md#cycles).

### Things to watch

- **A word ending in `'s` is always a possessive,** so a name can't end in `'s`.
- **`every d in dogs's age` takes the field of the list,** not of each dog. Use a `for each` loop to go through fields.
- **A field can't always hold its own kind:** `a next, which is a node.` would need a node before any node could be made. Make it `a node or nothing`.
- **A function that gives back a record first and nothing later** is worked out as giving back a record. Write `giving back a node or nothing` (or give back nothing first).

### What's checked while the program runs

Only a record that was checked not to be nothing, and then set to nothing through another name before it was used. Every other mistake (a missing field, a wrong kind, a field that could be nothing) is caught before the program runs.

Try it: `./easyscript run examples/programs/linked_list.es` and `./easyscript run examples/programs/binary_tree.es`

---

## 13. Maps

**Status: Available.**

A map is a lookup table: it finds a value by its **key**, like a person's age by their name or a price by the name of a drink. Write one out with `a map of`, each key followed by `to` and its value:

```
let prices be a map of "coffee" to 3, "tea" to 2 and "milk" to 1.
say prices.
say the value for "tea" in prices.
```

```
"coffee" is 3, "tea" is 2 and "milk" is 1
2
```

Keys are **text or numbers**, all of one kind; values can be any kind, all of one kind too. To start with nothing in it, say which kinds it will hold: `let ages be an empty map from text to numbers.` (or `an empty map from numbers to text`, `an empty map from text to lists of text`, ...). A map keeps its keys **in the order they were first added**, so printing it, going through it, or taking its keys always gives the same order.

### Looking up and setting

```
let ages be an empty map from text to numbers.
set the value for "Paris" in ages to 19.
set the value for "Junie" in ages to 24.
say the value for "Paris" in ages.          # 19
set the value for "Paris" in ages to 20.    # changes it, in the same place
```

Setting a key that isn't there adds it; setting one that's there changes its value. Asking for a key that isn't there stops the program, naming the key:

```
Line 3: There's no value for "Bob" in the map.
    say the value for "Bob" in ages
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^
To check first, use "has". To use another value when it's missing, add ", or" and that value after it.
```

To use another value when the key is missing, add `, or` and that value. It's only worked out when it's needed:

```
say the value for "Bob" in ages, or 0.      # 0
set the value for word in counts to (the value for word in counts, or 0) plus 1.
```

The value after `, or` is a single value: `the value for "Bob" in ages, or 0 plus 1` could mean adding 1 to the whole thing or only to the 0, so it's an error that shows both ways with parentheses.

Number keys that differ only by rounding errors (they're the same to 12 significant digits) are the same key: `0.1 plus 0.2` finds the value kept for `0.3`.

### Asking about a map

- `ages has "Paris"`, `ages does not have "Bob"` (or `doesn't have`): yes or no
- `the number of entries in ages`: how many keys it has
- `ages is empty`, `ages is not empty` (or `isn't empty`)
- `the keys of ages`, `the values of ages`: new lists, in the map's order
- `ages is other ages`: two maps are equal when they have the same keys with equal values, in any order

### Changing a map

```
add 1 to the value for "Paris" in ages.
increase the value for "Junie" in ages by 2.
remove "Paris" from ages.
```

`add`, `subtract`, `increase`, `decrease`, `multiply` and `divide` change a value in place; the key has to be there already (otherwise the program stops: "There's no value for "Bob" in the map."). `remove K from M` takes a key out; it has to be there too ("I can't remove "Bob": the map has no value for it."). Removing a key and adding it again puts it at the end.

`ages without "Paris"` is the value form: a new map with every key but that one, leaving `ages` as it was (lists and maps inside are copied, like `a copy of`). The key has to be there ("I can't leave out "Bob": the map has no value for it.").

### Going through a map

```
for each name and age in ages:
    say name followed by " is " followed by age.
```

The loop names each key and its value, in the map's order. Inside it, `it` is the key. Like a list's loop, it goes through the entries the map had when it started, so changing the map inside the loop doesn't change what it visits. A map always needs both names; to go through only the keys, write `for each name in the keys of ages:`.

### Sharing and copying

A map is **shared**, like a list: `let same be ages` gives the same map a second name, and a function you give a map to gets that same map, so it can change it. `a copy of ages` makes a separate map. Lists and maps inside a map belong to it, so the copy gets copies of them too; text and records inside are shared.

### Maps of lists, lists of maps

A map's values can be lists, maps or records, and lists and records can hold maps:

```
let graph be an empty map from text to lists of text.
set the value for "Ash" in graph to a list of "Birch" and "Cedar".
add "Elm" to the end of the value for "Ash" in graph.

let rows be a list of (a map of "a" to 1) and (a map of "b" to 2).

a student is a kind of thing with:
    a name, which is text.
    a scores, which is a map from text to numbers, starting at an empty map from text to numbers.
```

A list or map written inside `a map of` needs parentheses, because its own commas and `and`s would otherwise be ambiguous: `a map of "odd" to (a list of 1 and 3)`. `the value for K in M` gives the value itself (shared), so changing a list you got from a map changes the one in the map.

### Maps and kinds

A map's kind is "a map from KEYS to VALUES", worked out like any other ([Types](#9-types)): a key or value of the wrong kind is an error before the program runs:

```
Line 4: The keys of "ages" are text, but this is a number.
    say the value for 3 in ages
                      ^
```

Write map kinds in definitions as `(a map from text to numbers)`, `(a map from numbers to lists of text)`, and in lists and maps in the plural: `a list of maps from text to numbers`, `a map from text to maps from text to numbers`.

### Fixed maps

A map kept with `keep` is fixed, like a fixed list:

```
keep prices as a map of "coffee" to 3, "tea" to 2.
say the value for "tea" in prices, or 0.
```

Its keys are fixed text or numbers and its values fixed numbers, text or yes/no values (not lists or maps). Setting, changing or removing a value is an error that suggests `let`, and `the number of entries in prices` is worked out by the compiler too. `let mine be prices` gives `mine` its own copy, which can change.

### Things to watch

- **`the value for K in M` reads the map as a single value**, so `the value for "a" in ages plus 1` is `(the value for "a" in ages) plus 1`. A name straight before `in` is the key, even `item`: `the value for item in prices`.
- **`remove item from M`** reads as a list's item with its position missing. Name the key something else, or put it in parentheses: `remove (item) from prices`.
- **`, or` only goes after a value being read**: `set the value for "x" in ages, or 0 to 5` is an error.
- **Keys given twice in `a map of`** are an error when they're written out ("The key "tea" is in this map twice."); a key worked out while the program runs that's already there keeps its first place and its last value.

### What's checked while the program runs

Asking for, changing or removing a key that isn't there. Every kind mistake is caught before the program runs.

Try it: `./easyscript run examples/programs/word_counter.es` and `./easyscript run examples/programs/graph.es`

---

## 14. Kinds with variants

**Available.** Some values are one of several things: a shape is a circle, a rectangle or a point, and each has different fields. A **kind with variants** says so, and `check which kind` tells them apart:

```
a shape is one of:
    a circle with a radius (a number).
    a rectangle with a width (a number) and a height (a number).
    a point.

to area of s:
    check which kind s is:
        when it's a circle:
            give back 3.14159 times its radius times its radius.
        when it's a rectangle:
            give back its width times its height.
        when it's a point:
            give back 0.

say area of (a new rectangle with width 3 and height 4).
```

```
12
```

### Defining a kind

A definition is `NAME is one of:` at the start of a line (never inside an `if`, a loop or a function), with its **variants** indented below, one per line. A variant is a name, then `with` and its fields: each a name with its kind in parentheses, separated by `and` or commas. A variant can have no fields at all (`a point.`). Fields have no starting values.

Kinds can be **recursive**: a field can hold the kind it's in, as long as some variant doesn't need one, so a value can be made at all:

```
an expression is one of:
    a number value with an amount (a number).
    a sum with a left (an expression) and a right (an expression).
```

Every kind, record kind and variant needs its own name, because `a new circle` has to say which kind it makes.

### Making values

A value is made like a record: `a new circle with radius 3`, `a new point`. Its kind is the kind the variant is in (`a shape`), so a list can hold circles and rectangles together: `a list of (a new circle with radius 1) and (a new point)`, or `an empty list of shapes`.

A value **never changes** once it's made: `set s's radius to 4` is an error. Make a new one instead: `set s to a new circle with radius 4`.

### Telling variants apart

`check which kind S is:` has a `when` for each variant, and an optional `otherwise` at the end. Inside a `when`, `it` is the value, and `its radius` (or `it's radius`) is its field:

```
check which kind s is:
    when it's a circle:
        say "radius " followed by its radius.
    when it's a rectangle or a square:
        say "it has corners".
    otherwise:
        say "something else".
```

`when it's a rectangle or a square:` covers several variants; `when it is a circle:` works too. If S is a name (or a field, like `the drawing's shape`), it's checked in the `when` too, so `s's radius` works there as well.

A `when` can also test the value: `when it's a circle whose radius is over 10:` (or `where ...`, or `that ...`) runs only for those circles. It doesn't cover every circle, so a plain `when it's a circle:` (or an `otherwise`) is still needed after it:

```
check which kind s is:
    when it's a circle whose radius is over 10:
        say "a big circle".
    when it's a circle:
        say "a small circle".
    otherwise:
        say "not a circle".
```

Every variant needs a `when`, unless there's an `otherwise`. A missing one is an error before the program runs:

```
Line 11: This doesn't say what to do when s is a rectangle or a point.
    check which kind s is:
    ^^^^^^^^^^^^^^^^^^^^^^
Add "when it's a rectangle:" and the others, or an "otherwise:" at the end.
```

An `otherwise` after a `when` for every variant can never run. That's a **warning**: the program still runs, and the warning is shown before it.

```
Line 12 (a warning): This "otherwise" can never run: the "when"s above it cover every kind s can be.
        otherwise:
        ^^^^^^^^^^
Take it out, or the sentences in it will never be used.
```

`stop the loop` and `skip this one` inside a `when` work on the loop around the `check which kind`.

### Checking one variant

`s is a circle` (and `s is not a circle`, `s isn't a circle`) is yes or no. The `a` (or `an`) is needed, as in `is a letter`: `s is circle` compares s with a name called `circle`.

A field can only be used where the program has **checked** which variant the value is. Using one without checking is an error before the program runs:

```
Line 10: "s" might not be a circle, so check which kind it is first.
    say s's radius.
        ^^^^^^^^^^
Use it inside "if s is a circle:", or inside "when it's a circle:" after "check which kind s is:".
```

These count as checks, exactly like the checks for nothing in [chapter 12](#fields-that-can-be-nothing): `if s is a circle:` (and the `otherwise` of `if s is not a circle:`), `while s is a circle:`, the right side of `s is a circle and ...`, the rest of a block after `if s isn't a circle, give back 0.`, a `when` of `check which kind`, and right after `let s be a new circle ...` (until s is set again). While the program runs the variant is checked once more, so if a field was set to another variant through another name for its record in between, the program stops with "This was checked to be a circle, but something has set it to a point since then." instead of crashing.

### Values made of fields

A kind with just one variant is written in one sentence: `a point is made of an x (a number) and a y (a number).` Its values are made with `a new point with x 1 and y 2`, and since there's no other variant, their fields need no check (`start's x`, `the y of start`). Like every kind's values they never change and are equal when their fields are, so a list of points can be searched by contents:

```
a point is made of an x (a number) and a y (a number).
let visited be a list of (a new point with x 1 and y 2).
say visited contains (a new point with x 1 and y 2).     # yes
```

A record (`has`) is the other way round: two dogs with the same fields are still two different dogs, and a dog can change.

### Equality and copies

Two values are **equal** when they're the same variant with equal fields, so `s is a new point` and `shapes contains (a new circle with radius 1)` work as you'd expect. (Records inside are compared as records: the same record.)

Values are shared like text: since they never change, sharing one can't be seen, except through a list or map in a field, which can change. `a copy of s` gets its own copies of those.

### Kinds everywhere

A kind with variants works wherever any other kind does: in lists, maps and record fields, as a function's inputs and result (`(a shape)`, `giving back an expression`), worked out by the compiler like any other kind. A field or input can be `a shape or nothing`, and `check which kind` can have a `when it's nothing:` for it.

Methods work too, and can share a name with methods for record kinds:

```
to describe, for a shape:
    check which kind the shape is:
        when it's a circle:
            say "a circle of radius " followed by its radius.
        otherwise:
            say "another shape".
```

A value prints as its variant and fields: `a circle with radius 3`, `a point`.

### Memory

A value is freed as soon as nothing holds it, like a record. A value can't change, so it can only end up holding itself through a list or map in it (a tree whose list of children holds the tree). That circle is never freed, and the memory check reports it at the end, like records in a circle.

### Things to watch

- **Variant names are global:** two kinds can't both have a `circle`.
- **`its` is only special before a field's name:** `its radius` is the field of `it`, but a name called `its` still works elsewhere.
- **Fields are read-only.** Make a new value to change one.

### What's checked while the program runs

Only a value reached through a record's field, checked to be a variant, and then set to another one through another name for the record before it was used. Every other mistake (a missing `when`, an unchecked field, a field a variant doesn't have) is caught before the program runs.

Try it: `./easyscript run examples/programs/shapes.es`, `./easyscript run examples/programs/calculator.es` and `./easyscript run examples/programs/lambda.es`

## 15. Functions as values

**Available.** A function is a value too: it can be kept in a name, a list, a map or a record's field, given to another function, and given back by one.

```
to double with x:
    give back x times 2.

to apply twice with f and start:
    give back call f with (call f with start).

say apply twice with the function double and 5.
say apply twice with (a function taking n that gives back n plus 3) and 1.
```

```
20
7
```

### Functions as values

`the function double` is the function defined with `to double ...`, as a value: it isn't called. (Without `the function`, `double` alone calls a function that takes no inputs.)

A function can also be made right where it's needed. On one line:

```
let shout be a function taking word that gives back word followed by "!".
```

Or with a block, like a `to` function. The colon ends the sentence it's in:

```
let introduce be a function taking name (text) and age:
    say name followed by " is " followed by age.
```

`a function that gives back 42` and `a function:` take no inputs. As with `to`, the kinds of the inputs and the result are worked out from how the function is used; when nothing decides one, write it after the input (`a function taking x (a number) ...`), or after the inputs: `a function taking x, giving back a number:`.

### Calling one

`call F with X and Y` calls the function kept in F, as a sentence or as a value (`of` and `using` work like `with`). F can be a name or any single value:

```
call introduce with "Ann" and 7.
say call shout with "hello".
call the value for "click" in handlers with "OK button".
call item 1 of steps with 3.
```

When F is a name, `call` can be left out in a value: `say shout with "hello"`. A function with no inputs is called with `call F` alone (`let n be call next`), because `next` on its own is the function itself.

### Kinds of functions

A function's kind says what it takes and what it gives back: `a function taking a number and giving back text`, `a function taking text` (it gives back nothing), `a function giving back a number` (no inputs), or `a function`. They're written like any other kind:

```
to apply with f (a function taking a number and giving back a number) and x (a number):
    give back call f with x.

let handlers be an empty map from text to functions taking text.

a button is a kind of thing with:
    label, which is text.
    action, which is a function taking text and giving back text.
```

An input that is a function itself goes in parentheses: `a function taking (a function taking a number) and text`. Calling a function with the wrong number of inputs, or the wrong kinds, is an error before the program runs:

```
Line 6: "f" takes 1 value, but this gives it 2.
    say call f with 1 and 2.
        ^^^^^^^^^^^^^^^^^^^
It's a function taking a number and giving back a number.
```

### Names from around it

A function made inside a value can use the names made before it, in the main program or in the function around it, and change them. They're **shared, not copied**: the function and the code around it see the same name, even after the function around it has ended. This makes a counter:

```
to make counter, giving back a function giving back a number:
    let count be 0.
    give back a function giving back a number:
        add 1 to count.
        give back count.

let next be call make counter.
let other be call make counter.
say call next.
say call next.
say call other.
```

```
1
2
1
```

Each call of `make counter` makes a new `count`, so `next` and `other` count separately. In the same way, each round of a loop makes its own names (`let`, the loop's number, a `for each` item), so a function made in each round keeps that round's:

```
let greetings be an empty list of functions giving back text.
for each name in a list of "Ann", "Bo" and "Cy":
    add (a function that gives back "Hello, " followed by name) to the end of greetings.
```

Inside such a function, `it` doesn't mean the loop's number or item (the function runs later, when it's called): use the loop's name. `stop the loop` doesn't leave a loop around it either. A function can't be made inside `every` or `only`; make it first and use its name there.

A check that a name isn't nothing doesn't last if a function made earlier can set the name whenever it's called, so that's an error:

```
Line 19: "maybe" was checked, but the function made on line 16 can set "maybe" whenever it's called, so the check doesn't last.
        say maybe plus 1.
            ^^^^^
Keep it in a name of its own first, like "let current maybe be maybe", and check that instead.
```

### Built-ins that take a function

Four built-ins go through a list with a function. Each has a value form, which makes something new, and (except combining) a sentence that changes the list itself:

```
to double with x:
    give back x times 2.
to sum of so far and x:
    give back so far plus x.
to even with n:
    give back n mod 2 is 0.

let scores be a list of 40, 75, 12, 90 and 51.
say double applied to every number in scores.
say the numbers in scores where it is more than 50.
say the numbers in scores that pass even.
say scores combined using sum, starting from 0.
```

```
80, 150, 24, 180 and 102
75, 90 and 51
40, 12 and 90
268
```

- **`F applied to every X in L`** gives a new list of F of each item; **`apply F to every X in L.`** replaces each item of L with it instead (so F has to give back the kind L holds).
- **`the Xs in L where C`** keeps the items for which C is yes; inside C, `it` is the item. **`the Xs in L that pass F`** keeps the items F gives back yes for. **`keep only the Xs in L where C.`** (or `that pass F`) takes the others out of L.
- **`L combined using F, starting from S`** gives F the start and the first item, then what that gave back and the second item, and so on: `scores combined using sum, starting from 0` adds them up. Without `starting from`, it starts with the first item, and an empty list stops the program with "There's nothing to combine: the list is empty." `the numbers in scores combined using sum` reads well too.
- **`L sorted by F`** orders the items by what F gives back for each (numbers or text), smallest first or `biggest first`; items with equal keys keep their order. **`sort L by F, biggest first.`** sorts L itself.

The words naming the items (`number`, `numbers`) are there to read well; any word works. A function's name means the function itself here, not a call of it, and any function value works, including one made right there:

```
let bonus be 5.
say (a function taking s that gives back s plus bonus) applied to every score in scores.
sort names by (a function taking name that gives back the number of characters in name), biggest first.
```

The condition after `where` (and a function made after `using`) runs to the end of the value, so put the whole thing in parentheses to use it inside a longer one: `(the numbers in scores where it is above 50) combined using sum, starting from 0`.

These are as fast as a loop you write yourself: a `to` function is called directly, with no closure in between (see [performance](performance.md)).

### Equality and printing

Two function values are equal (`is`) only when they're the same one, like records. A function prints as its name: `the function double`, or `the function made on line 9` for one made inside a value.

### Memory

Functions are freed when nothing holds them, like records. A function that's kept in a name it uses (directly, or in a list, record or map that name holds) keeps itself alive, and the memory check reports it at the end of the program. A `to` function never does, so use one when a function needs to call itself.

### What's checked while the program runs

Nothing new: every kind mistake with function values is caught before the program runs. A runtime error inside a function value shows it in the chain of calls, like any function: `while working out "the function made on line 3" (n was 0)`.

Try it: `./easyscript run examples/programs/counter.es`, `./easyscript run examples/programs/event_handlers.es` and `./easyscript run examples/programs/pipeline.es`

## 16. Interfaces

**Available.** An interface says what a value has to be able to do. Any kind of record, or kind with variants, that can do it fits, without saying so:

```
something that can speak is anything with:
    a way to speak, giving back text.

a dog is a kind of thing with:
    name, which is text.
a cat is a kind of thing with:
    name, which is text.

to speak, for a dog, giving back text:
    give back "Woof!".
to speak, for a cat, giving back text:
    give back "Meow.".

let zoo be an empty list of things that can speak.
add (a new dog with name "Rex") to the end of zoo.
add (a new cat with name "Tom") to the end of zoo.
for each animal in zoo:
    say speak animal.
```

```
Woof!
Meow.
```

### Defining one

`NAME is anything with:` at the start of a line, with its **ways** indented below, one per line. The name is every word before `is anything with`, so it can read as a phrase (`something that can speak`) or a noun (`a speaker`). A way is written like the first line of a method, without the kind of record: `a way to speak.`, `a way to greet with name (text), giving back text.` Each input's kind has to be written, because that's what the methods are compared with.

### What fits

A kind of record (or with variants) fits when it has a method for every way, with the same name, the same kinds of input, and the same result. A way that gives back an interface can be a method that gives back any kind that fits it. Nothing else is needed: the dog above never says it can speak. Giving one that doesn't fit is an error before the program runs:

```
Line 22: A fish can't be used as something that can speak: it has no way to speak.
    add (a new fish with name "Nemo") to the end of zoo.
        ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Give it one, like "to speak, for a fish:".
```

### Using one

An interface is a kind like any other: `(something that can speak)` for an input, `giving back a speaker`, `an empty list of things that can speak` (the plural of a name that starts with `something` or `anything` is `things ...`), `a map from text to speakers`, `(a speaker or nothing)`. A dog, a cat or a bird goes wherever one is kept.

Its ways are called like methods: `speak animal`, `greet animal with "Ann"`. Which method runs depends on the kind of the value while the program runs. A value of an interface has no fields (`animal's name` is an error), and only its ways can be called on it.

A list written out holds one kind, so `a list of rex and tom` is an error that says which interface both fit; start with an empty list of the interface and add each one instead.

Try it: `./easyscript run examples/programs/zoo.es`

## 17. Programs in several files

**Available.** A longer program can be split into files. The file you run says which other files it **uses**; those files only define things (functions, constants, kinds), and everything they define can be used as if it were written in the file that uses them.

`shapes.es`:

```
a shape is one of:
    a circle with a radius (a number).
    a rectangle with a width (a number) and a height (a number).

to area of s (a shape), giving back a number:
    check which kind s is:
        when it's a circle:
            give back its radius times its radius times 3.14159
        when it's a rectangle:
            give back its width times its height
```

`main.es`:

```
use shapes.

let bed be a new rectangle with width 4 and height 3
say area of bed
```

```
12
```

### Using a file

`use shapes.` (or `use the shapes file.`) reads `shapes.es` from the same folder as the file that says it. A used file can use other files too. Each file is read and checked once, however many files use it, but files can't use each other in a circle: that's an error showing the circle ("These files use each other in a circle: first.es uses second.es and second.es uses first.es.").

A file that's used can only define things. A sentence that does something, like `say "hi"`, is an error in it ("shapes.es is used by another file, so it can only define things."): only the file you run does things.

### Whose name is it?

A file sees its own definitions first, then those of the files it uses. Two used files can both define `double`; that's fine until `double` is used, which is then an error that names both files. Say which one you mean with the file's name and `'s`, which also works anytime:

```
say helpers's double with 3
say math's double with 3
```

Kinds of record, kinds with variants and interfaces are different: they share one set of names across the whole program, so two files can't both define a `circle`.

### Keeping a definition to its file

Put `privately` before `to` or `keep` to keep a helper to its own file:

```
privately to tidy with text:
    give back text without extra spaces
```

Another file using `tidy` gets an error saying where it's defined ("\"tidy\" is private to helpers.es, so only that file can use it.").

### When something goes wrong

Errors name the file as well as the line, before the program runs and while it runs:

```
Line 5 of calc.es: You divided by zero.
        give back (numbers added together) divided by (the number of items in numbers)
                  ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
while working out "average"
called from line 5 of main.es: say average of (an empty list of numbers)
```

Try it: `./easyscript run examples/programs/multi_file/main.es`

## 18. The standard library

**Available.** These phrases work in every program, without `use`. The full list, with the exact meaning of each, is in [stdlib.md](stdlib.md).

### Math

```
say the square root of 16.               # 4
say 2 to the power of 10.                # 1024
say the absolute value of -3.            # 3
say 2.5 rounded.                         # 3
say 2.1 rounded up.                      # 3
say pi rounded to 2 decimal places.      # 3.14
say the bigger of 3 and 4.               # 4
```

`the square root of` a number below zero, a number below zero to a power that isn't whole, and 0 to a power below zero are errors while the program runs. `pi` is a name the program can still make for itself.

### Random numbers

```
say a random number from 1 to 6.
say a random decimal between 0 and 1.
say pick one of cards.
say cards shuffled.
shuffle cards.
say roll a die.
```

Each run gives different numbers. `use the same random numbers every time.` makes every run give the same ones, for testing a game. Try `./easyscript run examples/programs/dice.es`.

### Dates

```
say today.                               # Sunday, October 11, 2026
say the time now.                        # Sunday, October 11, 2026, 2:05 PM
say 3 days ago.
say today's weekday.                     # Sunday
if the last change of file "log.txt" is in this month:
    say "The log is fresh.".
say the number of days between today and (30 days from now).
wait 2 seconds.
```

A date is a kind of its own, written `(a date)` in a definition. Dates compare with `is`, `is before` and `is after`, and become text when printed or joined. They can be kept in lists, maps and records like any other value:

```
an order is a kind of thing with:
    item, which is text.
    day, which is a date.

let orders be a list of (a new order with item "tea", day today) and (a new order with item "jam", day 40 days ago).
for each sale in orders:
    if sale's day is in this month, say sale's item.
let days be a list of today, yesterday and tomorrow.
say days sorted.
```

Sorting puts the earliest first. Inside a list, map or record a date prints in parentheses, because it has commas of its own: `(Saturday, October 10, 2026), (Sunday, October 11, 2026) and (Monday, October 12, 2026)`. A date can't be a map's key, and can't be "or nothing". See `examples/programs/orders.es`. `clear the screen.` and `wait` make a countdown: `./easyscript run examples/programs/countdown.es`.

### Files and folders

```
make a folder called "box".
put "hello" in file "box/a.txt".
copy the file "box/a.txt" to "box/b.txt".
rename the file "box/b.txt" to "box/c.txt".
say the files in "box".                  # a.txt and c.txt
delete the folder "box" and everything in it.
```

Paths are text. Nothing is ever replaced: making, renaming, copying or moving onto a name that's taken is an error. Deleting a folder that isn't empty needs `and everything in it`. Try `./easyscript run examples/programs/tidy_folder.es`.

### CSV files

```
read csv file "sales.csv" and call it rows.
for each row in rows:
    say the value for "region" in row.
save totals as csv file "totals.csv".
```

Each row is a map from the column's name (from the header line) to its text. Mistakes in the file say where: "In sales.csv, line 14 has 3 values, but the header has 4." Try `./easyscript run examples/programs/sales_report.es`.

## 19. Programs and the system

**Available.** A program can use the words it was started with, read what's sent to it, look at settings, run other programs, and tell whoever started it whether it worked. The exact meanings are in [stdlib.md](stdlib.md#system-tools).

### Words given to the program

Words written after the program's name when it's started reach it as a list of text:

```
note: greet.es
if the number of words given to the program is 0:
    complain "Tell me who to greet, like: easyscript run greet.es Ada".
    stop the program, saying it failed.
for each name in the words given to the program:
    say "Hello, " followed by name followed by "!".
```

```bash
./easyscript run greet.es Ada Grace     # Hello, Ada!  Hello, Grace!
./easyscript build greet.es -o greet
./greet "Ada Lovelace"                  # Hello, Ada Lovelace!
```

`word 1 given to the program` is the first one. Asking for one that isn't there stops the program ("There's no word 2: the program was given 1 word."), so check `the number of words given to the program` first.

### Text sent to the program

`the text sent to the program` is everything sent to it: a file given with `<`, or another program's output given with `|`. `the lines sent to the program` is the same, split into lines:

```
let count be 0.
for each line in the lines sent to the program:
    if line contains "error", add 1 to count.
say count followed by " lines mention an error.".
```

```bash
./easyscript run errors.es < server.log
```

`ask` still works: it reads one line, and `the text sent to the program` is what's left after the lines `ask` has read.

### Settings

`the setting called "HOME"` is an environment setting, or nothing if it isn't set, so it has to be checked before it's used:

```
let editor be the setting called "EDITOR".
if editor is nothing:
    say "No editor is set.".
otherwise:
    say "Your editor is " followed by editor followed by ".".
```

### Running other programs

`run` starts another program with a list of words and waits for it:

```
run "git" with "status".
```

Its output is shown as it comes, and if it doesn't work, this program stops too. To look at what it printed, and carry on whether it worked or not, call the result something:

```
run "git" with "log", "-1" and "--format=%s" and call the result last.
if last worked:
    say "The last change: " followed by last's output.
otherwise:
    complain "git said: " followed by last's error output.
```

A program result has an `output`, an `error output` and an `exit code` (0 when it worked). The words after `with` go to the program exactly as written: **there's no shell in between**, so a word like `"notes; rm -rf ~"` is one strange word, never a second command. A list of text gives one word per item: `run "ls" with "-l" and the files in "photos".`.

### The error output and the exit code

`complain X` is like `say X`, but writes to the error output, which stays on the screen when the normal output goes to a file (`./report > out.txt`). `stop the program, saying it failed` ends it with the code 1, and `stop the program with code N` with any code from 0 to 255, so scripts and other programs can tell whether it worked:

```bash
./greet && echo "it worked"
```

## 20. The keyboard

**Available.** Programs can read keys one at a time, without Enter: for menus, games and anything that reacts as you type.

```
say "Press a key (Q stops).".
forever:
    wait for a key and call it key.
    if key is "q", stop the loop.
    say "You pressed " followed by key.
```

`wait for a key and call it key` waits for the next key. `the key being pressed` doesn't wait: it's a key pressed since the last one read, or nothing, so a game can keep moving between key presses:

```
forever:
    let key be the key being pressed.
    if key is not nothing:
        if key is "left", say "going left".
        if key is "escape", stop the loop.
    wait 100 milliseconds.
```

Arrow keys are `"up"`, `"down"`, `"left"` and `"right"`; others with names are `"enter"`, `"escape"`, `"space"`, `"tab"`, `"backspace"`, `"delete"`, `"home"`, `"end"`, `"page up"`, `"page down"` and `"insert"`. Any other key is the character it types (`"a"`, `"A"`, `"7"`).

While the keyboard is in use, keys aren't shown as you type them. The terminal goes back to normal when the program ends, however it ends (even with Ctrl+C or a mistake), and while `ask` reads a line or another program runs. Keys can also come from a file (`./game < keys.txt`), read one at a time, which is how games are tested.

Try Snake: `./easyscript run examples/programs/snake/play.es`. The keyboard works on macOS and Linux; on Windows, use WSL until the Windows port.

## 21. Questions and totals

**Available.** Most of what people do with a list is ask about it: how many, is any, is every, what's the total, which is the biggest. EasyScript has a phrase for each, so you don't need a loop with a counter.

```
let scores be a list of 72, 45, 95, 60 and 88.
say how many scores are above 70.            # 3
if any score is above 90, say "Someone did great.".
if every score is at least 40, say "Everyone passed.".
if no score is below 0, say "No negatives.".
say the total of scores, the average of scores, the median of scores.   # 360 72 72
say the top 3 scores.                        # 95, 88 and 72
```

`any score` looks through the list named like its plural, `scores`. To name the list yourself, write `any score in results is ...`, or use the plural: `all scores are below 100`, `any of scores are above 95`, `none of scores are above 95`.

### Records, fields and groups

With records, say which field:

```
an order has a customer (text), a region (text) and an amount (a number).
...
say the total amount of orders.                      # 390
say the total amount of orders per region.           # "east" is 140 and "west" is 250
say the number of orders per region.                 # "east" is 2 and "west" is 1
say the order with the highest amount's customer.    # Bo
say the top 2 orders by amount.
if every order's amount is above 10, say "All above 10.".
say how many orders whose amount is over 50 there are.
```

`per` makes a map from each group to its result, in the order the groups first came up; add `, sorted biggest first` to put the biggest first, and go through it with `for each region and total in ...:`. With dates, `per month`, `per year`, `per day` and `per weekday` group by the item's date (`per month of signup` when it has more than one).

### Math words

`half of price`, `twice price`, `20 percent of price` and `the remainder of 17 divided by 5` mean what they say. `minutes divided by 60, rounded down` divides and then rounds: a comma before `rounded`, `sorted` or `as` puts it after the whole value. `total is infinite` asks whether a number is infinity.

### What each one means exactly

Each phrase is one pass over the list, written here with the loops it stands for (`it` is the item):

- `how many L are ...` starts a count at 0 and adds 1 for each item that passes. `how many L there are` is the number of items.
- `any ITEM ...` is no until an item passes, then yes, and stops. `every ITEM ...` is yes until an item fails, then no, and stops. `no ITEM ...` is yes until an item passes, then no, and stops.
- `the total of L` starts at 0 and adds each item (or each item's field). `the average` is that total divided by the number of items, an error for an empty list. `the median` keeps the numbers, sorts them, and takes the middle one (or halfway between the two middle ones).
- `the top N L` keeps the N biggest seen so far (in a small heap, so it never sorts the whole list) and gives them back biggest first; `the lowest N` the same for the smallest. Ties keep the item that came first.
- `the N most common L` counts each item in a map, then takes the N biggest counts.
- `the ITEM with the highest FIELD` keeps the best item seen so far; the first one wins a tie. An empty list is an error.
- `... per FIELD` keeps a map from each group to its running result.
- A filter in the list (`the orders whose amount is over 50`) is tested in the same pass, unless its test calls a definition (then the filter keeps its own pass, so calls happen in order).

`easyscript plan FILE` (or `easyscript build --plan FILE`) prints one line for each of these, saying what it became:

```
Line 3: "how many": counted in 1 pass, no list made.
Line 7: "the top": 1 pass keeping the best ones in a heap, no sort of the whole list.
```

### Things to watch

- `the total of`, `the average of`, `the median of` and `the number of` mean the program's own definition when it has one with that name: `total of scores` calls `to total of numbers:`. Written with `the`, it's an error asking which was meant.
- `the total of rex`, when `rex` is a record with a field called `total`, is an error: the phrase adds up a list. Write `rex's total` for the field.
- These phrases are values, so `say how many scores are above 70.` works anywhere a value does.

## 22. Choices without if

**Available.** Most programs are full of `if`s that say what something *is*: an order is big when its amount is over 1000, shipping costs 5 when the total is over 50. EasyScript lets you say those things once, as sentences, and then use them anywhere.

### Defaults

```
add 1 to the value for word in counts, starting at 0.
say the nickname, or "friend" if there isn't one.
```

`starting at` gives a key that isn't in the map yet a value to start from, and `, or` gives another value when something is nothing. In both, the other value is only worked out when it's needed.

### Describing words

```
an order is big if its amount is over 1000.
a cheap order is one whose amount is under 100.

if o is big, say "Big order!".
say how many orders are big.
say the cheap orders.
if every order is big, say "All big.".
```

`its` (or `their`, or `it`) is the order being described. Once a word is defined, `X is big` means the word, and `the big orders` means the orders it describes.

### Values worked out from a record, and named values

```
an order's profit is its amount minus its cost.
a customer's profit is the total profit of their orders.
the best customer is the one with the highest profit.
the big orders are the ones whose amount is over 1000.

say o's profit, the profit of o.
say the best customer's name.
```

A value like `profit` is worked out each time it's used, from the record it belongs to, so it's always up to date with the record's fields. A named value (`the best customer`) means its words, worked out where it's used: `the customer with the highest profit`, going through `customers`.

### Tables

```
the shipping cost depends on the total:
    0 when it's over 100.
    5 when it's over 50.
    10 otherwise.

the price of a drink depends on its size:
    3 when it's "small".
    4 when it's "medium".
    5 otherwise.
```

The first table makes `shipping cost` right there, from the total as it is then (like `let`). The second says what every drink's price is, worked out whenever it's used: `latte's price`. Each line is a value and when it applies (`when it's ...`, `when it starts with ...`, any comparison), and the last line says `otherwise`, so there's always a value. Without it, EasyScript says which case is missing: ""the shipping cost" doesn't say what it is when the total is 50 or less."

### Rules

```
an age must be between 0 and 150.
a customer's email must contain "@".
a team's players must not be empty.
```

A rule is checked every time a field gets a value: in `a new ...`, `set ...`, and `add ... to ...` (and the other changes). If it's broken, the program stops: "Line 12: An age must be between 0 and 150, but this one is 200."

### Acting on a whole group

```
remove the tasks that are done.
remove the scores that are below 10.
set every monster's color to "blue".
add 10 to every player's score.
```

### Events

```
let score be 0.
every 2 seconds:
    add 40 to score.
    say "Score:", score.
whenever score is over 100:
    say "Level up!".
    stop the program.
whenever a key is pressed:
    if it is "q", stop the program.
every Monday at 9am:
    say "Time for the meeting.".
```

Events run once the rest of the program has: `every 2 seconds:` runs its lines every 2 seconds, `every Monday at 9am:` at that time, `whenever ...:` each time its condition becomes yes, `when ..., ...` the first time, and `whenever a key is pressed:` for each key (`it` is the key). The program keeps going until `stop the program`. The Snake game (`examples/programs/snake/play.es`) is written this way.

### What each one means exactly

- `a customer is loyal if C.` is `to is loyal, for a customer: give back C`, with `it` the customer; `rex is loyal` calls it.
- `an order's profit is V.` is a method giving back V; `o's profit` calls it.
- `the best customer is the one with the highest profit.` puts `the customer with the highest profit` wherever `the best customer` is written.
- A table is the first line's value whose condition is yes, tried in order: `if X is over 100 ... otherwise if X is over 50 ... otherwise ...`.
- A rule is a check after the field gets its value, stopping the program when it's no.
- Events are a loop after the program's other sentences: check each `whenever` (it runs when its condition is yes now and was no the round before) and `when` (the first time only), wait for a key until the next schedule is due, then run the schedules that are due.
- `remove the tasks that are done.` is `keep only the tasks in tasks where not (it is done)`; `set every monster's color to "blue".` is `for each monster in monsters: set it's color to "blue"`.

### Things to watch

- Definitions (describing words, values, tables of a kind, rules) only see the value they're about and constants, like `to` definitions: "This only sees the value it's about ("it") and constants."
- `a big order is one whose ...` (with "a ... one") describes, and `the big orders are the ones whose ...` (with "the ... the ones") names: the words `a` and `the` tell them apart.
- A table that makes a name (`the shipping cost depends on the total:`) works it out once, where it's written; use `the price of a drink depends on ...` (or a value of a record) for one that's worked out each time.

## 23. Talking to it

**Available.** EasyScript reads questions, requests and steps in a row the way people say them, and each means exactly what the same words mean in a program.

### Questions and requests

```
what is the total?          # say the total
total?                      # say the total
is total above 100?         # say total is above 100
how many orders are big?
which customer has the highest total?
show me the total.
can you show me the big orders?
```

A `?` ends a sentence like a period. `show me`, `give me`, `tell me` and `list` mean `say`, and polite words before a sentence (`please`, `can you`, `I want to`, `let's`, ...) are left out. Questions that start with `who`, `where`, `when` or `why` aren't understood yet, and EasyScript says so.

### Steps in a row

```
read the table in "sales.csv", then total the amount per month,
then sort it biggest first, then save it as "best months.csv".
```

Sentences joined by `, then`, `and then` or `and` are steps, done in order. In each step, `it` is what the step before gave: the table it read, the value it worked out, the list it sorted, the file it wrote to, or the name it made. A line that ends with a comma, `then` or `and` goes on to the next line. When `it` could also mean a loop's or a filter's item, EasyScript asks for a name instead.

`total the amount per month`, `average the score`, `median the price` and `count the big orders` are actions: they work something out and keep it (`... and call it totals`, or for the next step's `it`).

### Tables in files

```
read the table in "sales.csv" and call it sales.
say the total amount of sales per region, sorted biggest first.
say how many sales whose amount is over 100.
save the sales whose region is "East" as "east.csv".
```

A table's rows are records whose fields are the columns the program uses: `amount` is a number because the program adds it up, `region` is text. The program checks the file when it reads it: a column that isn't there stops it with "sales.csv has no column called "amout". Did you mean "amount"?", and so does a value of the wrong kind. Saved rows keep every column of the file, with any changes the program made.

### Asking for numbers and yes or no

```
ask for a number "How old are you? " and call it age.
ask yes or no "Do you like tea? " and call it likes tea.
```

They ask again until the answer fits: "Please type a number, like 42." or "Please answer yes or no." (`y` and `n` work too, in any case).

### Writing numbers for people

```
say 1234.5 as money.          # $1,234.50
say 0.25 as a percent.        # 25%
say 1234567 with commas.      # 1,234,567
```

Like `as text`, they take the value just before them, so write `(price times 3) as money`.

### Values that are always up to date

```
let prices be a list of 3, 4 and 5.
the total is always the sum of prices.
add 10 to the end of prices.
say total.                    # 22
```

### When a file can't be used

```
try to read file "notes.txt" and call it notes.
if that fails:
    say "No notes yet:", the reason.
    let notes be "".
```

`try to` works with sentences that use a file or a folder. EasyScript checks first that the sentence can be done (the file is there, the folder for a new file exists, nothing is in the way of a new name); if it can't, the sentences after `if that fails` run instead, and `the reason` says why.

### What each one means exactly

- `ask for a number Q and call it age` asks Q until the answer reads as a number, then keeps that number. `ask yes or no` is the same with `yes`/`y`/`no`/`n`.
- `X as money` is a dollar sign, X rounded to two decimals, with commas between thousands. `X as a percent` is X times 100 (up to 10 digits) followed by `%`.
- `the total is always E` makes `total` mean E wherever it's read.
- `try to S. if that fails, F` checks S's files first and runs S, or else F with `the reason`.
- `what is X?` and `X?` are `say X`; `is X above 100?` is `say X is above 100`; `which customer has the highest total?` is `say the customer with the highest total`.
- `S1, then S2` is S1, then (if S2 says "it") a hidden name for what S1 gave, then S2 with "it" meaning that name.
- `total the amount per month` is `let (a hidden name) be the total amount of it per month`.
- `read the table in F` reads the csv file, makes a record for each row with the columns the program uses, and keeps them in a list.

