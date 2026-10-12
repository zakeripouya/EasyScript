# Vocabulary

Every word, symbol, and sentence pattern in EasyScript, grouped by category, with an example and its status.

**Status labels:**

- **Available:** works today: it compiles and runs.
- **Coming soon:** planned for the phase shown. Sentence patterns marked this way are **proposals**: the wording may change before they ship, and they don't compile today.

In patterns, `NAME` is a name you choose, `X` and `Y` are any values or expressions, and `CONDITION` is anything that is yes or no. Keywords are case-insensitive. The filler words `the`, `a`, and `an` may appear anywhere and are ignored, so they're left out of the patterns below (except in `is a letter`, `is a digit` and `is a space`, which need their `a`; see [Text](#characters-and-their-codes)).

## Sentences and layout

| Word / symbol | Meaning | Example | Status |
|---|---|---|---|
| `.` or new line | Ends a sentence | `say "hi".` | Available |
| `,`, `then` or `and` at the end of a line | The sentence carries on to the next line (whose indentation doesn't matter) | `let colors be a list of "red",`<br>`    "green" and "blue".` | Available |
| `SENTENCE and SENTENCE` / `... and then SENTENCE` | Two sentences, one after the other. `and` starts a new sentence when a sentence word (`say`, `add`, `stop`, ...) follows it and then something that starts a value (`and stop` and `and skip` need nothing after them); a function's name after `and` is a value, so use `and then` to call it as a sentence. Block sentences (`if`, loops) can't follow `and` | `add 5 to total and say total.`<br>`if done, say "bye" and stop the loop.`<br>`greet with "Ann" and then greet with "Bo".` | Available |
| `:` + indented lines | Opens a block of sentences | `if x is 1:`<br>`    say "one".` | Available |
| indentation | Blocks are indented. A tab counts as 4 spaces. A line must line up with an enclosing block | (see [blocks.es](../examples/lexer/blocks.es)) | Available |
| `the`, `a`, `an` | Filler words; ignored outside text, except that `is a letter`, `is a digit` and `is a space` need their `a` | `set the total to 0.` = `set total to 0.` | Available |
| `#` | Comment to the end of the line | `say 1. # ignored` | Available |
| `note:` | A line starting with `note:` (any capitalization) is a comment | `note: explain the next step` | Available |

## Values

| Form | Meaning | Example | Status |
|---|---|---|---|
| whole numbers | Integers | `42`, `007` | Available |
| ordinals | A whole number with `st`, `nd`, `rd` or `th`, only in `every 3rd item of L`. The ending has to be the right one (`2th` is an error) | `2nd`, `3rd`, `21st`, `7th` | Available |
| decimals | Digits, a point, digits | `19.99` (but `3.` is the number 3 and then a period, and `.5` is an error: write `0.5`) | Available |
| `"..."` | Text. It must end on the line it starts on | `"Hello, world!"` | Available |
| `\n` `\t` `\"` `\\` | Escapes inside text: new line, tab, quote, backslash | `"Line one\nLine two"` | Available |
| `yes`, `true` | The yes value | `done is yes` | Available |
| `no`, `false` | The no value | `ready is false` | Available |
| `nothing` | What a function gives back when it gives back no value. Otherwise it can only be used where a record can be nothing (see [Records](#records)) | `return` (gives back nothing), `set node's next to nothing` | Available |
| `it` | The number of the loop you're in, or the item of a `for each` loop (see [Loops](#it-and-names-in-loops)) | `say it` | Available |
| `a list of X, Y and Z` | A list (see [Lists](#lists)) | `a list of 72, 95 and 88` | Available |
| names | Start with a letter, then letters, digits, `_`, or an apostrophe between letters. A word ending in `'s` is a name and then a possessive (see [Records](#records)), so a name can't end in `'s` | `total`, `player_2`, `isn't`, `rock'n'roll`, `clientName` | Available |
| names of several words | Words separated by spaces, made by `let`, `keep`, `to`, `as`, `call`, `for each` or `every`, and then recognized everywhere (the longest known name wins). They can't contain the operator and value words or `be`, `equal`, `to`, `from`, `by`, `of`, `with`, `in`, `at`, `then`, `that`, `giving` | `late fee`, `hourly rate`, `session number` | Available |
| `( X )` | Grouping: work this out first | `(price plus tax) times 2` | Available |

**How values print:** whole numbers print exactly, without decimals or an exponent, up to 9007199254740992 (2^53; `7`, not `7.0`, and `1000000 times 1000000 times 1000` prints `1000000000000000`), other numbers with at most 10 significant digits (`0.1 plus 0.2` prints `0.3`, `1 divided by 3` prints `0.3333333333`; a number that rounds to a whole one prints as one), yes/no as `yes`/`no`, and lists in English: `72, 95 and 88`.

**Kinds:** every value is a number, text, yes/no, a list, or a record, worked out before the program runs (see [Kinds of values](#kinds-of-values)).

**Words that can't be names:** the operator words `and`, `or`, `not`, `is`, `isn't`, `equals`, `reaches`, `plus`, `minus`, `times`, `multiplied`, `divided`, `mod`, `followed`, `as`, and `using`. The value words `yes`, `no`, `true`, `false`, `nothing`, and `it` can't be names either. Every other word can be, including `count`, `length`, `contents`, and `file`.

## Expressions

Expressions combine values. They're listed here from the **loosest** to the **tightest** binding: `price plus tax times 2` means `price plus (tax times 2)`, because `times` binds tighter than `plus`. Use parentheses to group things differently.

### Logic

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `X or Y` | Yes if either side is yes | `day is "sat" or day is "sun"` | Available |
| `X and Y` | Yes if both are yes (two yes/no values). It doesn't join text: use commas in `say`, brackets in text, or `followed by` | `age is at least 13 and age is under 20` | Available |
| `not X` | The opposite of a yes/no value | `not done` | Available |

If the left side is `no`, the answer is `no` and the right side isn't worked out at all; likewise `or` stops at a `yes` on the left. `and` with text on either side is an error showing the ways to join text, and `and` between two numbers suggests `plus`. `and` followed by the word `call` ends the expression, so `ask ... and call the answer X` works. (`and` also separates the items of a list, `a list of 1, 2 and 3`, and the inputs of a call, `area of 3 and 4`.)

### Joining text: pieces and brackets

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `say A, B, C` | The pieces turned into text and joined, with a space between two pieces unless one of them is empty, the first ends with a space, a new line, `(`, `[` or `$`, or the second starts with a space, a new line or one of `. , ! ? ; : ) ] %`. Works in `say`, `show`, `print`, `display` and `complain`, at the top level only | `say "Hello,", name, "!"` prints `Hello, Ann!` | Available |
| `"... [VALUE] ..."` | Text with a value in brackets: the value is worked out and put in the text exactly where it is (no spaces added); `"[name]!"` is `"" followed by name followed by "!"` | `say "You have [tasks] tasks."` | Available |
| `[[` and `]]` in text | A `[` or `]` in the text itself (a single `]` outside a value is an error) | `"[[draft]]"` is `[draft]` | Available |

Pieces are separated by commas at the top level of the sentence. A comma that belongs to a value keeps its meaning: the items of `a list of 1, 2 and 3`, the inputs after `using`, the fields of `a new dog with name "Rex", age 3`, and a comma followed by `or`, `sorted`, `as`, `rounded` or `starting` (`say the value for K in M, or 0` is one piece). The value in brackets can't contain text in quotes (its quote would end the text); give it a name first.

### Comparisons

All comparisons are **Available**. Equal and not equal work between any two values of the same kind (comparing different kinds is an error); two lists are equal when their items are, in order.

**Numbers are equal when they're within a relative 1e-12 of each other** (a millionth of a millionth of the larger number), so tiny rounding differences don't matter: `0.1 plus 0.2 is 0.3` is `yes`, even though computers store `0.1 plus 0.2` as 0.30000000000000004. This applies to `is`, `is not`, `isn't`, `equals`, `is equal to`, `=`, `==` and `!=`. The other comparisons agree with it: numbers that count as equal are neither less nor greater, so `0.1 plus 0.2 is at most 0.3` is `yes` too. The tolerance is far smaller than any difference you'd write on purpose: `1 is 1.0000000001` is `no`, and so is `1000000000 is 1000000000.5`. Because it's relative to the larger number, it's tiny near zero (`0 is 0.000000000001` is `no`). The ordering comparisons need two numbers or two texts (texts compare alphabetically, by character code); comparing text with a number is an error. A sentence can only have one comparison in a row: `1 < x < 10` is an error that suggests `1 < x and x < 10`.

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
| `L followed by M` | Two lists of the same kind: a new list with L's items, then M's (lists inside are copied). Lists of different kinds are an error | `morning slots followed by evening slots` | Available |

`followed by` binds more loosely than arithmetic, so the maths happens first: `"Sum: " followed by 2 plus 3` gives `Sum: 5`. It binds more tightly than comparisons, so `name followed by "!" is "Ada!"` compares the joined text. 

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

**Math words** (all **Available**):

| Pattern | Means | Example |
|---|---|---|
| `half of X` | `X times 0.5` | `half of price` |
| `twice X` | `X times 2` | `twice price` |
| `N percent of X` | `N times X divided by 100` | `20 percent of price` |
| `the remainder of X divided by Y` | `X mod Y` | `the remainder of 17 divided by 5` |
| `X is infinite` / `X is not infinite` / `X isn't infinite` | Yes when X is infinity or minus infinity | `if total is infinite, say "Too big."` |
| `X divided by Y, rounded down` | Whole-number division: a comma before `rounded` (or `sorted`, or `as`) puts it after the whole value | `minutes divided by 60, rounded down` |

Like `length of`, `half of` and `twice` take a single value (`twice price plus 1` is `(twice price) plus 1`), and so does what comes after `percent of`. They mean the program's own definition instead when it defines one called `half`, `twice` or `remainder` (and `twice with 4` still calls a function kept in a name `twice`).

Arithmetic needs numbers on both sides; anything else is an error before the program runs, such as "Line 12: I can't subtract text from a number." To join text, use `and` or `followed by` (which turns numbers into text for you). Dividing or taking the remainder by zero is the error "You divided by zero."

### Conversions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `X as a number` | Turns text into a number | `"42" as a number` | Available |
| `X as text` | Turns a value into text | `total as text` | Available |
| `X as exact text` | Turns a number into text with every digit it needs to read back as exactly the same number (`as text` keeps 10) | `(1 divided by 3) as exact text` | Available |

`as a number` accepts text like `"42"`, `"-3.5"`, `"1e5"` (a power of ten, also `2.5E-3`) or `" 12 "` (spaces around it are fine); anything else, such as `"12abc"`, is a runtime error. Conversions bind tightest of all: `-x as text` means `-(x as text)`, and `"42" as a number plus 1` means `("42" as a number) plus 1`.

### Built-in values

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `length of X` | How many characters a text has, or how many items a list has | `length of name` | Available |
| `contents of file X` | Everything in a file, as text | `contents of file "notes.txt"` | Available |

These take a single value, so `length of name plus 1` means `(length of name) plus 1`. Write `length of (name plus 1)` for the other meaning.

### Calling a function

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `NAME of X and Y` | Call a function and use the value it gives back | `area of 3 and 4` | Available |
| `NAME with X and Y` | Same as `of` | `area with 3 and 4` | Available |
| `NAME to X` | Same, for a function whose definition introduced its inputs with `to` (and likewise `for`, `from`, `in`) | `add tax to 100`, `busiest artist from names` | Available |
| `NAME using X, Y` | Same, with the inputs separated by commas (or `and`) | `area using 3, 4` | Available |
| `NAME` | Call a function that takes no inputs (written like a variable) | `say pi times 2` | Available |

- **Inputs are separated by `and`.** Each input is a single value: a number, text, a name, a negative number, a conversion, another call in parentheses, or anything in parentheses. So `"Hi " and name` as an input needs parentheses: `greet with ("Hi " and name)`. Every `and` straight after an input starts another input, and the checker then says if the count is wrong.
- **Arithmetic straight after the inputs** could belong to the last input or to the whole result, so `double of 21 plus 1` is an error that asks you to write `(double of 21) plus 1` or `double of (21 plus 1)`. When the last input is already in parentheses it's clear, so `fib of (n minus 1) plus fib of (n minus 2)` adds two calls.
- Comparisons, `or`, and `and call` end the inputs: `area of 3 and 4 is 12` compares the result with 12.

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
| `add X to NAME` | `NAME` becomes `NAME plus X`. When `NAME` is a list, adds X to its end (see [Lists](#lists)) | `add 5 to total` | Available |
| `subtract X from NAME` | `NAME` becomes `NAME minus X` | `subtract 1 from count` | Available |
| `increase NAME by X` | Same as `add X to NAME` | `increase score by 10` | Available |
| `decrease NAME by X` | Same as `subtract X from NAME` | `decrease lives by 1` | Available |
| `multiply NAME by X` | `NAME` becomes `NAME times X` | `multiply price by 2` | Available |
| `divide NAME by X` | `NAME` becomes `NAME divided by X` | `divide total by 4` | Available |

**Names** are a word or several (see [Values](#values)); where a sentence makes or changes a name, the name runs up to the sentence's next word (`set late fee to 30`). `a`, `an`, and `the` can't be names because EasyScript ignores them, so `let a be 5` is an error that explains this. Words that already mean something (`yes`, `no`, `true`, `false`, `nothing`, `it`, and the operator words) can't be names either.

### Constants

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `keep NAME as X` | Make a constant: a name whose value never changes | `keep tax rate as 0.2` | Available |
| `keep NAME at X` | Same as `as` | `keep speed_limit at 50` | Available |
| `keep NAME as a list of X, Y and Z` | A fixed list: a list that can't change | `keep weekdays as a list of "Mon", "Tue" and "Wed"` | Available |
| `keep NAME as a map of K to V, ...` | A fixed map: a map that can't change | `keep prices as a map of "coffee" to 3, "tea" to 2` | Available |

- **Fixed before the program starts:** X may use numbers, text, yes and no, arithmetic, joining (`followed by`, `and`), comparisons, `and`/`or`/`not`, `as text`, `as a number`, `length of`, and constants made above it. Variables, function calls, `it`, `nothing`, and `contents of file` are errors, because their values are only known while the program runs. The compiler works the value out, so a constant costs nothing while the program runs.
- **Visible everywhere,** including inside functions (which can't see variables), and even above the line that makes it.
- **Fixed lists:** a constant can be a list written out (`a list of ...`, `an empty list of ...`) of fixed numbers, text or yes/no values, but not of lists; `length of` a fixed list is fixed too. Anything else worked out from a list is an error. `add`, `insert`, `remove`, `set item ... to`, and `sort` on a fixed list are errors that suggest `let`. Storing it in a variable or giving it to a function gives that variable or function its own copy, which can change; reading it (items, `contains`, loops, `every`, slices) uses the fixed list itself.
- **Fixed maps:** a constant can be a map written out (`a map of ...`, `an empty map from ...`) with fixed text or number keys (each once) and fixed number, text or yes/no values, but not lists or maps; `the number of entries in` a fixed map is fixed too. Anything else worked out from a map is an error. Setting, changing or removing a value is an error that suggests `let`. Storing it in a variable or giving it to a function gives that variable or function its own copy; reading it (lookups, `has`, loops) uses the fixed map itself.
- **Never changes:** `set`, `change`, `add`, `subtract`, `increase`, `decrease`, `multiply`, and `divide` on a constant are errors that suggest `let` instead. A variable, input, loop number, or function can't have a constant's name.
- **Top level only:** not inside an `if`, a loop, or a function.

### Output and input

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `say X` | Print a value and a new line | `say "Hello!"` | Available |
| `print X` | Synonym of `say` | `print total` | Available |
| `show X` | Synonym of `say` | `show total` | Available |
| `display X` | Synonym of `say` | `display total` | Available |
| `write X` | Synonym of `say` (unless followed by `to file`; see [Files](#files)) | `write "done"` | Available |
| `ask X and call the answer NAME` / `ask X and call it NAME` | Print a question, then read a line of text typed by the user into `NAME` (empty text if there's no more input). Makes `NAME` if it doesn't exist yet | `ask "What's your name? " and call the answer name` | Available |

### Files

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `write X to file F` | Replace a file's contents with `X` and a new line | `write "hello" to file "notes.txt"` | Available |
| `append X to file F` | Add `X` and a new line to the end of a file | `append "more" to file "notes.txt"` | Available |
| `read file F and call it NAME` / `read file F and call the answer NAME` | Read a whole file into a variable (without its final new line) | `read file "notes.txt" and call it notes` | Available |
| `file X exists` / `folder X exists` | A condition: is there a file (a folder)? | `if file "notes.txt" exists:` | Available |
| `put X in file F` / `write X in file F` | Like `write X to file F` | `put "hello" in file "notes.txt"` | Available |
| `add X to the end of file F` | Like `append X to file F` | `add "more" to the end of file "notes.txt"` | Available |

More file and folder sentences are in [the standard library](#the-standard-library).

Reading a file inside an expression also works: `contents of file X` ([Built-in values](#built-in-values)).

### Ending the program

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `stop the program` | End the program right away | `stop the program` | Available |
| `stop the program, saying it failed` (the comma is optional) | End it, telling whoever started it that it didn't work (code 1) | `stop the program, saying it failed.` | Available |
| `stop the program with code N` | End it with the code N, a whole number from 0 (it worked) to 255 | `stop the program with code 3.` | Available |
| `complain X` | Write X and a new line to the error output instead of the normal output | `complain "I need a file name."` | Available |

## Decisions

| Pattern | Meaning | Example | Status |
|---|---|---|---|
| `if CONDITION:` + indented lines | Run the indented sentences only when the condition is yes | `if total is greater than 10:` | Available |
| `otherwise if CONDITION:` + indented lines | Checked only when every condition before it was no | `otherwise if total is 10:` | Available |
| `otherwise:` + indented lines | Runs when every condition before it was no | `otherwise:` | Available |
| `else` | Synonym of `otherwise` (also `else if`) | `else:` | Available |
| `if CONDITION, SENTENCE` | One-line form: one simple sentence, on the same line | `if x is 5, say "five".` | Available |
| `if CONDITION then SENTENCE` | Same as the comma form | `if x is 5 then say "five".` | Available |
| `unless CONDITION:` + indented lines | Run the indented sentences only when the condition is **no** (the same as `if not (CONDITION):`). It can't have an `otherwise` | `unless ready:` | Available |
| `unless CONDITION, SENTENCE` / `unless CONDITION then SENTENCE` | One-line form | `unless ready, say "Not yet."` | Available |

Rules:

- **Blocks:** the colon ends the line, and the sentences that belong to it are on the lines below, indented further than the `if` (4 spaces is usual). The block ends where the indentation goes back. Blocks can contain more `if`s.
- **Lining up:** an `otherwise` belongs to the `if` that starts in the same column, just above it. An `otherwise` indented differently, or with no `if` above it, is an error that says so (and which column the `if` is in).
- **Order:** any number of `otherwise if`s, then at most one plain `otherwise`, last.
- **One-line ifs** hold a single simple sentence (not another `if`), and can't have an `otherwise`; the sentence ends at the period or the end of the line, so in `if x is 5, add 1 to x. say x` the `say` always runs.
- **Conditions must be yes or no.** Anything else is an error before the program runs: "Line 3: An "if" needs yes or no to decide, but this is a number." Conditions short-circuit like any `and`/`or`.
- **Names made inside a block exist only inside it.** Using one after the block is an error that explains this; make the name before the `if` to use it afterwards. A name that exists outside can't be made again inside (use `set` to change it).

## Loops

All loops are **Available**. Every loop ends its first line with a colon and puts the sentences to repeat on the indented lines below, like an `if`.

### Counting

| Pattern | Meaning | Example |
|---|---|---|
| `count from A to B:` | Count up from A to B, one at a time. Runs zero times when A is bigger than B (a warning when both are written as numbers); use `count down` to count down | `count from 1 to 10:` |
| `... by S` / `... in steps of S` | Count S at a time. S must be more than zero; the direction still comes from A and B | `count from 0 to 100 by 10:` |
| `... and call each number N` / `... as N` | Name the number (it's called `number` otherwise) | `count from 1 to 3 as row:` |
| `count down from A to B:` | Count down only; runs zero times if A is smaller than B | `count down from 10 to 1:` |
| `go from A to B in steps of S:` | Same as `count from` (the step is optional) | `go from 0 to 1 in steps of 0.25:` |
| `for each N from A to B:` | Same as `count from ... as N` (also takes `by` / `in steps of`) | `for each i from 0 to limit:` |
| `for each N in L:` | Go through a list, item by item (see [Lists](#lists)) | `for each score in scores:` |

- A, B, and S are worked out once, before the first round, and must be numbers.
- The count includes B when it lands on it: `count from 1 to 10 by 4` gives 1, 5, 9. Each number is worked out from the start (no drifting), and a number equal to B lands exactly on it, so `count from 0 to 1 by 0.1` ends at exactly 1.
- In the first line of a count, `as` names the number. To turn a bound into a number, use parentheses: `count from 1 to (limit as a number):`.

### Repeating

| Pattern | Meaning | Example |
|---|---|---|
| `do this N times:` / `repeat N times:` | Repeat N times. N must be a whole number, zero or more | `repeat 3 times:` |
| `while C:` / `as long as C:` / `repeat while C:` | Repeat while C is yes; checked before each round | `while total is less than 100:` |
| `keep doing this until C:` / `repeat until C:` | Repeat until C is yes; checked before each round, so it may run zero times | `repeat until done:` |
| `forever:` | Repeat until `stop the loop` (or `stop the program`) | `forever:` |

Conditions must be yes or no, like an `if`'s.

### Leaving a loop early

| Pattern | Meaning | Example |
|---|---|---|
| `stop the loop` / `stop` / `break` | Leave the innermost loop now | `if number is 5, stop the loop.` |
| `skip this one` / `skip` / `continue` / `move on` | Go straight to the next round of the innermost loop | `if number is 3, skip this one.` |

Using either outside a loop is an error. `stop the program` still ends the whole program.

### `it` and names in loops

- **`it`** is the number of the innermost counting or `repeat ... times` loop (the count, or the round: 1, 2, 3, ...), or the item of the innermost `for each N in L` loop, `every`, or `only`. Inside a `while`, `until`, or `forever` loop it refers to the nearest such loop around it. Anywhere else, `it` is an error.
- **A loop's number, and names made in its body, exist only inside the loop.** Each round starts fresh. A loop's number can't reuse a name that already exists (including an outer loop's `number`), so nested counts need names: `count from 1 to 3 as row:`.

## Functions

All function sentences are **Available**.

| Pattern | Meaning | Example |
|---|---|---|
| `to NAME:` | Define a function with no inputs. NAME is every word before the colon | `to cheer:`, `to make tea:` |
| `to NAME with INPUT and INPUT:` | Define a function with inputs (`of`, `using`, `to`, `for`, `from` and `in` work like `with`; commas work like `and`). NAME is every word before them | `to area with width and height:`, `to add tax to amount:`, `to pause for seconds:`, `to busiest artist from names:`, `to count vowels in word:` |
| `give back X` / `return X` | End the function and hand X back to whoever called it | `give back width times height` |
| `return` | End the function early, handing back nothing (`give back nothing` is the same) | `return` |
| `give back X if CONDITION` / `give back X unless CONDITION` | Leave early with X, but only when the condition is yes (`if`) or no (`unless`); otherwise go on to the next sentence. The same as `if CONDITION, give back X` | `give back 0 if scores is empty.`<br>`give back x unless x is below 0.` |
| `stop here` / `stop here if CONDITION` / `stop here unless CONDITION` | Leave a function that gives back nothing (the same as `return`), always or only when the condition says so. Only inside a function: to end the program, `stop the program` | `stop here if amount is 0.` |
| `INPUT (KIND)` | Say what kind of value an input is (optional) | `to area with width (a number) and height (a number):`<br>`to average of scores (a list of numbers):` |
| `, giving back KIND` | Say what kind of value the function gives back (optional; the comma is optional) | `to roll, giving back a number:` |
| `NAME X and Y` | Call a function as a sentence | `greet "Paris".` |
| `NAME with X and Y` | Same (`of` and `using` too, and `to`, `for`, `from` or `in` for a function defined with that word) | `greet with "Paris".`, `add tax to 100.`, `pause for 3.` |
| `call NAME [with X and Y]` | Same | `call greet with "Paris".` |

Rules:

- **Top level only:** a function is defined at the start of a line, outside every `if`, loop, and other function, with its sentences indented below. It can be used anywhere in the file, even before its definition, and it can call itself (recursion) or other functions.
- **Its own names:** a function sees only its inputs and the names it makes, not the program's other variables (pass those in as inputs). Names made inside it follow the same block rules as `if`s and loops.
- **Giving back:** a function with no `give back X` gives back nothing, and can only be called as a sentence of its own. One that gives back a value always gives back the same kind, and stops with an error if it reaches its end without `give back`. `give back` outside a function is an error.
- **Kinds:** an input's kind comes from how the function uses it and what it's given; what it gives back comes from its `give back`s. When nothing decides a kind, write it (see below).
- **Names:** a function's name is every word before the word that introduces its inputs (`with`, `of`, `using`, `to`, `for`, `from`, `in`; `for` always ends the name), so `to greet someone:` is a function called `greet someone` with no inputs (`to greet with someone:` gives it one). A one-word name can't be a sentence word (like `say` or `count`), but a name of several words can start with one (`add tax`), and then a sentence starting with it calls the function. Functions and variables share names, so `area` can't be both. Each input needs its own name.
- **Checks before running:** calling a function that doesn't exist suggests the closest one; giving it the wrong number of inputs says how many it needs and where it's defined.
- **Deep recursion** stops with "Definitions are using each other too deeply (more than 10000 inside each other)." instead of crashing.

## Functions as values

All of these are **Available**. A function can be kept in a name, a list, a map or a field, given to another function and given back, like any other value. See the [language guide](language-guide.md#15-functions-as-values).

| Pattern | Meaning | Example |
|---|---|---|
| `the function NAME` | A function defined with `to`, as a value (not called). `function` means this only before a function's name | `let twice be the function double` |
| `a function taking X [(KIND)] and Y that gives back E` | A function made where it's used, whose result is E. E is the rest of the value (use parentheses to end it sooner) | `a function taking x that gives back x times 2` |
| `a function that gives back E` | The same with no inputs | `a function that gives back 42` |
| `a function taking X [and Y] [, giving back KIND]:` + indented sentences | A function with a block below it. The colon ends the sentence it's in, so it's always last | `let shout be a function taking word:` |
| `a function:` / `a function, giving back KIND:` | The same with no inputs | `a function giving back a number:` |
| `call F with X and Y` | Calls the function kept in F, as a sentence or a value (`of` and `using` work too). F is a name or any single value (`item 1 of handlers`, `the value for "click" in handlers`) | `call handler with "Ann".` |
| `call F` | Calls a function that takes no inputs | `let n be call next.` |
| `F with X and Y` / `F of X` / `F using X` | The same, in a value, when F is a name | `say twice with 4` |
| `F is G` | Yes if both are the same function value | `if action is the function save` |

Rules:

- **Kinds:** a function's kind is `a function taking KIND and KIND and giving back KIND` (or `a function giving back KIND`, `a function taking KIND`, or just `a function` for one with no inputs and no result). An input that is a function itself goes in parentheses: `a function taking (a function taking a number) and text`. In a list or map: `functions taking text`. Inputs and results are worked out like a `to` function's; when nothing decides one, write it (`a function taking x (a number) ...`).
- **Names from around it:** a function made inside a value sees every name made before it (in the main program, or in the function around it) and can change them. They're **shared, not copied**: a change made by either side is seen by both, even after the function around it has ended. Each `let` makes a new name, so functions made in different rounds of a loop each keep their own. Inside, `it` doesn't mean the loop's number (use the loop's name), and `stop the loop` doesn't leave a loop around it.
- **Not inside `every` or `only`:** make the function first and use its name there.
- **Its inputs** need names that aren't taken around it.
- **Equal:** two function values are equal only when they're the same one (like records).
- **Memory:** a function that's kept in a name it uses (or in a list, record or map that name holds) keeps itself alive; the memory check explains it at the end of the program. A `to` function never does: use one for recursion.

### Built-ins that take a function

All **Available**. F is any single value holding a function; a function's name there means the function itself (`using sum`, not a call of `sum`). Each value makes a new list (or a value); each sentence changes the list itself, so every name for it sees the change. The words naming the items (`number` in `every number in`, `numbers` in `the numbers in`) are only there to read well: inside, `it` is the item.

| Pattern | Meaning | Example |
|---|---|---|
| `F applied to every X in L` (or `of L`) | A new list of F of each item | `double applied to every number in scores` |
| `apply F to every X in L` | Replaces each item of L with F of it (F gives back the kind L holds) | `apply double to every number in scores.` |
| `the Xs in L where C` | A new list of the items for which C is yes (`it` is the item). C is the rest of the value | `the numbers in scores where it is above 50` |
| `the Xs in L that pass F` / `that passes F` | A new list of the items F gives back yes for | `the words in names that pass short` |
| `keep only the Xs in L where C` / `keep only the Xs in L that pass F` | Takes out of L the items that don't pass | `keep only the numbers in scores where it is at least 50.` |
| `L combined using F, starting from S` / `the Xs in L combined using F ...` | S combined with each item in turn: F is given what's combined so far and the next item | `scores combined using sum, starting from 0` |
| `L combined using F` | The same, starting from the first item (an empty list is an error) | `names combined using longer` |
| `L sorted by F` / `L sorted by F biggest first` | A new list ordered by what F gives back for each item (numbers or text; equal ones keep their order) | `names sorted by length biggest first` |
| `sort L by F [, biggest first]` | Puts L itself in that order (`smallest first` is the usual order) | `sort sales by price, biggest first.` |

## Lists

All list words are **Available**. A list holds values of one kind, in order; positions start at 1. See the [language guide](language-guide.md#10-lists).

### Making lists

| Pattern | Meaning | Example |
|---|---|---|
| `a list of X, Y and Z` | A list of these items. Commas separate items until the first `and`; after that only `and` does. Each item is a value up to the next comma or `and`: arithmetic and `followed by` are part of it, comparisons end the list | `a list of 72, 95 and 88`<br>`a list of yes and no` |
| `a list of (L) and (M)` | A list of lists: each inner list in parentheses (without them it's an error) | `a list of (a list of 1 and 2) and (a list of 3)` |
| `an empty list of KINDS` | A list with nothing in it yet; KINDS is `numbers`, `text`, `yes/no values` (or `yes or no values`), or `lists of KINDS` | `an empty list of text`<br>`an empty list of lists of numbers` |
| `a copy of L` | A separate list with the same items (and copies of the lists in it) | `a copy of scores` |
| `L followed by M` | A new list: L's items, then M's (both lists of the same kind) | `morning slots followed by evening slots` |

### Items

| Pattern | Meaning | Example |
|---|---|---|
| `item N of L` / `item N in L` | The item at position N (from 1). N is a number, a name, `it`, or something in parentheses | `item 2 of scores`, `item (n plus 1) of scores` |
| `the first item in L` / `the first item of L` | The first item | `the first item in names` |
| `the last item in L` / `the last item of L` | The last item | `the last item of scores` |

A position that isn't in the list (0, past the end, or not whole) stops the program with an error that says the length.

### Parts of lists

Each of these gives back a **new list** (changing it doesn't change the original; lists inside are copied too).

| Pattern | Meaning | Example |
|---|---|---|
| `items A to B of L` / `items A to B in L` | Items A to B, both included. Both must be positions in the list, and A can't be after B | `items 2 to 4 of scores` |
| `items A to the end of L` / `... to the end in L` | Item A and everything after it | `items 3 to the end of scores` |
| `the first N items in L` / `the first N items of L` | The first N items (all of them if there are fewer) | `the first 3 items in scores` |
| `the last N items in L` / `the last N items of L` | The last N items (all of them if there are fewer) | `the last 2 items of scores` |
| `L without the first item` / `L without the last item` | All but the first or last item (an empty list is an error) | `scores without the first item` |
| `L without item N` | All but the item at position N, the value form of `remove item N from L` (a position that isn't there is an error) | `scores without item 2` |
| `every other item of L` / `every other item in L` | The items at positions 1, 3, 5, ... (starting with the first) | `every other item of scores` |
| `every second item of L` / `every ... item in L` | The items at positions 2, 4, 6, ... (starting with the second): `second`, `third`, `fourth`, `fifth`, `sixth`, `seventh`, `eighth`, `ninth`, `tenth`, or any number written as an ordinal (`3rd`, `12th`, `21st`) | `every third item of scores` |

**`every other item` and `every second item` are different:** of `10, 20, 30, 40 and 50`, every other item is `10, 30 and 50` (the first, then every second one after it), and every second item is `20 and 40` (the second, the fourth, ...). `A`, `B`, and `N` are numbers, names, or something in parentheses, like positions. `without` binds tightly, like `reversed`. `items` is only special in these forms, so it can still be a name. `item`, `first`, and `last` are only special in these forms, so they can still be names (`add item to total`).

### Asking about lists

| Pattern | Meaning | Example |
|---|---|---|
| `the length of L` / `the number of items in L` | How many items | `the number of items in scores` |
| `L contains X` | Yes if some item is equal to X | `scores contains 95` |
| `L does not contain X` / `L doesn't contain X` | The opposite | `names doesn't contain "Zoe"` |
| `L is empty` | Yes if it has no items (also works for text: `name is empty`) | `if scores is empty, say "none"` |
| `L is not empty` / `L isn't empty` | The opposite | `while queue isn't empty:` |
| `L added together` | The total of a list of numbers (0 for an empty list) | `scores added together` |
| `the biggest of L` / `the smallest of L` | The biggest or smallest item of a list of numbers or text (an empty list is an error) | `the biggest of scores` |
| `L reversed` | A new list, last item first (lists inside are copied) | `scores reversed` |
| `L sorted` | A new sorted list, sorted like `sort L` (lists inside are copied); L doesn't change | `scores sorted` |
| `L sorted biggest first` / `L sorted smallest first` | Choose the order | `scores sorted biggest first` |
| `L sorted by their length` / `... by their length longest first` / `... shortest first` | Text by how many characters, lists by how many items | `names sorted by their length longest first` |

`contains`, `is empty` and the other questions are comparisons, so `scores contains 95 and scores is not empty` works. `added together`, `reversed` and `sorted` bind tightly, like `as text`: `scores added together divided by 2` halves the total.

### Changing lists

| Pattern | Meaning | Example |
|---|---|---|
| `add X to the end of L` | Put X after the last item | `add 60 to the end of scores` |
| `add X to L` | The same, when L is a list that doesn't hold numbers; for a list of numbers it's an error asking which you meant | `add "Bob" to names` |
| `insert X at position N of L` / `... in L` | Put X at position N (1 to one past the end); the items from there move along | `insert 7 at position 1 of scores` |
| `remove item N from L` | Take out the item at position N | `remove item 2 from scores` |
| `remove the first item from L` / `remove the last item from L` | Take out the first or last item | `remove the last item from names` |
| `set item N of L to X` / `change item N of L to X` | Replace an item (also `the first item`, `the last item`) | `set item 1 of scores to 100` |
| `sort L` | Numbers smallest first ("not a number" last), text alphabetically (by character code). The list itself changes | `sort scores` |
| `sort L, biggest first` / `sort L, smallest first` | Choose the order (the comma is optional) | `sort scores, biggest first` |
| `sort L by their length` | Text by how many characters, lists by how many items | `sort names by their length` |
| `sort L by their length, longest first` / `..., shortest first` | Choose the order | `sort names by their length, longest first` |

Sorting text, and sorting by length, keeps equal items in the order they were in. `remove`, `insert`, and `sort` start sentences, so they can't be function names.

### Going through lists

| Pattern | Meaning | Example |
|---|---|---|
| `for each N in L:` + indented lines | Run the lines once for each item, with N (and `it`) the item. Goes through the items the list had when it started | `for each score in scores:` |
| `for each N:` + indented lines | The same, going through the list named like N's plural (`score` → `scores`, `box` → `boxes`, `berry` → `berries`). If there's no such name, it's an error asking which list | `for each score:` |
| `each N in L REST` / `every N in L REST` | A new list: REST worked out for each item, as if it started with N. Only at the start of a value (or in parentheses) | `each score in scores plus 5` |
| `L that COMPARISON X` / `L that are [not] empty` | A new list of the items for which the comparison is yes (`only` before it is the same) | `the scores that are at least 70` |
| `L whose FIELD REST` | The records (or values of a kind) whose field makes REST yes, REST read as if it started with the field | `the dogs whose age is over 3` |
| `L where C` | The items for which C is yes; in C, `it` is the item | `the scores where it is below 70` |
| `L that pass F` / `L that passes F` | The items F gives yes for (a function taking an item, giving back yes or no) | `the scores that pass is even` |
| `only L that COMPARISON X` | Same as `L that COMPARISON X` | `only the scores that are at least 70` |
| `only L that are empty` / `... that are not empty` / `... that aren't empty` | The items (text or lists) that are, or aren't, empty | `only the names that aren't empty` |

These filters go after a single value, like `'s`, so a longer list goes in parentheses (`(item 1 of groups) where it is empty`), and `the number of items in (the dogs whose age is over 3)` needs them too. The comparisons after `that` are the usual ones with `are` for `is`: `are`, `are not`, `aren't`, `are equal to`, `are not equal to`, `aren't equal to`, `are greater than`, `are more than`, `are bigger than`, `are above`, `are over`, `are less than`, `are smaller than`, `are below`, `are under`, `are at least`, `are at most`, `are greater than or equal to`, `are more than or equal to`, `are bigger than or equal to`, `are less than or equal to`, and `are smaller than or equal to`.

### Questions and totals

All **Available**. Each goes through the list once, makes no list it doesn't give back, and stops early where it can (`easyscript plan FILE` shows what each became). L is a single value with its filters (`the orders whose amount is over 100`); in a test, `it` is the item.

| Pattern | Meaning | Example |
|---|---|---|
| `how many L are COMPARISON X` / `how many L is ...` / `how many of L are ...` | How many items pass the test (also `are empty`, `start with`, `end with`, `contain`, `don't contain`, ...) | `how many scores are above 70` |
| `how many L there are` / `how many L there is` | How many items | `how many guests there are` |
| `how many L whose FIELD ...` / `how many L where C` | How many items the filter keeps | `how many orders whose amount is over 50` |
| `the number of L` | How many items (`the number of items in L` too) | `the number of orders` |
| `any ITEM TEST` / `every ITEM TEST` / `no ITEM TEST` | Yes if some, every, or no item passes, going through the list named like ITEM's plural; stops as soon as the answer is known. An empty list makes `every` and `no` yes and `any` no | `if any score is above 90:`, `if every order's amount is above 0:` |
| `any ITEM in L TEST` / `no ITEM in L TEST` | The same, with the list named | `if any word in names starts with "Z":` |
| `all L are ...` / `all of L are ...` / `any of L are ...` / `none of L are ...` | The same, with the plural list | `if all scores are at least 50:` |
| `the total of L` / `the total FIELD of L` | All the numbers (or each item's FIELD) added up; 0 for an empty list | `the total amount of orders` |
| `the average of L` / `the average FIELD of L` | The total divided by how many; an empty list is an error | `the average age of dogs` |
| `the median of L` / `the median FIELD of L` | The middle number (halfway between the two middle ones for an even count); an empty list is an error | `the median of scores` |
| `the top N L` / `the lowest N L` | A new list of the N biggest (or smallest) numbers or dates, biggest (or smallest) first; ties in the order they were in. All of them if there are fewer | `the top 3 scores` |
| `the top N L by FIELD` / `the lowest N L by FIELD` | The N items with the biggest (smallest) FIELD | `the top 2 orders by amount` |
| `the N most common L` | A new list of the N items (text or numbers) that come up most, most first; ties in the order they first came up | `the 10 most common words` |
| `the ITEM with the highest FIELD` / `... the lowest FIELD` | The item with the biggest (smallest) FIELD, the first one on a tie; from the list named like ITEM's plural. Also `biggest`, `largest`, `greatest`, `most`, and `smallest`, `fewest`, `least`. A FIELD that's a list counts its items. An empty list is an error | `the customer with the highest total`, `the order with the most items` |
| `the ITEM in L with the highest FIELD` | The same, with the list named | `the dog in pets with the lowest age` |
| `... per FIELD` | After `the total`, `the average`, `the median` or `the number of`: a map from each value of the field to the result for the items with it, in the order each first came up | `the total amount of orders per region` |
| `... per month` / `per year` / `per day` / `per weekday` | Grouped by the item's date (its one date field, or `per month of FIELD`): months as text like "October 2026", days like "Sunday, October 11, 2026", weekdays like "Sunday", years as numbers | `the number of sales per month` |
| `M sorted` / `M sorted biggest first` | A map's copy with its entries in the order of their values (numbers, dates or text) | `the total amount of orders per region, sorted biggest first` |
| `sort M` / `sort M biggest first` | The same order, changing the map itself | `sort totals biggest first` |

Each is written in core terms in the [language guide](language-guide.md#21-questions-and-totals). The words `total`, `average`, `median`, `number`, `top`, `lowest`, `any`, `all`, `none` and `how` keep meaning the program's own definition when it defines one with that name (`total of scores` calls it); written with `the` (`the total of scores`), that's an error asking which was meant.

The plural verbs after `that` and in `how many` are `contain`, `do not contain`, `don't contain`, `start with`, `do not start with`, `don't start with`, `end with`, `do not end with` and `don't end with`: `the words that start with "a"`.

### Sharing

A list is **shared**: `let same be scores` gives the same list another name, and a function given a list can change it. Changes through any name show through all of them. Use `a copy of L` for a separate list. A variable keeps its kind, so a list variable can only be given another list of the same kind. A list kept with `keep` is fixed (see [Constants](#constants)).

## Text

All text words are **Available**. A **character** is one Unicode character (a code point, stored as UTF-8), so `"café"` has 4 characters. Positions start at 1, and taking characters uses exactly the forms and rules of [taking list items](#items), with `character` for `item`. Text never changes: every form here makes new text (or a new list). See the [language guide](language-guide.md#11-text).

### Characters and pieces

| Pattern | Meaning | Example |
|---|---|---|
| `character N of T` / `character N in T` | The character at position N (from 1) | `character 3 of word` |
| `the first character of T` / `the last character in T` | The first or last character | `the first character of name` |
| `characters A to B of T` / `... in T` | Characters A to B, both included | `characters 2 to 5 of word` |
| `characters A to the end of T` | Character A and everything after it | `characters 3 to the end of word` |
| `the first N characters of T` / `the last N characters in T` | The first or last N characters (all of them if there are fewer) | `the first 3 characters of word` |
| `T without the first character` / `T without the last character` | All but the first or last character (empty text is an error) | `word without the first character` |
| `T without character N` | All but the character at position N | `word without character 3` |
| `every other character of T` | The characters at positions 1, 3, 5, ... | `every other character of word` |
| `every second character of T` / `every 3rd character in T` | The characters at positions 2, 4, 6, ... (any ordinal, as for items) | `every second character of word` |
| `the characters of T` | A list of text, one character each | `for each letter in the characters of word:` |
| `the length of T` / `the number of characters in T` | How many characters | `the number of characters in word` |

A position that isn't in the text (0, past the end, or not whole) stops the program with an error that says the length, as for lists.

### Searching

| Pattern | Meaning | Example |
|---|---|---|
| `T contains X` | Yes if X appears in T (empty text appears in everything) | `word contains "ab"` |
| `T does not contain X` / `T doesn't contain X` | The opposite | `name doesn't contain " "` |
| `T starts with X` / `T ends with X` | Yes if T begins or ends with X | `word starts with "un"`, `word ends with "ing"` |
| `T does not start with X` / `T doesn't start with X` | The opposite | `line doesn't start with "#"` |
| `T does not end with X` / `T doesn't end with X` | The opposite | `file does not end with ".txt"` |
| `the position of X in T` | The position of the character where X first starts, or **nothing if it isn't there**: it's `a number or nothing`, so check it before using it as a number. Empty text is found at position 1 | `the position of "x" in word` |
| `the position of X in L` | The same for a list: the position of the first item equal to X, or nothing | `the position of 95 in scores` |

These are comparisons, like `contains` for lists: `word starts with "un" and word ends with "able"` works. `X` and `T` in `the position of X in T` are single values (names, text, numbers, or something in parentheses).

### Splitting and joining

| Pattern | Meaning | Example |
|---|---|---|
| `the words of T` | A list of the pieces of T between spaces (spaces, tabs and line breaks; any number of them, at the start and end too) | `the words of sentence` |
| `the lines of T` | A list of the lines of T. A line break at the very end doesn't start another line, a `\r` before a line break is dropped, and empty text has no lines | `the lines of contents` |
| `T split by S` | A list of the pieces of T between each S, empty pieces included. Empty text gives an empty list; splitting by empty text is an error | `"a,b,,c" split by ","` |
| `L joined with S` | The items of a list of text, numbers or yes/no values as text, with S between each two | `names joined with ", "` |

### Changing text

Each gives back new text.

| Pattern | Meaning | Example |
|---|---|---|
| `T in uppercase` / `T in lowercase` | T with its letters changed to capitals or small letters | `name in uppercase` |
| `T without extra spaces` | T without the spaces, tabs and line breaks at its start and end | `answer without extra spaces` |
| `T with X replaced by Y` | T with every X (from the left, not overlapping) replaced by Y. Replacing empty text is an error | `text with "a" replaced by "b"` |

`in uppercase`, `without extra spaces`, `split by`, `joined with` and `with ... replaced by` bind tightly, like `as text`, and the values after `split by`, `joined with`, `with` and `replaced by` are single values (put anything longer in parentheses). After a name, `with X replaced by` replaces text, unless the name is a function: then `with` starts its inputs.

### Characters and their codes

| Pattern | Meaning | Example |
|---|---|---|
| `C is a letter` / `C is not a letter` / `C isn't a letter` | Yes if C is exactly one character, and it's a letter | `if letter is a letter, add 1 to count` |
| `C is a digit` / `C is not a digit` / `C isn't a digit` | Yes if C is exactly one of `0` to `9` | `character 1 of code is a digit` |
| `C is a space` / `C is not a space` / `C isn't a space` | Yes if C is exactly one space, tab, or line break | `if letter is a space, skip this one` |
| `the code of C` | The Unicode number of the one character in C (an error for empty text or more than one character) | `the code of "A"` (65) |
| `the character with code N` | The character with that Unicode number (a whole number from 1 to 1114111) | `the character with code 233` (`é`) |

**`is a letter` needs the `a`:** `C is letter` (or `C is the letter`) compares C with a variable called `letter`, so `letter`, `digit` and `space` can still be names. Letters, uppercase and lowercase cover English, the accented Latin letters (Latin-1 and Latin Extended-A), Greek and Cyrillic; other characters (Chinese, emoji, ...) aren't letters and don't change case.

### Building text

`set T to T followed by X` (or `set T to T and X`) adds to the end of T. When nothing else holds that text, it grows in place instead of being copied, so building a long text one piece at a time is fast: a million pieces take about 5 milliseconds. Every other name that held the old text still sees the old text.

## Records

All record words are **Available**. A record holds named values, its **fields**, and a kind of record is defined once at the top of the program. Records are shared like lists. See the [language guide](language-guide.md#12-records).

### Defining a kind of record

| Pattern | Meaning | Example |
|---|---|---|
| `NAME has FIELD (KIND) and FIELD (KIND).` | Defines a kind of record in one sentence (the main form): fields separated by commas and `and`, each with its kind in parentheses. Only at the top level | `a dog has a name (text) and an age (a number).` |
| `FIELD` (no kind) | The kind is worked out from how the field is used (`a new dog with name "Rex"` makes `name` text). If nothing decides it, it's an error asking for `(KIND)` | `a cat has a name and a favorite toy.` |
| `FIELD (starting at X)` / `FIELD (KIND, starting at X)` | A field with a starting value (its kind is the value's, unless written) | `an age (starting at 0)`, `some pets (a list of cats, starting at an empty list of cats)` |
| `some FIELD` | `some` before a field's name is left out, like `a` and `an`: `some drinks` is the field `drinks` | `an order has a customer and some drinks.` |
| `NAME is a kind of thing with:` + indented fields | The same, a field per line (a synonym of `has`) | `a dog is a kind of thing with:` |
| `NAME, which is KIND.` | A field, one per line (or sentence). KIND is any kind: `a number`, `text`, `yes or no`, a list, or a kind of record | `a name, which is text.`<br>`a start time, which is a number.` |
| `NAME, which is KIND, starting at X.` | A field with a starting value, used when `new` doesn't give one. X is fixed before the program runs, like a constant: numbers, text, yes or no, constants, fixed lists, or an empty list | `an age, which is a number, starting at 0.` |
| `NAME, which is KIND or nothing.` | A field that can be nothing (any kind but a list or map). It starts as nothing | `a next, which is a node or nothing.`<br>`a nickname, which is text or nothing.` |

A kind of record is written like any other kind: `(a dog)` for an input, `giving back a dog or nothing`, `a list of dogs` (the plural adds `s`, or `es` after `s`, `x`, `z`, `ch` and `sh`; `y` after a consonant becomes `ies`). A field can't always hold a record of its own kind (`a next, which is a node.`), because then no node could ever be made; make it `a node or nothing`.

### Making and using records

| Pattern | Meaning | Example |
|---|---|---|
| `a new NAME with FIELD X and FIELD Y` | A new record. Every field needs a value, unless it has a starting value or can be nothing. Fields are separated by `and` or commas; each value is a value up to the next `and` or comma (arithmetic and `followed by` are part of it). A `new` inside another `new` needs parentheses | `a new dog with name "Rex" and age 3` |
| `a new NAME` | A new record whose fields all start at their starting values | `a new chain` |
| `R's FIELD` | A field of a record. Chains work, and so does `it's` | `rex's name`, `the order's customer's name`, `it's age` |
| `(X)'s FIELD` | A field of a value in parentheses | `(item 1 of dogs)'s name` |
| `the FIELD of R` | The same as `R's FIELD`, with no parentheses needed. R is a single value, so it chains from the right; it works in `set`, `add ... to` and the other change sentences too. If a function has the field's name, it's an error asking which (`rex's size` or `size with rex`); phrases like `the length of` keep their meaning | `the name of rex`, `the name of item 1 of dogs`, `the name of the owner of rex`, `set the age of rex to 4` |
| `set R's FIELD to X` | Changes a field | `set rex's age to 4` |
| `add X to R's FIELD`, `subtract`, `increase`, `decrease`, `multiply`, `divide` | Changes a number field, like a variable (`add X to R's FIELD` adds to the end of a list field that doesn't hold numbers, as for list variables) | `add 1 to rex's age` |
| `R is S` / `R is not S` | Yes if R and S are **the same record** (two records with the same fields are still two records) | `if rex is fido, ...` |
| `a copy of R` | A new record whose fields hold the same values (the text, lists and records in them are shared, not copied) | `let twin be a copy of rex` |
| `say R` | Shows the record: its kind, then each field (text in quotes, lists and records inside it in parentheses, three levels deep) | `a dog with name "Rex" and age 3` |

`'s` binds tightest of all: `rex's age plus 1` is `(rex's age) plus 1`, and `the first item of dogs's name` takes the first item of `dogs's name`; write `(the first item of dogs)'s name` for the name of the first dog. A word ending in `'s` is always a possessive.

**Comparing contents:** `is` asks whether two records are the same one. To compare what's in them, compare their fields (`if rex's name is fido's name and rex's age is fido's age`), or write a method that does, like `to matches, for a dog, with other (a dog), giving back yes or no:`.

### Fields that can be nothing

| Pattern | Meaning | Example |
|---|---|---|
| `X is nothing` / `X is not nothing` / `X isn't nothing` | Yes if X (a value that can be nothing) is nothing, or isn't | `if the node's next is not nothing:` |
| `set R's FIELD to nothing` | Empties a field that can be nothing | `set ann's friend to nothing` |
| `give back nothing` | In a function that gives back something or nothing (or in one that gives back nothing at all, where it's `return`) | `give back nothing` |

A value that can be nothing (a field, a variable made from one, or an input) **can't be used as a record until it's checked**: its fields, and giving it where a record is needed, are compile errors outside a check. Checks:

- inside `if X is not nothing:` (and in `otherwise` after `if X is nothing:`)
- inside `while X is not nothing:` and `repeat until X is nothing:`
- on the right of `X is not nothing and ...` and `X is nothing or ...`
- after `if X is nothing, ...` when that `if` always leaves (with `give back`, `return`, `stop` or `skip`)

X is a name or a chain of fields (`node`, `the node's next`). Setting X (or anything it's part of) ends the check, and a loop that sets X can't rely on a check made before it. The program also checks again while it runs, so if another name for the same record changed it in between, that's a runtime error instead of a crash.

### Methods

| Pattern | Meaning | Example |
|---|---|---|
| `KIND can NAME:` | A method (the main form), at the start of a line: the same as `to NAME, for a KIND:`. Inputs and a result come after the name as in any function | `a dog can bark:`<br>`a dog can fetch with a toy:`<br>`a dog can tell its age, giving back text:` |
| `to NAME, for a KIND:` / `to NAME for a KIND:` | A method: a function whose first input is a record of that kind, called like its kind inside (`the dog`). Its name is every word before `for` | `to bark, for a dog:`<br>`to have a birthday for a dog:` |
| `to NAME, for a KIND, with INPUTS:` | A method with more inputs (also `using`, `of`, `to`, `from`, `in`, and `giving back`) | `to rename, for a dog, with new name:` |
| `NAME R` / `NAME for R` | Calls a method, as a sentence or as a value | `bark rex.`, `have a birthday for rex.` |
| `NAME R with X and Y` | Calls a method with more inputs (or with the word its definition used: `rename rex to "Max".`) | `rename rex with "Max".` |

A method is a function, so `call bark with rex`, `bark of rex` and `describe for rex followed by "!"` (an ambiguity: use parentheses) work as for any function. **Different kinds of record can have methods with the same name** (`to speak, for a dog:` and `to speak, for a cat:`): the call picks the one for its record's kind. A kind can't have two methods with the same name, and a plain function can't share its name.

### Memory

Records are freed when nothing holds them, like text and lists, including what their fields hold. Records that point to each other in a circle (two people who are each other's friend) keep each other alive and are never freed; the memory check (`ES_DEBUG_MEMORY=1`) reports them. See [memory](memory.md#circles).

## Maps

All map words are **Available**. A map finds a value by its key: keys are text or numbers (one kind per map), values any kind (one kind per map), kept in the order keys were first added. See the [language guide](language-guide.md#13-maps).

### Making maps

| Pattern | Meaning | Example |
|---|---|---|
| `a map of K to V, K to V and K to V` | A map with these keys and values. Commas separate entries until the first `and`; after that only `and` does. Each key and value is a value up to `to`, the next comma or `and` | `a map of "coffee" to 3, "tea" to 2` |
| `a map of K to (L)` | A list or map as a value: in parentheses (without them it's an error) | `a map of "odd" to (a list of 1 and 3)` |
| `an empty map from KEYS to VALUES` | A map with nothing in it yet; KEYS is `text` or `numbers`, VALUES is in the plural like a list's items (`numbers`, `text`, `yes/no values`, `lists of text`, `maps from text to numbers`, `dogs`) | `an empty map from text to numbers` |
| `a copy of M` | A separate map with the same keys and values (and copies of the lists and maps in it) | `a copy of prices` |

### Looking up and setting

| Pattern | Meaning | Example |
|---|---|---|
| `the value for K in M` | The value kept for key K. A key that isn't there stops the program with an error naming it | `the value for "tea" in prices` |
| `the value for K in M, or D` | The same, but D (a single value, only worked out when it's needed) when K isn't there | `the value for "Bob" in ages, or 0` |
| `set the value for K in M to X` / `change ...` | Keep X for key K: changes its value, or adds it at the end | `set the value for "Paris" in ages to 19` |

Number keys are the same key when they're the same to 12 significant digits, so numbers that differ only by rounding errors are one key (`0.1 plus 0.2` finds `0.3`). A name straight before `in` is the key: `the value for item in prices`. `value`, `keys`, `values`, `entries` and `map` are only special in these forms, so they can still be names.

### Asking about maps

| Pattern | Meaning | Example |
|---|---|---|
| `M has K` | Yes if K is one of its keys | `if ages has "Paris":` |
| `M does not have K` / `M doesn't have K` | The opposite | `if counts doesn't have word, ...` |
| `the number of entries in M` | How many keys | `the number of entries in ages` |
| `M is empty` / `M is not empty` / `M isn't empty` | Whether it has no keys | `while queue isn't empty:` |
| `the keys of M` | A new list of its keys, in the map's order | `the keys of ages` |
| `the values of M` | A new list of its values, in the map's order | `the values of ages` |

`has` is a comparison, like `contains`. Two maps are equal (`is`) when they have the same keys with equal values, in any order.

### Changing maps

| Pattern | Meaning | Example |
|---|---|---|
| `add X to the value for K in M` (also `subtract ... from`, `increase`/`decrease`/`multiply`/`divide ... by`) | Change a value in place; K has to be there | `add 1 to the value for word in counts` |
| `add X to the end of the value for K in M` | Add to a list kept in a map | `add "Elm" to the end of the value for "Ash" in graph` |
| `remove K from M` | Take key K (and its value) out; K has to be there | `remove "Paris" from ages` |
| `M without K` | A new map with every key but K (lists and maps in it are copied); M doesn't change, and K has to be there. K is a single value | `ages without "Paris"` |

`remove item from M` reads as a list's item with its position missing (an error); write the key another way, like `remove (item) from M`.

### Going through maps

| Pattern | Meaning | Example |
|---|---|---|
| `for each K and V in M:` + indented lines | Run the lines once for each entry, in order, with K (and `it`) the key and V its value. Goes through the entries the map had when it started | `for each name and age in ages:` |

A map needs both names; `for each name in ages:` is an error that suggests `for each name in the keys of ages:`.

### Sharing

A map is **shared** like a list: `let same be ages` gives the same map another name, and a function given a map can change it. Lists and maps in a map belong to it (`a copy of` copies them); text and records in it are shared. A map kept with `keep` is fixed (see [Constants](#constants)).

## Kinds with variants

All of these are **Available**. A kind with variants is a value that's one of several things, each with its own fields. Values never change once made, and are equal when they're the same variant with equal fields. See the [language guide](language-guide.md#14-kinds-with-variants).

| Pattern | Meaning | Example |
|---|---|---|
| `NAME is one of:` + indented variants | Defines a kind with variants called NAME (one or more words). Only at the top level | `a shape is one of:` |
| `NAME is made of FIELD (KIND) and FIELD (KIND).` | A kind with a single variant of the same name: its values are equal when their fields are, never change, and their fields can be used without a check. Good for small values like positions, which `contains` and `the position of` then find by their contents | `a point is made of an x (a number) and a y (a number).` |
| `VARIANT.` | A variant without fields, one per line (or sentence) | `a point.` |
| `VARIANT with FIELD (KIND) and FIELD (KIND).` | A variant with fields, separated by `and` or commas. KIND is any kind, also the kind being defined (`(an expression)`) or `... or nothing` | `a rectangle with a width (a number) and a height (a number).` |
| `a new VARIANT with FIELD X and FIELD Y` / `a new VARIANT` | A new value of the variant's kind. Every field needs a value, unless it can be nothing. As for records, a `new` inside another needs parentheses | `a new circle with radius 3`, `a new point` |
| `check which kind S is:` + indented `when`s | Does the part for the variant S is. S is a value up to `is` (arithmetic and `followed by` are part of it) | `check which kind s is:` |
| `when it's VARIANT:` / `when it is VARIANT:` | The sentences indented below run when S is that variant. Inside, `it` is S, and S itself (if it's a name or a field) is checked to be that variant | `when it's a circle:` |
| `when it's VARIANT or VARIANT:` | For any of several variants | `when it's a rectangle or a square:` |
| `when it's nothing:` | When S (a kind or nothing) is nothing | `when it's nothing:` |
| `otherwise:` / `else:` | Last, for every variant without a `when`. Without it, every variant needs one; with it but nothing left to do, it's a warning | `otherwise:` |
| `its FIELD` | `it's FIELD`: the field of `it` (only before a field's name; elsewhere `its` is an ordinary name) | `its radius` |
| `S is VARIANT` / `S is not VARIANT` / `S isn't VARIANT` | Yes if S is (or isn't) that variant. The `a` or `an` before it is needed: `s is circle` compares with a name | `if s is a circle:` |
| `S's FIELD` | A field of a value **checked to be a variant that has it** (inside `if s is a circle:`, a `when`, after `let s be a new circle ...`); otherwise a compile error | `s's radius` |
| `S is T` | Yes if both are the same variant with equal fields (records in them: the same record) | `s is a new point` |
| `a copy of S` | The same value with its own copies of the lists and maps in its fields | `a copy of tree` |

A kind with variants is written like any other kind: `(a shape)`, `giving back an expression`, `a list of shapes`, `a map from text to shapes`, `(a shape or nothing)`, and methods `to describe, for a shape:`. Fields can't be changed (`set s's radius to 4` is an error): make a new value instead. Every kind, kind of record and variant needs its own name.

## Interfaces

All of these are **Available**. An interface says what a value has to be able to do, as its ways; any kind of record (or kind with variants) whose methods match them fits it, without saying so. See the [language guide](language-guide.md#16-interfaces).

| Pattern | Meaning | Example |
|---|---|---|
| `NAME is anything with:` + indented ways | Defines an interface. NAME is every word before `is anything with` (so it can be `something that can speak`). Only at the top level | `something that can speak is anything with:`, `a speaker is anything with:` |
| `a way to NAME.` | A way with no inputs that gives back nothing | `a way to speak.` |
| `a way to NAME with INPUT (KIND) and INPUT (KIND), giving back KIND.` | A way with inputs (each with its kind, which has to be written) and a result (`of`, `using`, `to`, `for`, `from` and `in` work like `with`) | `a way to greet with name (text), giving back text.` |
| `(something that can speak)`, `a list of things that can speak`, `(a speaker or nothing)` | The interface as a kind. Its plural is `things that can ...` for a name starting with `something` or `anything`, otherwise like a record kind's | `to introduce with pet (something that can speak):` |
| `NAME X ...` | Calls the way on a value of the interface: the method of the value's kind runs | `speak pet`, `greet pet with "Ann"` |

Rules:

- **Fitting:** a kind fits when it has a method for every way with the same name, the same kinds of input, and the same result (or, for a way that gives back an interface, any kind that fits it). Giving a value of a kind that doesn't fit is a compile error that names the missing way.
- **Mixed lists:** `an empty list of things that can speak`, then add dogs, cats and birds. A list written out (`a list of rex and tom`) still holds one kind; the error says which interface both fit.
- **Only ways:** a value of an interface has no fields, and only its ways can be called on it. It can be compared (`is`: the same record, or equal values of a kind with variants), printed, kept in lists, maps and fields, and be `or nothing`.

## Files that use other files

All of these are **Available**. A program can be split into files: the file you run uses others, which only define things. See the [language guide](language-guide.md#17-programs-in-several-files).

| Pattern | Meaning | Example |
|---|---|---|
| `use NAME.` (or `use the NAME file.`) | Reads `NAME.es` from the same folder as the file that says it, so its definitions can be used here. A file is read once however many files use it; files can't use each other in a circle | `use helpers.`, `use the shapes file.` |
| `privately to NAME ...:` / `privately keep NAME as X.` | A function or constant only its own file can use (an optional comma can follow `privately`) | `privately to tidy with text:`, `privately, keep rate at 0.2.` |
| `FILE's NAME` | A function or constant of a file this one uses, by the file's name: needed when two used files define the same name, and allowed anytime | `helpers's double with 3`, `say shapes's unit`, `helpers's greet with "Ann".` |

Rules:

- **What a file sees:** its own definitions, then those of the files it uses directly (not the files they use). Its own come first, so a file can define a name another file also defines. A name two used files both define is only a mistake where it's used without `FILE's`.
- **Only definitions:** a file that's used can only hold `use`, `to`, `keep`, kinds of record, kinds with variants and interfaces. Only the file you run does things.
- **Kinds share one set of names:** kinds of record, kinds with variants, their variants and interfaces are named once across the whole program, and every file sees them.
- **Errors** name the file as well as the line (`Line 4 of helpers.es: ...`), both before and while the program runs.

## The standard library

All of these are **Available**, without `use`. Each is described with its exact meaning in [stdlib.md](stdlib.md).

| Pattern | Gives | Example |
|---|---|---|
| `the square root of X` | a number | `the square root of 16` |
| `X to the power of Y` | a number | `2 to the power of 10` |
| `the absolute value of X` | a number | `the absolute value of -3` |
| `X rounded`, `X rounded up`, `X rounded down` | a number | `2.5 rounded` |
| `X rounded to N decimal places` (`1 decimal place`) | a number | `price rounded to 2 decimal places` |
| `the bigger of X and Y`, `the smaller of X and Y` | a number | `the bigger of 3 and 4` |
| `pi` | a number | `pi times 2` |
| `a random number from A to B` | a whole number | `a random number from 1 to 6` |
| `a random decimal between A and B` | a number | `a random decimal between 0 and 1` |
| `pick one of L`, `a random item of L`, `a random item in L` | an item | `pick one of cards` |
| `L shuffled` | a list | `cards shuffled` |
| `shuffle L.` | (a sentence) | `shuffle cards.` |
| `roll a die` | a whole number | `say roll a die` |
| `use the same random numbers every time.` | (a sentence) | `use the same random numbers every time.` |
| `the time now`, `today`, `tomorrow`, `yesterday` | a date | `say the time now` |
| `N days ago`, `N days from now` (`1 day ago`, `1 day from now`) | a date | `3 days ago` |
| `the year of D` (`month`, `day`, `weekday`, `hour`, `minute`), `D's year` | a number (weekday: text) | `today's weekday` |
| `D is before E`, `D is after E` | yes/no | `yesterday is before today` |
| `D is in this month` (`last month`, `next month`, `this year`, `last year`, `next year`) | yes/no | `paid is in last month` |
| `the number of days between D and E` | a number | `the number of days between today and holiday` |
| `wait N seconds.` (`second`, `milliseconds`, `millisecond`, `minutes`, `minute`) | (a sentence) | `wait 2 seconds.`, `wait a second.` |
| `(a date)` | the kind | `to describe with day (a date):` |
| `dates` (in a list or map kind) | the kind of the items | `an empty list of dates`, `(a map from text to dates)` |
| `the lines of file F` | a list of text | `the lines of file "todo.txt"` |
| `make a new file called F.`, `make a folder called F.`, `make a new folder called F.` | (a sentence) | `make a folder called "photos".` |
| `rename the file F to G.`, `rename the folder F to G.` | (a sentence) | `rename the file "a.txt" to "b.txt".` |
| `copy the file F to G.` | (a sentence) | `copy the file "a.txt" to "b.txt".` |
| `move the file F to the folder D.`, `move the folder F to the folder D.` | (a sentence) | `move the file "a.txt" to the folder "old".` |
| `delete the file F.`, `delete the folder D.`, `delete the folder D and everything in it.` | (a sentence) | `delete the folder "scratch" and everything in it.` |
| `the files in D`, `the folders in D`, `the files in the folder D` | a list of text | `the files in "photos"` |
| `the size of file F` | a number (bytes) | `the size of file "notes.txt"` |
| `the last change of file F` | a date | `the last change of file "log.txt"` |
| `read csv file F and call it NAME.` | (a sentence): a list of maps from text to text | `read csv file "sales.csv" and call it rows.` |
| `save L as csv file F.` | (a sentence) | `save report as csv file "report.csv".` |
| `clear the screen.` | (a sentence) | `clear the screen.` |

`wait`, `shuffle`, `save` and `put` start sentences, so they can't be the one-word name of a function. `make`, `rename`, `copy`, `move`, `delete`, `clear` and `read` only mean these sentences with the words after them (`delete the file`, `clear the screen`), so `to rename, for a dog:` still works. `pi`, `today`, `tomorrow` and `yesterday` are the library's only where the program hasn't made those names.

## System tools

All of these are **Available**, without `use`. Exact meanings are in [stdlib.md](stdlib.md#system-tools).

| Pattern | Gives | Example |
|---|---|---|
| `the words given to the program` | a list of text | `easyscript run greet.es Paris 19` gives `Paris` and `19` |
| `the number of words given to the program` | a number | `if the number of words given to the program is 0:` |
| `word N given to the program` | text | `word 1 given to the program` |
| `the text sent to the program` | text | `echo hi \| ./app`: everything sent (piped) to it, after any `ask` |
| `the lines sent to the program` | a list of text | `for each line in the lines sent to the program:` |
| `the setting called X` | text or nothing | `the setting called "HOME"` (an environment setting) |
| `run P.` / `run P with A and B.` | (a sentence) | `run "git" with "status".`: runs the program P with those words, showing its output; the program stops if it doesn't work |
| `run P with A and B and call the result NAME.` / `... and call it NAME.` | (a sentence): a program result | `run "git" with "status" and call the result r.` |
| `R's output`, `R's error output`, `R's exit code` | text, text, a number | `say r's output` |
| `R worked`, `R didn't work`, `R did not work` | yes/no | `if r worked:` |
| `complain X.` | (a sentence) | `complain "That file is missing."` |
| `stop the program, saying it failed.`, `stop the program with code N.` | (a sentence) | `stop the program with code 2.` |
| `wait for a key and call it NAME.` (or `call the answer NAME`) | (a sentence): text | `wait for a key and call it key.` |
| `the key being pressed` | text or nothing | `let key be the key being pressed` |

Keys are named in plain words: `"up"`, `"down"`, `"left"`, `"right"`, `"enter"`, `"escape"`, `"space"`, `"tab"`, `"backspace"`, `"delete"`, `"home"`, `"end"`, `"page up"`, `"page down"`, `"insert"`; any other key is the character it types (`"a"`, `"A"`, `"7"`, `"?"`).

`word given to the program` without a position is an error that asks which word. The words after `with` are each text, a number (turned into text) or a list of text, separated by `and` or commas. `run` and `complain` only start these sentences where the program hasn't defined a function with that name. `program result` is a kind of record the language defines, only in programs that use it: `(a program result)` works in definitions.

## Kinds of values

Every value has a kind, worked out by the compiler before the program runs (**Available**). Kind mistakes are compile errors with the line and a caret. See the [language guide](language-guide.md#9-types).

| Kind | Written in a definition as | Comes from |
|---|---|---|
| number | `(a number)`, `giving back a number` | number values, arithmetic, `length of`, `as a number`, `it`, counting loops, `the code of` |
| text | `(text)`, `giving back text` | text values, `followed by`, `and` with text, `as text`, `ask`, reading a file, characters, `in uppercase`, `joined with`, ... |
| yes/no | `(yes or no)` or `(yes/no)`, `giving back yes or no` | `yes`/`no`, comparisons, `and`/`or`/`not` between yes/no values |
| nothing | `giving back nothing` | a function with no `give back X` (only as what a function gives back) |
| list | `(a list of numbers)`, `(a list of text)`, `(a list of yes/no values)` or `(a list of yes or no values)`, `(a list of lists of numbers)`, `(a list of dogs)`, `giving back a list of text` | `a list of ...`, `an empty list of ...`, `every`, `only`, `reversed`, `a copy of` |
| record | `(a dog)`, `giving back a dog` | `a new dog`, a field holding one, `a copy of` |
| record or nothing | `(a dog or nothing)`, `giving back a dog or nothing` | a field `which is a dog or nothing`, `nothing` where one goes |
| kind with variants | `(a shape)`, `(a shape or nothing)`, `giving back an expression` | `a new circle ...`, a field holding one, `a copy of` |
| number, text or yes/no that can be nothing | `(a number or nothing)`, `(text or nothing)`, `(yes or no or nothing)`, `giving back a number or nothing` | `the position of` (nothing when it isn't there), a field or input of that kind, `nothing` where one goes. It has to be checked (`if X is not nothing:`, `check which kind X is:`) before it's used as a number, text or yes/no. A list or map can't be nothing |
| map | `(a map from text to numbers)`, `(a map from numbers to lists of text)`, `giving back a map from text to numbers`; in a list or map: `maps from text to numbers` | `a map of ...`, `an empty map from ...`, `a copy of` |
| interface | `(something that can speak)`, `(a speaker or nothing)`; in a list or map: `things that can speak`, `speakers` | any record or value of a kind with variants that fits it, where one is kept |
| date | `(a date)`, `giving back a date`; in a list or map: `dates` (`(a list of dates)`, `(a map from text to dates)`) | `today`, `the time now`, `3 days ago`, `the last change of file F`, a field `which is a date` |
| program result | `(a program result)`; in a list or map: `program results` | `run P ... and call the result NAME` |
| function | `(a function taking a number and giving back text)`, `(a function taking text)`, `(a function giving back a number)`, `(a function)`; in a list or map: `functions taking text` | `the function double`, `a function taking x ...`, a field or input holding one |

## Later phases

Modules (Phase 2), and everything after, will be added here as their designs settle. See the [roadmap](roadmap.md).
