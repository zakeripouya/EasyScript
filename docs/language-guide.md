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
| [8. Files](#8-files) | Available, except `file X exists` |
| [9. Types](#9-types) | Available |
| [10. Lists](#10-lists) | Available |

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

Capital letters don't matter for words, so `Set` and `set` are the same word. The words `the`, `a`, and `an` are always ignored, so you can write naturally: `set the total to 0.` and `set total to 0.` are the same sentence. Because they're ignored, `the`, `a`, and `an` can't be used as names.

Try it: [`examples/lexer/sentences.es`](../examples/lexer/sentences.es). The output lists every word the compiler saw, and `the` isn't among them.

### Names

A name starts with a letter and can contain letters, digits, and underscores: `total`, `player_2`, `clientName`. An apostrophe can sit between letters, so `guest's` and `isn't` are single words.

A name can also be **several words**: `let late fee be 25.`, `keep hourly rate as 150.`, `count from 1 to 3 as session number:`. Once the program makes a name like that (with `let`, `keep`, `to`, `as`, `call`, or `for each`), EasyScript recognizes it everywhere, always taking the longest name it knows: if the program has both `total` and `total cost`, then `total cost plus 1` uses `total cost`. Words that already mean something in a sentence (like `plus`, `is`, `and`, `of`, `with`, `to`, `in`, `be`) can't be part of a name of several words; trying it is an error that suggests another name:

```
Line 1: "plus" can't be part of a name, because it already means something in EasyScript.
    let price plus tax be 5
              ^^^^
Try a name without it, like "price_plus_tax".
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

```
say "Two lines:\nfirst\nsecond".
say "She said \"hi\"".
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

`plus`/`+`, `minus`/`-`, `times`/`multiplied by`/`*`, `divided by`/`/`, and `mod`/`%` (the remainder). They work on numbers; `say 7 divided by 2` prints `3.5`, and whole numbers print without decimals. Using text in arithmetic is caught before the program runs:

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

`and` also joins text, and so does `followed by`; numbers are turned into text for you. Joining happens after the maths, so `"Sum: " followed by 2 plus 3` gives `Sum: 5`. (Between two yes/no values `and` means "both"; the compiler decides which before the program runs, from the kinds of the two sides. `and` between yes/no and text is an error.) A value can be turned into text with `as text`, and text into a number with `as a number`:

```
"Hello, " and name
"Total: " followed by total as text
"42" as a number plus 1
length of name
contents of file "notes.txt"
```

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
A variable keeps the kind of value it was made with. To keep text, make a new variable for it.
```

### Values that never change

A **constant** is a name for a value that stays the same for the whole program. Make one with `keep`:

```
keep tax rate as 0.2.
keep shop at "Corner Shop".
keep welcome as "Welcome to " followed by shop.
say welcome.
```

A constant's value has to be known before the program starts, so it can use numbers, text, yes and no, and other constants, but not variables or anything the program works out while it runs. Trying to change a constant is an error that suggests `let` instead. Constants can be used everywhere, including inside functions (chapter 7).

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
say "Nice to meet you, " and name and ".".
ask "How old are you? " and call the answer age.
say "Next year you'll be " and age as a number plus 1.
```

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

If the start is bigger than the end, the count goes down by itself: `count from 3 to 1` says 3, 2, 1. `for each i from 1 to 10:` and `go from 0 to 1 in steps of 0.25:` are other ways to say the same thing. `for each score in scores:` goes through a list instead (see [Lists](#10-lists)); there, `it` is the item.

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

A function is a named set of steps. It's defined with `to`, like a recipe ("to make tea: ..."), and its sentences are indented below. Its name is every word up to `with`, `of`, `using`, or `to`, which introduce its inputs:

```
to make tea:
    say "Boil the water.".

to greet with someone:
    say "Hello, " and someone and "!".

make tea.
greet "Ada".
greet with "Alan".
call greet with "Grace".
```

So `to make tea:` is a function called `make tea` with no inputs, and `to greet with someone:` is `greet` with the input `someone`. (Writing `to greet someone:` makes a function called `greet someone`; the error you get when you then use `someone` explains how to make it an input.) A function's name can be several words, and if its inputs are introduced with `to`, it's called with `to` too:

```
to add tax to amount:
    give back amount plus amount times 0.25.

say add tax to 100.
add tax to 100.
```

A sentence that starts with a function's name calls it, even if its first word usually starts other sentences (like `add`).

A function can take several inputs, separated by `and`, and give back a result. Use it like a value with `of` (or `with`):

```
to area with width and height:
    give back width times height.

say area of 3 and 4.
if area of 2 and 5 is 10, say "ten".
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

**Status: Available, except `file X exists` (coming in Phase 2).**

```
write "first line" to file "notes.txt".
append "second line" to file "notes.txt".
read file "notes.txt" and call it notes.
say notes.
```

`write X to file F` replaces the file's contents, and `append` adds to the end; both end what they write with a new line, so each one is a line of the file. Reading gives back the whole file without its last new line, so the example prints the two lines. If a file can't be read, the program stops with a message like `Line 3: I couldn't read the file "notes.txt": it doesn't exist.` Inside an expression, `contents of file "notes.txt"` gives the whole file as text.

Coming soon: checking whether a file exists.

```
if file "settings.txt" exists:
    say "Found the settings.".
```

Try it: `./easyscript run examples/programs/files.es`

---

## 9. Types

**Status: Available.**

Every value has a **kind** (its type): a number, text, yes/no, or a list of one of these (see [Lists](#10-lists)). The compiler works out the kind of every value before the program runs, so mistakes like adding text to a number are reported with the line and a caret, the same way as a misspelled name, instead of stopping the program halfway. The program then runs faster too, because it never has to check kinds while it runs.

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
    say "Hello, " and who.
to both with first (yes or no) and second (yes or no):
    give back first and second.
```

The kinds are `a number`, `text`, `yes or no` (or `yes/no`), and lists like `a list of numbers`; `giving back nothing` says the function gives back no value. Kinds are optional, and you can write some and leave out others; whatever you write is checked like everything else.

**Why this wording?** It had to read like English and not look like the `as a number` / `as text` conversions, which change a value. Other choices that were considered:

| Considered | Why not |
|---|---|
| `to area with width as a number:` | Reads like the conversion `width as a number`, which means something different |
| `to area with number width:` | Puts the kind before the name, which reads awkwardly with "with" |
| `to area with width: number` | The colon already opens the function's block |
| `to area with width which is a number:` | Long, and "is" already compares values |
| `to area ... gives back a number:` / `returns a number` | "to area ... gives back" doesn't read as a sentence; `giving back` matches `give back` |

### What's still checked while the program runs

Kinds can't tell you everything. These are still found while the program runs, with the line they happened on: asking for a list item that isn't there, dividing by zero, counting with a step that isn't more than zero, a number of times that isn't a whole number, text that isn't a number in `as a number`, files that can't be read or written, a function that gives back a value but reaches its end without `give back`, and functions calling each other too deeply.

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

**Sorting** puts numbers smallest first ("not a number", which only comes from sums like infinity minus infinity, goes last) and text in alphabetical order (by character codes, so capital letters come first). Add `, biggest first` (or `smallest first`) to choose the order, and sort text, or lists of lists, by their length with `sort names by their length` (`, longest first` or `shortest first`). Items that are equal stay in the order they were in. Sorting changes the list itself.

### Going through a list

```
for each score in scores:
    say score.
```

Inside the loop, `it` is the item too. `stop the loop` and `skip this one` work as in every loop. The loop goes through the items the list had when it started, so adding to or removing from the list inside the loop doesn't change which items it visits.

### New lists from old ones

`every NAME in L` works out the rest of the value for each item and gives the results as a new list:

```
say every score in scores plus 5.          # 77, 100 and 93
say every name in names followed by "!".
```

`only L that are ...` keeps the items for which a comparison is yes. The comparisons are the usual ones with `are` instead of `is` (`are at least`, `are less than`, `are not`, `are empty`, ...):

```
let passing be only the scores that are at least 70.
```

And to sum up a list:

- `scores added together`: the total of a list of numbers
- `the biggest of scores`, `the smallest of scores`: for numbers or text
- `scores reversed`: a new list, last item first

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
- Lists can't be constants (`keep`), because lists can change. Use `let`.

### What's checked while the program runs

Asking for an item that isn't there (or item 0, or item 1.5), a range whose positions aren't in the list or go backwards, a negative or fractional number of items, inserting past the end, and asking for the first, last, biggest or smallest item of an empty list (or taking it away with `without`) stop the program with the line and what went wrong. Everything else about lists, including every kind mistake, is checked before the program runs.

Try it: `./easyscript run examples/programs/grade_statistics.es` and `./easyscript run examples/programs/top_scores.es`


