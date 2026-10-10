# The EasyScript Language Guide

This guide teaches EasyScript from zero, one chapter at a time. Each chapter is marked with its status:

- **Available:** you can try everything in the chapter today. "Available (lexer)" and "Available (parser)" mean the compiler already reads and understands it (try `easyscript tokens` or `easyscript ast`), but it doesn't run yet.
- **Coming soon:** the chapter describes the *planned* language. Its examples don't compile yet, and the exact wording may change. They're here so you can see where the language is going and give feedback.

The full list of words and patterns is in the [vocabulary](vocabulary.md).

| Chapter | Status |
|---|---|
| [0. Getting set up](#0-getting-set-up) | Available |
| [1. How EasyScript reads your sentences](#1-how-easyscript-reads-your-sentences) | Available (lexer) |
| [2. Values and expressions](#2-values-and-expressions) | Available (parser) |
| [3. Variables](#3-variables) | Available (parser) |
| [4. Output and input](#4-output-and-input) | Available (parser) |
| [5. Decisions](#5-decisions) | Coming soon |
| [6. Loops](#6-loops) | Coming soon |
| [7. Functions](#7-functions) | Coming soon |
| [8. Files](#8-files) | Available (parser), except `file X exists` |
| [Appendix: the legacy prototype](#appendix-the-legacy-prototype) | Available (legacy) |

---

## 0. Getting set up

**Status: Available**

Build the compiler (you need `cc`, `make`, and macOS or Linux):

```bash
git clone https://github.com/zakeripouya/EasyScript.git
cd EasyScript
make
```

EasyScript programs are plain text files ending in `.es`. Today, the most useful command for the new syntax is `tokens`, which shows how the compiler reads a file:

```bash
./easyscript tokens examples/lexer/sentences.es
```

---

## 1. How EasyScript reads your sentences

**Status: Available (lexer).** Everything in this chapter is what the compiler's lexer does today, and you can watch it with `easyscript tokens`. What the sentences *mean* comes in the later chapters.

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

A name starts with a letter and can contain letters, digits, and underscores: `total`, `player_2`. An apostrophe can sit between letters, so `guest's` and `isn't` are single words.

There are no reserved words. Whether `count` is a command or a variable depends on where it appears in the sentence, so you can name a variable `count`.

### Numbers

Whole numbers (`42`) and decimals (`19.99`). A period after a number ends the sentence; it's only a decimal point when a digit follows it:

```
set price to 19.99.
set count to 3.
```

A number and a word need a space between them: `3times` is an error, `3 times` is fine.

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

**Status: Available (parser).** The parser understands everything in this chapter, and `easyscript ast` shows how it reads each line. Nothing runs yet. The examples show expressions on their own; in a program they appear inside statements, such as `say price plus tax` (chapter 4).

### Values

| Kind | Examples |
|---|---|
| Numbers | `42`, `19.99` |
| Text | `"Hello, world!"` |
| Yes and no | `yes`, `no` (or `true`, `false`) |
| Nothing | `nothing` |
| Names | `total`, `player_2`, `guest's` |

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

`plus`/`+`, `minus`/`-`, `times`/`multiplied by`/`*`, `divided by`/`/`, and `mod`/`%` (the remainder). As in maths, multiplying and dividing come first: `price plus tax times 2` means `price plus (tax times 2)`. Use parentheses to change that: `(price plus tax) times 2`.

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

The full list is in the [vocabulary](vocabulary.md#comparisons). A sentence can hold one comparison at a time: instead of `1 < x < 10`, write `1 < x and x < 10`. The compiler tells you this if you forget.

### Combining yes and no

`and`, `or`, and `not` combine comparisons. `not` comes first, then `and`, then `or`:

```
age is at least 13 and age is under 20
day is "sat" or day is "sun"
not done and ready
```

### Working with text

`and` also joins text, and so does `followed by`. A value can be turned into text with `as text`, and text into a number with `as a number`:

```
"Hello, " and name
"Total: " followed by total as text
"42" as a number plus 1
length of name
contents of file "notes.txt"
```

Try it: `./easyscript ast examples/parser/text.es`

### Calling functions

A function is called by its name, with `using` and its inputs:

```
greet using "Ada"
add using 2, 3
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

## 3. Variables

**Status: Available (parser).** These sentences parse today (`easyscript ast` shows them) but don't run yet.

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

**Coming soon:** using a name that doesn't exist will be an error that suggests the closest name you did create (the error format is ready; the checker that produces it isn't):

```
Line 4: I don't know anything called "totl".
    add 5 to totl.
             ^^^^
Did you mean "total"? You made it on line 1.
```

Try it: `./easyscript ast examples/parser/variables.es`

---

## 4. Output and input

**Status: Available (parser).** These sentences parse today but don't run yet.

`say` prints a value on its own line. `print`, `show`, `display`, and `write` mean the same thing:

```
say "Hello, world!".
print total.
show "Total: " followed by total as text.
```

`ask` prints a question and keeps the line of text the user types:

```
ask "What's your name? " and call the answer name.
say "Nice to meet you, " and name and ".".
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

Try it: `./easyscript ast examples/parser/output.es` and `examples/parser/input.es`

---

## 5. Decisions

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

```
if temperature is greater than 30:
    say "It's hot.".
otherwise if temperature is less than 10:
    say "It's cold.".
otherwise:
    say "It's nice.".
```

The conditions are the comparisons from [chapter 2](#2-values-and-expressions), which already parse today; the `if` sentences around them are coming soon.

If a sentence could be read two ways, EasyScript doesn't guess. It stops and shows you the possible readings so you can say which one you meant.

---

## 6. Loops

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

```
repeat 3 times:
    say "hip hip hooray!".

set count to 3.
repeat while count is greater than 0:
    say count.
    subtract 1 from count.

for each n from 1 to 10:
    say n * n.
```

`stop.` leaves a loop early, and `skip.` jumps to the next round.

---

## 7. Functions

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

A function is a named set of steps. It's defined with `to`, like a recipe ("to make tea: ..."):

```
to greet using name:
    say "Hello, " and name and "!".

greet using "Ada".
greet using "Alan".
```

A function can give back a result:

```
to double using number:
    give back number * 2.

say double using 21.
```

---

## 8. Files

**Status: Available (parser), except `file X exists` (coming soon).** These sentences parse today but don't run yet.

```
write "first line\n" to file "notes.txt".
append "second line\n" to file "notes.txt".
read file "notes.txt" and call it notes.
say notes.
```

`write X to file F` replaces the file's contents, and `append` adds to the end. Inside an expression, `contents of file "notes.txt"` gives the whole file as text.

Coming soon: checking whether a file exists.

```
if file "settings.txt" exists:
    say "Found the settings.".
```

Try it: `./easyscript ast examples/parser/files.es`

---

## Appendix: the legacy prototype

**Status: Available (legacy).** The original 2024 prototype syntax still compiles and runs end to end. It will be removed once the new parser works, so it's documented here only so you can try the full compile-and-run pipeline today.

```
MAKE A VARIABLE language ASSIGN "EasyScript"
MAKE A VARIABLE year ASSIGN 2026
PRINT # language
PRINT # year
```

```bash
./easyscript run examples/legacy/variables.es
```

Every legacy feature is in the [vocabulary](vocabulary.md#legacy-prototype-syntax), and runnable programs are in [`examples/legacy/`](../examples/legacy/).
