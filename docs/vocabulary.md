# Vocabulary

Every word, symbol, and sentence pattern in EasyScript, grouped by category, with an example and its status.

**Status labels:**

- **Available (parser):** the parser understands it today. Check with `easyscript ast file.es`, which prints the syntax tree. It doesn't run yet: running needs the checker and code generator.
- **Available (lexer):** the lexer reads it correctly (check with `easyscript tokens`). Its meaning is not implemented yet.
- **Coming soon:** planned for the phase shown. Sentence patterns marked this way are **proposals**: the wording may change before they ship, and they don't compile today.
- **Legacy:** the 2024 prototype syntax. It runs today but is being removed; don't write new code in it.

In patterns, `NAME` is a name you choose, `X` and `Y` are any values or expressions, and `CONDITION` is anything that is yes or no. Keywords are case-insensitive. The filler words `the`, `a`, and `an` may appear anywhere and are ignored, so they're left out of the patterns below.

## Sentences and layout

| Word / symbol | Meaning | Example | Status |
|---|---|---|---|
| `.` or new line | Ends a sentence | `say "hi".` | Available (lexer) |
| `:` + indented lines | Opens a block of sentences | `if x is 1:`<br>`    say "one".` | Available (lexer) |
| indentation | Blocks are indented. A tab counts as 4 spaces. A line must line up with an enclosing block | (see [blocks.es](../examples/lexer/blocks.es)) | Available (lexer) |
| `the`, `a`, `an` | Filler words; always ignored outside text | `set the total to 0.` = `set total to 0.` | Available (lexer) |
| `#` | Comment to the end of the line | `say 1. # ignored` | Available (lexer) |
| `note:` | A line starting with `note:` (any capitalization) is a comment | `note: explain the next step` | Available (lexer) |

## Values

| Form | Meaning | Example | Status |
|---|---|---|---|
| whole numbers | Integers | `42`, `007` | Available (parser) |
| decimals | Digits, a point, digits | `19.99` (but `3.` is the number 3 and then a period, and `.5` is an error: write `0.5`) | Available (parser) |
| `"..."` | Text. It must end on the line it starts on | `"Hello, world!"` | Available (parser) |
| `\n` `\t` `\"` `\\` | Escapes inside text: new line, tab, quote, backslash | `"Line one\nLine two"` | Available (lexer) |
| `yes`, `true` | The yes value | `done is yes` | Available (parser) |
| `no`, `false` | The no value | `ready is false` | Available (parser) |
| `nothing` | No value at all | `answer is nothing` | Available (parser) |
| `it` | The result of the previous sentence (its exact meaning comes with statements) | `it plus 1` | Available (parser) |
| names | Start with a letter, then letters, digits, `_`, or an apostrophe between letters. A name is one word | `total`, `player_2`, `guest's` | Available (parser) |
| `( X )` | Grouping: work this out first | `(price plus tax) times 2` | Available (parser) |

**Words that can't be names:** the operator words `and`, `or`, `not`, `is`, `isn't`, `equals`, `reaches`, `plus`, `minus`, `times`, `multiplied`, `divided`, `mod`, `followed`, `as`, and `using`. The value words `yes`, `no`, `true`, `false`, `nothing`, and `it` can't be names either. Every other word can be, including `count`, `length`, `contents`, and `file`.

## Expressions

Expressions combine values. They're listed here from the **loosest** to the **tightest** binding: `price plus tax times 2` means `price plus (tax times 2)`, because `times` binds tighter than `plus`. Use parentheses to group things differently.

### Logic and joining text

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `X or Y` | Yes if either side is yes | `day is "sat" or day is "sun"` | Available (parser) |
| `X and Y` | With two yes/no values: yes if both are yes. With anything else: joins them as text | `age is at least 13 and age is under 20`<br>`"Hello, " and name` | Available (parser); which meaning applies is decided by the checker (coming soon) |
| `not X` | The opposite of a yes/no value | `not done` | Available (parser) |

`and` followed by the word `call` isn't treated as joining, so that a sentence can continue with `and call ...` (statements are coming soon).

### Comparisons

All comparisons are **Available (parser)**. A sentence can only have one comparison in a row: `1 < x < 10` is an error that suggests `1 < x and x < 10`.

| Means | Words | Symbols | Example |
|---|---|---|---|
| equal | `is`, `equals`, `is equal to` | `=`, `==` | `answer is 42` |
| not equal | `is not`, `isn't`, `is not equal to`, `isn't equal to` | `!=` | `name isn't ""` |
| greater | `is greater than`, `is more than`, `is bigger than`, `is above`, `is over` | `>` | `score is above 100` |
| less | `is less than`, `is smaller than`, `is below`, `is under` | `<` | `lives is below 1` |
| greater or equal | `is at least`, `is greater than or equal to`, `is more than or equal to`, `is bigger than or equal to`, `reaches` | `>=` | `points reaches 100` |
| less or equal | `is at most`, `is less than or equal to`, `is smaller than or equal to` | `<=` | `items is at most 10` |

Not available, with the error message suggesting the right one: `is not greater than` (use `is at most`), `is not less than` (use `is at least`), and `is above or equal to` (use `is at least`).

### Arithmetic

All arithmetic is **Available (parser)**.

| Means | Words | Symbol | Example |
|---|---|---|---|
| add | `plus` | `+` | `price plus tax` |
| subtract | `minus` | `-` | `total minus discount` |
| join text | `followed by` | | `"Total: " followed by total as text` |
| multiply | `times`, `multiplied by` | `*` | `width times height` |
| divide | `divided by` | `/` | `total divided by count` |
| remainder | `mod` | `%` | `minutes mod 60` |
| negative | | `-` before a value | `-temperature` |

`times` multiplies only when a value follows it, so a sentence like `repeat 3 times:` keeps its own `times`.

### Conversions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `X as a number` | Turns text into a number | `"42" as a number` | Available (parser) |
| `X as text` | Turns a value into text | `total as text` | Available (parser) |

Conversions bind tightest of all: `-x as text` means `-(x as text)`, and `"42" as a number plus 1` means `("42" as a number) plus 1`.

### Built-in values

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `length of X` | How many characters a text has | `length of name` | Available (parser) |
| `contents of file X` | Everything in a file, as text | `contents of file "notes.txt"` | Available (parser) |

These take a single value, so `length of name plus 1` means `(length of name) plus 1`. Write `length of (name plus 1)` for the other meaning.

### Calling a function

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `NAME using X, Y, ...` | Call a function with inputs, separated by commas | `greet using "Ada"`<br>`add using 2, 3` | Available (parser) |
| `NAME` | Call a function with no inputs (written like a variable) | `cheer` | Available (parser) |

Each input is a single value (a negative number, a conversion, or anything in parentheses is fine). Arithmetic straight after the inputs could belong to the last input or to the whole result, so `double using 21 plus 1` is an error that asks you to write `(double using 21) plus 1` or `double using (21 plus 1)`. Comparisons and `and`/`or` end the inputs: `double using 2 is 4` compares the result with 4.

## Statements

A program is a list of statements: sentences that each start with a word saying what to do. A period or the end of the line ends a statement, and several statements can share a line when periods separate them: `let x be 1. say x.` A statement may start with **`please`**, which is skipped: `please say "hi"` is the same as `say "hi"`.

The words that start statements are only special at the start of a sentence, so `say`, `add`, `file`, or `count` can still be variable names elsewhere.

### Variables

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `let NAME be X` | Create a variable | `let total be 0` | Available (parser) |
| `let NAME equal X` | Synonym of `let NAME be X` | `let count equal 3` | Available (parser) |
| `set NAME to X` | Change a variable's value | `set total to 10` | Available (parser) |
| `change NAME to X` | Synonym of `set` | `change total to 10` | Available (parser) |
| `add X to NAME` | `NAME` becomes `NAME plus X` | `add 5 to total` | Available (parser) |
| `subtract X from NAME` | `NAME` becomes `NAME minus X` | `subtract 1 from count` | Available (parser) |
| `increase NAME by X` | Same as `add X to NAME` | `increase score by 10` | Available (parser) |
| `decrease NAME by X` | Same as `subtract X from NAME` | `decrease lives by 1` | Available (parser) |
| `multiply NAME by X` | `NAME` becomes `NAME times X` | `multiply price by 2` | Available (parser) |
| `divide NAME by X` | `NAME` becomes `NAME divided by X` | `divide total by 4` | Available (parser) |

**Names** are single words (see [Values](#values)). `a`, `an`, and `the` can't be names because EasyScript ignores them, so `let a be 5` is an error that explains this. Words that already mean something (`yes`, `no`, `true`, `false`, `nothing`, `it`, and the operator words) can't be names either.

### Output and input

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `say X` | Print a value and a new line | `say "Hello!"` | Available (parser) |
| `print X` | Synonym of `say` | `print total` | Available (parser) |
| `show X` | Synonym of `say` | `show total` | Available (parser) |
| `display X` | Synonym of `say` | `display total` | Available (parser) |
| `write X` | Synonym of `say` (unless followed by `to file`; see [Files](#files)) | `write "done"` | Available (parser) |
| `ask X and call the answer NAME` | Print a question, then read a line of text typed by the user into `NAME` | `ask "What's your name? " and call the answer name` | Available (parser) |

### Files

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `write X to file F` | Replace a file's contents with `X` | `write "hello" to file "notes.txt"` | Available (parser) |
| `append X to file F` | Add `X` to the end of a file | `append "more" to file "notes.txt"` | Available (parser) |
| `read file F and call it NAME` | Read a whole file into a variable | `read file "notes.txt" and call it notes` | Available (parser) |
| `file X exists` | A condition: does the file exist? | `if file "notes.txt" exists:` | Coming soon (Phase 1) |

Reading a file inside an expression also works: `contents of file X` ([Built-in values](#built-in-values)).

### Ending the program

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `stop the program` | End the program right away | `stop the program` | Available (parser) |

## Decisions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `if CONDITION:` | Run the block only when the condition is yes | `if total is greater than 10:` | Coming soon (Phase 1); the condition itself is Available (parser) |
| `otherwise if CONDITION:` | Checked when the conditions before it were no | `otherwise if total is 10:` | Coming soon (Phase 1) |
| `otherwise:` | Runs when every condition before it was no | `otherwise:` | Coming soon (Phase 1) |
| `else` | Synonym of `otherwise` | `else:` | Coming soon (Phase 1) |

## Loops

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `repeat X times:` | Run the block a fixed number of times | `repeat 3 times:` | Coming soon (Phase 1) |
| `repeat while CONDITION:` | Run the block as long as the condition stays yes | `repeat while count is greater than 0:` | Coming soon (Phase 1) |
| `while CONDITION:` | Synonym of `repeat while` | `while lives is more than 0:` | Coming soon (Phase 1) |
| `repeat until CONDITION:` | Run the block until the condition becomes yes | `repeat until done:` | Coming soon (Phase 1) |
| `for each NAME from X to Y:` | Count from the first number to the second, inclusive | `for each n from 1 to 10:` | Coming soon (Phase 1) |
| `stop.` | Leave the innermost loop (`stop the program` ends the whole program) | `stop.` | Coming soon (Phase 1) |
| `skip.` | Go straight to the next round of the innermost loop | `skip.` | Coming soon (Phase 1) |

## Functions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `to NAME:` | Define a function with no inputs | `to cheer:` | Coming soon (Phase 1) |
| `to NAME using NAME, NAME, ...:` | Define a function with inputs | `to greet using name:` | Coming soon (Phase 1) |
| `give back X.` | Return a value from a function | `give back number times 2.` | Coming soon (Phase 1) |
| `return X.` | Synonym of `give back` | `return number times 2.` | Coming soon (Phase 1) |

Calling a function is an expression; see [Calling a function](#calling-a-function).

## Later phases

Records, lists, maps, modules, and static types (Phase 2), and everything after, will be added here as their designs settle. See the [roadmap](roadmap.md).

## Legacy prototype syntax

The original 2024 prototype. It **runs today** with `easyscript run`, but it's being removed once the new parser works. Keywords must be uppercase, and there are no comments.

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `MAKE A VARIABLE NAME ASSIGN NUMBER` | Whole-number variable | `MAKE A VARIABLE year ASSIGN 2026` | Legacy |
| `MAKE A VARIABLE NAME ASSIGN "TEXT"` | Text variable | `MAKE A VARIABLE language ASSIGN "EasyScript"` | Legacy |
| `PRINT # "TEXT"` | Print text | `PRINT # "Hello, world!"` | Legacy |
| `PRINT # NAME` | Print a variable | `PRINT # year` | Legacy |
| `FILE OPEN NAME` | Open (and empty) a file for writing | `FILE OPEN notes.txt` | Legacy |
| `FILE WRITE NAME VARIABLE` | Write a variable's value to an open file | `FILE WRITE notes.txt message` | Legacy |
| `FILE READ NAME` | Print the first line of an open file (followed by an extra blank line) | `FILE READ notes.txt` | Legacy |
| `FILE CLOSE NAME` | Close a file | `FILE CLOSE notes.txt` | Legacy |

Things the legacy prototype does **not** support, even though its old banner listed some of them: loops, functions, `IF`/`ELSE`, `CALL`, printing a number directly (`PRINT # 3`), and arithmetic. See the runnable [legacy examples](../examples/legacy/).
