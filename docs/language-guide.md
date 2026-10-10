# The EasyScript Language Guide

This guide teaches EasyScript from zero, one chapter at a time. Each chapter is marked with its status:

- **Available:** you can try everything in the chapter today.
- **Coming soon:** the chapter describes the *planned* language. Its examples don't compile yet, and the exact wording may change. They're here so you can see where the language is going and give feedback.

The full list of words and patterns is in the [vocabulary](vocabulary.md).

| Chapter | Status |
|---|---|
| [0. Getting set up](#0-getting-set-up) | Available |
| [1. How EasyScript reads your sentences](#1-how-easyscript-reads-your-sentences) | Available (lexer) |
| [2. Variables](#2-variables) | Coming soon |
| [3. Output and input](#3-output-and-input) | Coming soon |
| [4. Decisions](#4-decisions) | Coming soon |
| [5. Loops](#5-loops) | Coming soon |
| [6. Functions](#6-functions) | Coming soon |
| [7. Files](#7-files) | Coming soon |
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

EasyScript also understands a few symbols: `+ - * / %` for arithmetic, `( )` for grouping, `= != < > <= >=` for comparisons, and `,` for lists of things. See [`examples/lexer/comparisons.es`](../examples/lexer/comparisons.es).

---

## 2. Variables

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

A variable is a named value. `set` creates it or changes it:

```
set total to 0.
set name to "Ada".
```

Changing a number has its own sentences:

```
add 5 to total.
subtract 1 from total.
multiply total by 2.
divide total by 4.
```

`increase total by 5.` and `decrease total by 1.` mean the same as `add` and `subtract`.

If you use a name that doesn't exist, the compiler tells you, and suggests the closest name you did create:

```
Line 4: I don't know anything called "totl".
    add 5 to totl.
             ^^^^
Did you mean "total"? You made it on line 1.
```

---

## 3. Output and input

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

`say` prints a value on its own line (`print` and `show` mean the same thing):

```
say "Hello, world!".
say total.
say "Total: " + total.
```

`ask` prints a question and stores what the user types:

```
ask "What's your name?" into name.
say "Nice to meet you, " + name + ".".
```

---

## 4. Decisions

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

```
if temperature is greater than 30:
    say "It's hot.".
otherwise if temperature is less than 10:
    say "It's cold.".
otherwise:
    say "It's nice.".
```

Conditions can use words (`is`, `is not`, `is greater than`, `is less than`, `is at least`, `is at most`) or symbols (`=`, `!=`, `>`, `<`, `>=`, `<=`), and can be combined with `and`, `or`, and `not`.

If a sentence could be read two ways, EasyScript doesn't guess. It stops and shows you the possible readings so you can say which one you meant.

---

## 5. Loops

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

## 6. Functions

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

A function is a named set of steps. It's defined with `to`, like a recipe ("to make tea: ..."):

```
to greet using name:
    say "Hello, " + name + "!".

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

## 7. Files

**Status: Coming soon (Phase 1).** These examples are the planned syntax and don't compile yet.

```
write "first line\n" to file "notes.txt".
append "second line\n" to file "notes.txt".
read file "notes.txt" into contents.
say contents.

if file "settings.txt" exists:
    say "Found the settings.".
```

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
