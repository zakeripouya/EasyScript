# Vocabulary

Every word, symbol, and sentence pattern in EasyScript, grouped by category, with an example and its status.

**Status labels:**

- **Available:** works today. For the new syntax that currently means the *lexer* reads it correctly (check with `easyscript tokens`). Nothing in the new syntax runs yet.
- **Coming soon:** planned for the phase shown. Sentence patterns marked this way are **proposals**: the wording may change before they ship, and they don't compile today.
- **Legacy:** the 2024 prototype syntax. It runs today but is being removed; don't write new code in it.

In patterns, `NAME` is a name you choose, `VALUE` is any value or expression, and `CONDITION` is anything that is true or false. Keywords are case-insensitive. The filler words `the`, `a`, and `an` may appear anywhere and are ignored, so they're left out of the patterns below.

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
| whole numbers | Integers | `42`, `007` | Available (lexer) |
| decimals | Digits, a point, digits | `19.99` (but `3.` is the number 3 and then a period) | Available (lexer) |
| `"..."` | Text. It must end on the line it starts on | `"Hello, world!"` | Available (lexer) |
| `\n` `\t` `\"` `\\` | Escapes inside text: new line, tab, quote, backslash | `"Line one\nLine two"` | Available (lexer) |
| names | Start with a letter, then letters, digits, `_`, or an apostrophe between letters | `total`, `player_2`, `guest's` | Available (lexer) |
| `true`, `false` | Yes/no values | `set done to false.` | Coming soon (Phase 1) |

## Arithmetic and comparison symbols

| Symbol | Meaning | Example | Status |
|---|---|---|---|
| `+` `-` `*` `/` `%` | Add, subtract, multiply, divide, remainder | `set area to width * height.` | Available (lexer); arithmetic coming soon (Phase 1) |
| `+` on text | Joins text | `say "Hello, " + name.` | Coming soon (Phase 1) |
| `( )` | Grouping | `set x to (a + b) * 2.` | Available (lexer); meaning coming soon (Phase 1) |
| `=` `!=` | Equal, not equal | `if total != 0:` | Available (lexer); meaning coming soon (Phase 1) |
| `<` `>` `<=` `>=` | Less, greater, at most, at least | `if age >= 18:` | Available (lexer); meaning coming soon (Phase 1) |

## Variables

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `set NAME to VALUE.` | Create a variable or change its value | `set total to 0.` | Coming soon (Phase 1) |
| `add VALUE to NAME.` | `NAME` becomes `NAME + VALUE` | `add 5 to total.` | Coming soon (Phase 1) |
| `subtract VALUE from NAME.` | `NAME` becomes `NAME - VALUE` | `subtract 1 from count.` | Coming soon (Phase 1) |
| `multiply NAME by VALUE.` | `NAME` becomes `NAME * VALUE` | `multiply price by 2.` | Coming soon (Phase 1) |
| `divide NAME by VALUE.` | `NAME` becomes `NAME / VALUE` | `divide total by 4.` | Coming soon (Phase 1) |
| `increase NAME by VALUE.` | Synonym of `add VALUE to NAME.` | `increase score by 10.` | Coming soon (Phase 1) |
| `decrease NAME by VALUE.` | Synonym of `subtract VALUE from NAME.` | `decrease lives by 1.` | Coming soon (Phase 1) |

## Output and input

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `say VALUE.` | Print a value and a new line | `say "Hello!".` | Coming soon (Phase 1) |
| `print VALUE.` | Synonym of `say` | `print total.` | Coming soon (Phase 1) |
| `show VALUE.` | Synonym of `say` | `show total.` | Coming soon (Phase 1) |
| `ask VALUE into NAME.` | Print a question, then read a line typed by the user into `NAME` | `ask "What's your name?" into name.` | Coming soon (Phase 1) |

## Decisions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `if CONDITION:` | Run the block only when the condition is true | `if total is greater than 10:` | Coming soon (Phase 1) |
| `otherwise if CONDITION:` | Checked when the conditions before it were false | `otherwise if total is 10:` | Coming soon (Phase 1) |
| `otherwise:` | Runs when every condition before it was false | `otherwise:` | Coming soon (Phase 1) |
| `else` | Synonym of `otherwise` | `else:` | Coming soon (Phase 1) |

### Conditions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `VALUE is VALUE` | Equal (same as `=`) | `if answer is 42:` | Coming soon (Phase 1) |
| `VALUE is not VALUE` | Not equal (same as `!=`) | `if name is not "":` | Coming soon (Phase 1) |
| `VALUE is greater than VALUE` | `>` | `if score is greater than 100:` | Coming soon (Phase 1) |
| `VALUE is more than VALUE` | Synonym of `is greater than` | `if age is more than 12:` | Coming soon (Phase 1) |
| `VALUE is less than VALUE` | `<` | `if count is less than 3:` | Coming soon (Phase 1) |
| `VALUE is at least VALUE` | `>=` | `if age is at least 18:` | Coming soon (Phase 1) |
| `VALUE is at most VALUE` | `<=` | `if items is at most 10:` | Coming soon (Phase 1) |
| `CONDITION and CONDITION` | Both are true | `if age is at least 13 and age is less than 20:` | Coming soon (Phase 1) |
| `CONDITION or CONDITION` | At least one is true | `if day is "sat" or day is "sun":` | Coming soon (Phase 1) |
| `not CONDITION` | The opposite | `if not done:` | Coming soon (Phase 1) |

## Loops

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `repeat VALUE times:` | Run the block a fixed number of times | `repeat 3 times:` | Coming soon (Phase 1) |
| `repeat while CONDITION:` | Run the block as long as the condition stays true | `repeat while count is greater than 0:` | Coming soon (Phase 1) |
| `while CONDITION:` | Synonym of `repeat while` | `while lives is more than 0:` | Coming soon (Phase 1) |
| `repeat until CONDITION:` | Run the block until the condition becomes true | `repeat until done:` | Coming soon (Phase 1) |
| `for each NAME from VALUE to VALUE:` | Count from the first number to the second, inclusive | `for each n from 1 to 10:` | Coming soon (Phase 1) |
| `stop.` | Leave the innermost loop | `stop.` | Coming soon (Phase 1) |
| `skip.` | Go straight to the next round of the innermost loop | `skip.` | Coming soon (Phase 1) |

## Functions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `to NAME:` | Define a function with no inputs | `to cheer:` | Coming soon (Phase 1) |
| `to NAME using NAME, NAME, ...:` | Define a function with inputs | `to greet using name:` | Coming soon (Phase 1) |
| `NAME.` | Call a function with no inputs | `cheer.` | Coming soon (Phase 1) |
| `NAME using VALUE, VALUE, ...` | Call a function with inputs | `greet using "Ada".` | Coming soon (Phase 1) |
| `give back VALUE.` | Return a value from a function | `give back number * 2.` | Coming soon (Phase 1) |
| `return VALUE.` | Synonym of `give back` | `return number * 2.` | Coming soon (Phase 1) |

## Files

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `write VALUE to file VALUE.` | Replace a file's contents | `write "hello" to file "notes.txt".` | Coming soon (Phase 1) |
| `append VALUE to file VALUE.` | Add to the end of a file | `append "more" to file "notes.txt".` | Coming soon (Phase 1) |
| `read file VALUE into NAME.` | Read a whole file into a variable | `read file "notes.txt" into text.` | Coming soon (Phase 1) |
| `file VALUE exists` | A condition: does the file exist? | `if file "notes.txt" exists:` | Coming soon (Phase 1) |

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
