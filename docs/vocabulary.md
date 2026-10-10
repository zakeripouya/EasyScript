# Vocabulary

Every word, symbol, and sentence pattern in EasyScript, grouped by category, with an example and its status.

**Status labels:**

- **Available:** works today: it compiles and runs.
- **Parses only:** the parser understands it (see `easyscript ast`), but using it in a program is an error because its meaning isn't implemented yet.
- **Coming soon:** planned for the phase shown. Sentence patterns marked this way are **proposals**: the wording may change before they ship, and they don't compile today.

In patterns, `NAME` is a name you choose, `X` and `Y` are any values or expressions, and `CONDITION` is anything that is yes or no. Keywords are case-insensitive. The filler words `the`, `a`, and `an` may appear anywhere and are ignored, so they're left out of the patterns below.

## Sentences and layout

| Word / symbol | Meaning | Example | Status |
|---|---|---|---|
| `.` or new line | Ends a sentence | `say "hi".` | Available |
| `:` + indented lines | Opens a block of sentences | `if x is 1:`<br>`    say "one".` | Available |
| indentation | Blocks are indented. A tab counts as 4 spaces. A line must line up with an enclosing block | (see [blocks.es](../examples/lexer/blocks.es)) | Available |
| `the`, `a`, `an` | Filler words; always ignored outside text | `set the total to 0.` = `set total to 0.` | Available |
| `#` | Comment to the end of the line | `say 1. # ignored` | Available |
| `note:` | A line starting with `note:` (any capitalization) is a comment | `note: explain the next step` | Available |

## Values

| Form | Meaning | Example | Status |
|---|---|---|---|
| whole numbers | Integers | `42`, `007` | Available |
| decimals | Digits, a point, digits | `19.99` (but `3.` is the number 3 and then a period, and `.5` is an error: write `0.5`) | Available |
| `"..."` | Text. It must end on the line it starts on | `"Hello, world!"` | Available |
| `\n` `\t` `\"` `\\` | Escapes inside text: new line, tab, quote, backslash | `"Line one\nLine two"` | Available |
| `yes`, `true` | The yes value | `done is yes` | Available |
| `no`, `false` | The no value | `ready is false` | Available |
| `nothing` | No value at all | `answer is nothing` | Available |
| `it` | Reserved for referring to an earlier result | `it plus 1` | Parses only |
| names | Start with a letter, then letters, digits, `_`, or an apostrophe between letters. A name is one word | `total`, `player_2`, `guest's` | Available |
| `( X )` | Grouping: work this out first | `(price plus tax) times 2` | Available |

**How values print:** whole numbers print without decimals (`7`, not `7.0`), other numbers with up to 15 significant digits (`0.1 plus 0.2` prints `0.3`), yes/no as `yes`/`no`, and nothing as `nothing`.

**Words that can't be names:** the operator words `and`, `or`, `not`, `is`, `isn't`, `equals`, `reaches`, `plus`, `minus`, `times`, `multiplied`, `divided`, `mod`, `followed`, `as`, and `using`. The value words `yes`, `no`, `true`, `false`, `nothing`, and `it` can't be names either. Every other word can be, including `count`, `length`, `contents`, and `file`.

## Expressions

Expressions combine values. They're listed here from the **loosest** to the **tightest** binding: `price plus tax times 2` means `price plus (tax times 2)`, because `times` binds tighter than `plus`. Use parentheses to group things differently.

### Logic and joining text

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `X or Y` | Yes if either side is yes | `day is "sat" or day is "sun"` | Available |
| `X and Y` | With two yes/no values: yes if both are yes. If either side is text: joins them, turning a number or nothing into text | `age is at least 13 and age is under 20`<br>`"Hello, " and name` | Available |
| `not X` | The opposite of a yes/no value | `not done` | Available |

`and` is decided while the program runs. If the left side is `no`, the answer is `no` and the right side isn't worked out at all; likewise `or` stops at a `yes` on the left. `and` between yes/no and text, or between two numbers, is a runtime error that explains what to write instead (`"as text"`, or `plus`). `and` followed by the word `call` ends the expression, so `ask ... and call the answer X` works.

### Comparisons

All comparisons are **Available**. Equal and not equal work between any two values (different kinds are never equal).

**Numbers are equal when they're within a relative 1e-12 of each other** (a millionth of a millionth of the larger number), so tiny rounding differences don't matter: `0.1 plus 0.2 is 0.3` is `yes`, even though computers store `0.1 plus 0.2` as 0.30000000000000004. This applies to `is`, `is not`, `isn't`, `equals`, `is equal to`, `=`, `==` and `!=`. The other comparisons agree with it: numbers that count as equal are neither less nor greater, so `0.1 plus 0.2 is at most 0.3` is `yes` too. The tolerance is far smaller than any difference you'd write on purpose: `1 is 1.0000000001` is `no`, and so is `1000000000 is 1000000000.5`. Because it's relative to the larger number, it's tiny near zero (`0 is 0.000000000001` is `no`). The ordering comparisons need two numbers or two texts (texts compare alphabetically, by character code); comparing text with a number is a runtime error. A sentence can only have one comparison in a row: `1 < x < 10` is an error that suggests `1 < x and x < 10`.

| Means | Words | Symbols | Example |
|---|---|---|---|
| equal | `is`, `equals`, `is equal to` | `=`, `==` | `answer is 42` |
| not equal | `is not`, `isn't`, `is not equal to`, `isn't equal to` | `!=` | `name isn't ""` |
| greater | `is greater than`, `is more than`, `is bigger than`, `is above`, `is over` | `>` | `score is above 100` |
| less | `is less than`, `is smaller than`, `is below`, `is under` | `<` | `lives is below 1` |
| greater or equal | `is at least`, `is greater than or equal to`, `is more than or equal to`, `is bigger than or equal to`, `reaches` | `>=` | `points reaches 100` |
| less or equal | `is at most`, `is less than or equal to`, `is smaller than or equal to` | `<=` | `items is at most 10` |

Not available, with the error message suggesting the right one: `is not greater than` (use `is at most`), `is not less than` (use `is at least`), and `is above or equal to` (use `is at least`).

### Joining text with `followed by`

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `X followed by Y` | Turns both sides into text and joins them | `"Total: " followed by total` | Available |

`followed by` binds more loosely than arithmetic, so the maths happens first: `"Sum: " followed by 2 plus 3` gives `Sum: 5`. It binds more tightly than comparisons, so `name followed by "!" is "Ada!"` compares the joined text. (`and` joins text too, and it binds more loosely still: `"Sum: " and 2 plus 3` also gives `Sum: 5`.)

### Arithmetic

All arithmetic is **Available**.

| Means | Words | Symbol | Example |
|---|---|---|---|
| add | `plus` | `+` | `price plus tax` |
| subtract | `minus` | `-` | `total minus discount` |
| multiply | `times`, `multiplied by` | `*` | `width times height` |
| divide | `divided by` | `/` | `total divided by count` |
| remainder | `mod` | `%` | `minutes mod 60` |
| negative | | `-` before a value | `-temperature` |

`times` multiplies only when a value follows it, so a sentence like `repeat 3 times:` keeps its own `times`.

Arithmetic needs numbers on both sides; anything else is a runtime error such as "Line 12: I can't subtract text from a number." To join text, use `and` or `followed by` (which turns numbers into text for you). Dividing or taking the remainder by zero is the error "You divided by zero."

### Conversions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `X as a number` | Turns text into a number | `"42" as a number` | Available |
| `X as text` | Turns a value into text | `total as text` | Available |

`as a number` accepts text like `"42"`, `"-3.5"` or `" 12 "` (spaces around it are fine); anything else, such as `"12abc"`, is a runtime error. Conversions bind tightest of all: `-x as text` means `-(x as text)`, and `"42" as a number plus 1` means `("42" as a number) plus 1`.

### Built-in values

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `length of X` | How many characters a text has | `length of name` | Available |
| `contents of file X` | Everything in a file, as text | `contents of file "notes.txt"` | Available |

These take a single value, so `length of name plus 1` means `(length of name) plus 1`. Write `length of (name plus 1)` for the other meaning.

### Calling a function

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `NAME using X, Y, ...` | Call a function with inputs, separated by commas | `greet using "Ada"`<br>`add using 2, 3` | Parses only (making functions is coming soon) |
| `NAME` | Call a function with no inputs (written like a variable) | `cheer` | Coming soon (today a bare name is always a variable) |

Each input is a single value (a negative number, a conversion, or anything in parentheses is fine). Arithmetic straight after the inputs could belong to the last input or to the whole result, so `double using 21 plus 1` is an error that asks you to write `(double using 21) plus 1` or `double using (21 plus 1)`. Comparisons and `and`/`or` end the inputs: `double using 2 is 4` compares the result with 4.

## Statements

A program is a list of statements: sentences that each start with a word saying what to do. A period or the end of the line ends a statement, and several statements can share a line when periods separate them: `let x be 1. say x.` A statement may start with **`please`**, which is skipped: `please say "hi"` is the same as `say "hi"`.

The words that start statements are only special at the start of a sentence, so `say`, `add`, `file`, or `count` can still be variable names elsewhere.

### Variables

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `let NAME be X` | Create a variable (making the same name twice is an error) | `let total be 0` | Available |
| `let NAME equal X` | Synonym of `let NAME be X` | `let count equal 3` | Available |
| `set NAME to X` | Change a variable's value (it must already exist) | `set total to 10` | Available |
| `change NAME to X` | Synonym of `set` | `change total to 10` | Available |
| `add X to NAME` | `NAME` becomes `NAME plus X` | `add 5 to total` | Available |
| `subtract X from NAME` | `NAME` becomes `NAME minus X` | `subtract 1 from count` | Available |
| `increase NAME by X` | Same as `add X to NAME` | `increase score by 10` | Available |
| `decrease NAME by X` | Same as `subtract X from NAME` | `decrease lives by 1` | Available |
| `multiply NAME by X` | `NAME` becomes `NAME times X` | `multiply price by 2` | Available |
| `divide NAME by X` | `NAME` becomes `NAME divided by X` | `divide total by 4` | Available |

**Names** are single words (see [Values](#values)). `a`, `an`, and `the` can't be names because EasyScript ignores them, so `let a be 5` is an error that explains this. Words that already mean something (`yes`, `no`, `true`, `false`, `nothing`, `it`, and the operator words) can't be names either.

### Output and input

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `say X` | Print a value and a new line | `say "Hello!"` | Available |
| `print X` | Synonym of `say` | `print total` | Available |
| `show X` | Synonym of `say` | `show total` | Available |
| `display X` | Synonym of `say` | `display total` | Available |
| `write X` | Synonym of `say` (unless followed by `to file`; see [Files](#files)) | `write "done"` | Available |
| `ask X and call the answer NAME` | Print a question, then read a line of text typed by the user into `NAME` (empty text if there's no more input). Makes `NAME` if it doesn't exist yet | `ask "What's your name? " and call the answer name` | Available |

### Files

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `write X to file F` | Replace a file's contents with `X` and a new line | `write "hello" to file "notes.txt"` | Available |
| `append X to file F` | Add `X` and a new line to the end of a file | `append "more" to file "notes.txt"` | Available |
| `read file F and call it NAME` | Read a whole file into a variable (without its final new line) | `read file "notes.txt" and call it notes` | Available |
| `file X exists` | A condition: does the file exist? | `if file "notes.txt" exists:` | Coming soon (Phase 1) |

Reading a file inside an expression also works: `contents of file X` ([Built-in values](#built-in-values)).

### Ending the program

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `stop the program` | End the program right away | `stop the program` | Available |

## Decisions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `if CONDITION:` | Run the block only when the condition is yes | `if total is greater than 10:` | Coming soon (Phase 1); the condition itself is Available |
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
