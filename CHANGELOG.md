# Changelog

All notable changes to EasyScript are recorded here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). EasyScript has no numbered releases yet.

## [Unreleased]

### Added

- **Benchmarks:** `benchmarks/` (recursive Fibonacci, a counting loop to 100 million, nested loops, building a large text) in EasyScript, C, Go, and Python, and `make bench`, which prints time and peak memory and checks that every language gives the same result. Results and history are in `docs/performance.md`; the README has a summary.
- **Constants:** `keep NAME as X` (or `at X`), top level only. The compiler works the value out before the program runs (numbers, text, yes/no, arithmetic, joining, comparisons, logic, conversions, `length of`, and earlier constants) and generates a C static initializer, so constants cost nothing at run time. Constants are visible everywhere, including inside functions.
  - Compile-time errors: values only known while the program runs (variables, function calls, `it`, file contents), later constants or itself, changing a constant (with a suggestion to use `let`), name clashes with variables, inputs, loop numbers, or functions, duplicates, constants inside blocks, and mistakes in the value (worded like the runtime's errors).
  - `examples/programs/tax.es`: a tax rate used by a function.
- **Functions** (top level only): `to greet someone:` / `to area with width and height:` (also `of`/`using`), `give back E` / `return E` / `return`, calls as sentences (`greet "Paris".`, `greet with "Paris".`, `call greet with "Paris".`) and as values (`area of 3 and 4`, `area with 3 and 4`; a bare name calls a function with no inputs).
  - Functions can be used before they're defined, and can call themselves. Each sees only its inputs and the names it makes; its locals are hoisted to the top of the generated C function.
  - Compile-time errors with suggestions: unknown functions ("Did you mean "area"? It's defined on line 1."), the wrong number of inputs (pointing to the definition), `give back` outside a function, functions defined inside blocks, duplicate functions or inputs, and variables named like functions.
  - Endless recursion stops with a friendly runtime error instead of crashing.
  - `and` straight after a call's input now starts another input (`greet using name and age` gives two inputs); a parenthesized last input ends the call, so `fib of (n minus 1) plus 1` adds.
  - Examples: `factorial.es`, `fibonacci.es`; the README's bigger example now runs and is tested.
- **Loops:** `count from A to B` (counts down by itself when A is bigger; `by S` / `in steps of S`; `and call each number N` / `as N`, otherwise the number is called `number`), `count down from`, `go from A to B in steps of S`, `for each N from A to B`, `do this N times` / `repeat N times`, `while` / `as long as` / `repeat while`, `keep doing this until` / `repeat until`, and `forever`.
  - `stop the loop` (`stop`, `break`) and `skip this one` (`skip`, `continue`, `move on`); both are errors outside a loop.
  - `it` is the innermost counting loop's number (or the round of a `times` loop); `it` outside one is an error.
  - A loop's number and names made in its body exist only inside the loop; a loop's number can't reuse an existing name.
  - Friendly runtime errors for counts that aren't numbers, steps that aren't more than zero, bad `times` counts, and loop conditions that aren't yes or no.
  - `examples/programs/loops.es`, and `logan.es`: the 2024 prototype's loop example ported to the new syntax.
- **Decisions:** `if CONDITION:` with an indented block, any number of `otherwise if CONDITION:`, and a final `otherwise:` (`else` works too). One-line forms: `if x is 5, say "hi".` and `if x is 5 then say "hi".`
  - An `otherwise` must line up with its `if`; a misplaced one, one with no `if` above it, one after a one-line `if`, or a second plain `otherwise` gets a friendly error saying what's wrong.
  - Conditions must be yes or no; anything else is a runtime error: "Line 3: An "if" needs yes or no to decide, but this is a number."
  - Names made inside a block exist only inside it ("You made "secret" inside the "if" on line 2, so it only exists inside that block.").
- An unexpected indented block is now skipped as a whole after its error, so it can't end an enclosing block early.

### Changed

- **Number equality has a tolerance:** numbers within a relative 1e-12 of each other count as equal (so `1000000000 is 1000000000.5` is still `no`), so `0.1 plus 0.2 is 0.3` is `yes`. `is at most`/`is at least` and the other ordering comparisons agree with it.
- **`followed by` binds more loosely than `plus` and `minus`,** so the maths happens before joining: `"Sum: " followed by 2 plus 3` gives `Sum: 5`. It still binds more tightly than comparisons.
- `main.c` moved to `src/main.c`, and `.vscode/` is no longer tracked.

### Added

- **Programs run.** `run`, `build`, and `emit` now use the new pipeline: lexer → parser → checker → C code generator → `cc -O2`.
  - **C backend** (`src/back/codegen_c.c`): top-level variables become C globals; evaluation is strictly left to right; `and`/`or` short-circuit.
  - **Runtime** (`runtime/es_runtime.h`), embedded in the compiler by `tools/embed.c`, so generated programs are self-contained. Values are nothing, numbers, text, or yes/no. Whole numbers print without decimals. `and` joins text or means logical and, decided while the program runs.
  - **Friendly runtime errors** with line numbers, e.g. "Line 3: I can't subtract text from a number." and "Line 8: You divided by zero."
  - **Checker** (`src/front/check.c`) for names: "I don't know anything called "totl". Did you mean "total"? You made it on line 1.", names used before they're made, names made twice, and functions and `it` (not available yet).
  - `write`/`append` end what they write with a new line, and reading a file leaves off its final new line.
- The interactive shell runs the new syntax and only keeps lines that work.
- `make test` checks that the README's programs match their example files. Run tests can provide input (`NAME.in`) and expect runtime errors (`NAME.err`).
- **Roadmap:** the planned EasyScript Notebook (a chat-style notebook, Jupyter kernel first; see `docs/roadmap.md`).
- **Statement parser** (`src/front/parse_stmt.c`). A program is now a list of statements, each ended by a period or the end of its line, and several can share a line:
  - `let X be E` / `let X equal E`, `set X to E` / `change X to E`
  - `add E to X`, `subtract E from X`, `increase X by E`, `decrease X by E`, `multiply X by E`, `divide X by E`
  - `say E`, with synonyms `print`, `show`, `display`, and plain `write`
  - `ask E and call the answer X`
  - `write E to file F`, `append E to file F`, `read file F and call it X`
  - `stop the program`, and a leading `please` is skipped
  - Errors: unknown first words suggest the closest statement word; `let a be 5` explains that `a`/`an`/`the` are ignored filler; `let yes be 1` explains reserved words; missing or wrong connecting words show a correct example; words after a finished statement are reported.
- The lexer records where it dropped a filler word (`Token.filler`); `easyscript tokens` shows it as `[after "the"]`.
- Suggestions now treat two swapped letters as one typo (`sya` suggests `say`).
- **Expression parser** (`src/front/parse_expr.c`) and **syntax tree** (`src/common/ast.h`). `easyscript ast FILE` prints the tree. For now a file is one expression per line (or several, separated by periods).
  - Precedence, loosest first: `or`, `and` (logical or text joining, decided later by the checker; stops before `and call`), `not`, comparisons, `followed by` (since moved below arithmetic), `plus`/`minus`, `times`/`multiplied by`/`divided by`/`mod`, unary minus, `as a number`/`as text`, then values.
  - Comparisons in words and symbols: `is`, `is not`, `isn't`, `equals`, `is equal to`, `is greater/more/bigger than`, `is above/over`, `is less/smaller than`, `is below/under`, `is at least`, `is at most`, the "or equal to" forms, `reaches`, `= == != < > <= >=`. Comparisons don't chain.
  - Values: numbers, text, `yes`/`no`/`true`/`false`, `nothing`, `it`, names, parentheses, `NAME using X, Y` calls, `length of X`, and `contents of file X`.
  - Errors with suggestions: phrases missing their last word ("Did you mean "is greater than"?"), `is not greater than`, chained comparisons, ambiguous call arguments, misspelled operators, unmatched parentheses, `.5`. The parser recovers at the next sentence.
- `==` is now a token.
- `examples/parser/` and `tests/ast/`, checked by `make test`. They're written as statements now; expression-only files are gone.
- **Documentation:** a new README, `docs/` (language guide, vocabulary, errors, architecture, roadmap), `CONTRIBUTING.md`, and this changelog.
- **Examples:** `examples/programs/` (runnable programs), `examples/lexer/`, and `examples/parser/`. Every example is checked by `make test`.
- **New lexer** (`src/front/lexer.c`) for the sentence syntax:
  - sentences ending in a period or a new line; blocks from a colon and indentation (`INDENT`/`DEDENT`, with a tab counting as 4 spaces)
  - lowercased words, with filler words (`the`, `a`, `an`) dropped and apostrophes allowed inside words
  - whole numbers and decimals (`5.` is 5 and then a period); text with `\n \t \" \\` escapes
  - the symbols `. , : ( ) + - * / % = < > <= >= !=`, and comments with `#` and `note:`
  - friendly errors for bad indentation, unterminated text, unknown escapes, letters stuck to numbers, curly quotes, and stray characters, all reported in one run
- **`easyscript tokens FILE`** to print the lexer's tokens.
- **Diagnostics** (`src/common/diag.c`): errors show `Line N:`, the source line, carets under the problem, and suggestions. Every error from a run is collected and shown together.
- **Foundation** (`src/common/`): arena allocator, growable string builder, dynamic arrays, and edit distance.
- **CLI commands** `run`, `build -o`, and `emit`. Generated C and binaries go in a temporary directory instead of the current folder.
- **Tests:** `make test` runs unit, lexer, program, compile-error, example, and CLI tests with a pass/fail summary. `make debug` and `make test-debug` build and test with AddressSanitizer and UndefinedBehaviorSanitizer.

### Changed

- The build uses `cc`, puts objects in `build/`, and tracks header dependencies automatically. It builds with no warnings.
- The interactive shell keeps its scratch file in a temporary directory and no longer has a line-length limit.
- The old prototype examples moved to `old/` (historical; they no longer compile).

### Fixed

- `FILE` followed by an unknown action no longer uses an uninitialized pointer. It now reports an error.
- Compiler warnings: sign comparisons in the lexer and code generator. The lexer also no longer reads past the end of an empty file.

### Removed

- Committed build outputs (`*.o`, binaries, generated C) and empty scratch files. A `.gitignore` now covers them.

### Removed

- The 2024 prototype pipeline (uppercase `PRINT #` / `MAKE A VARIABLE` syntax): its lexer, parser, and code generator, and the tests and examples written in it. That also removes its known bugs (a buffer overflow on words over 255 characters, the extra blank line after `FILE READ`, and redeclared variables reading the first declaration).

### Changed (build)

- The build uses `-std=c11`, and `make test-debug` no longer turns leak checking off (the compiler has no leaks).

### Known issues

- Text made while a program runs is freed only when it exits, so a long loop that builds text grows (about 20 bytes per round in a simple case).
- `stop` on its own now means "stop the loop"; to end the program, write `stop the program`.
- Functions can't see the program's other variables yet (pass values in as inputs).
- The interactive shell reads one line at a time, so blocks (`if ...:`, loops) can't be typed there; one-line `if`s work. It also reruns the whole session for every line, so earlier output (and questions from `ask`) repeat.

## 2024-07-06: First prototype

- The original EasyScript prototype: an uppercase command syntax (`MAKE A VARIABLE x ASSIGN 10`, `PRINT # x`, `FILE OPEN/WRITE/READ/CLOSE`) compiled to C, plus an interactive shell.
