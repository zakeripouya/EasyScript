# Changelog

All notable changes to EasyScript are recorded here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Added

- **English stack traces in runtime errors.** A runtime error now shows the line with carets under the part that failed, like a compile error, and inside functions the chain of calls that led there: `while working out "average" (count was 0)`, `called from "report" on line 5`, `called from line 8: say report`. Inputs are shown for numbers, yes/no and text (cut short after 30 characters); a function calling itself from the same place is collapsed (`...it called itself 4,998 more times...`). The runtime's call-depth counter became a call stack of frames (the function, the call's site, the values it was given); a call costs three more stores, about 3% on the recursive `fib` benchmark and nothing measurable on the others. Generated programs carry their source lines (`es_source`) and a table of sites (`es_sites`) for this.
- **`every other item of scores`:** items 1, 3, 5, ... (`every second item` is 2, 4, 6, ...).

- **Names of several words:** `let late fee be 25.`, `keep hourly rate as 150.`, `to add tax to amount:` (the function "add tax" with the input "amount"), `count from 1 to 3 as session number:`, `for each guest name in guests:`, `ask ... and call the answer visitor name`. A pre-scan finds every name the program makes with `let`, `keep`, `to`, `as`, `call`, `for each` and `every`, and values then match the longest known name. Names can't contain words that already mean something in a sentence (`plus`, `is`, `and`, `of`, `with`, `to`, ...), with an error suggesting another name. Functions defined with `to` before their inputs are called with `to` (`add tax to 100`), and a sentence starting with a function's name calls it even if the name starts with a sentence word. Messages show names as first written (`clientName`, `HOURLY_RATE`); generated C writes the spaces as `_`. The examples now use natural names (`tax rate`, `notes file`, `most tries`, `top names`, `add tax to amount`).

- **List slices:** `items 2 to 4 of scores`, `items 3 to the end of scores`, `the first 3 items in scores`, `the last 2 items in scores`, `scores without the first item` / `without the last item`, and `every second item of scores` (items 2, 4, 6, ...; any ordinal: `third` ... `tenth`, or `3rd`, `12th`, `21st`). Ranges are 1-based and include both ends; the first and last N give back the whole list when it's shorter; positions that aren't in the list are runtime errors naming the position and the length. Slices are new lists, with the lists inside them copied. Ordinals (`2nd`, `3rd`, `21st`) are a new token; a wrong ending (`2th`) is an error that suggests the right one. `grade_statistics.es` uses slices.

- **Lists** (Phase 2, step 15). A list holds values of one kind, in order. See the new [Lists chapter](docs/language-guide.md#10-lists).
  - Making them: `let scores be a list of 72, 95 and 88.`, `let names be an empty list of text.`, lists of lists (`a list of (a list of 1 and 2) and (a list of 3)`), `a copy of scores` (a separate list, copying the lists inside it too).
  - Items, from position 1: `item 2 of scores`, `the first item in scores`, `the last item of scores`. A position that isn't there stops the program with the position and the length: "There's no item 4 in the list: it has 3 items."
  - Changing them: `add 5 to the end of scores`, `insert 7 at position 1 of scores`, `remove item 2 from scores` (or the first or last item), `set item 2 of scores to 100`, and `sort scores` (`, biggest first`; `by their length, longest first`), a stable merge sort with the comparison inlined. `add X to L` adds to the end when L is a list that doesn't hold numbers; with a list of numbers it could also mean adding to each number, so it's an error that shows both ways to say it.
  - Asking: `the length of scores` / `the number of items in scores`, `scores contains 95` (`does not contain`), `scores is empty` (also for text), and `is` between whole lists.
  - Going through them: `for each score in scores:` (with `it` as the item; the loop goes through the items the list had when it started), `every score in scores plus 5`, `only the scores that are at least 70` (with every comparison, `are` for `is`), `scores added together`, `the biggest of scores`, `the smallest of scores`, `scores reversed`.
  - Lists are shared: every name for a list sees its changes, and functions can change the lists they're given. They're reference counted with everything in them, and a list can never hold itself, so nothing leaks. `say scores` prints "72, 95 and 88".
  - Kinds: a list's kind is inferred like any other ("\"scores\" is a list of numbers, so it can't hold text."), and definitions can say `(a list of numbers)`, `(a list of lists of text)`, `giving back a list of text`.
  - `examples/programs/grade_statistics.es` and `top_scores.es`; a `sort` benchmark against C's `qsort`, Go and Python (EasyScript 0.06 s, C 0.08 s, Go 0.08 s, Python 0.31 s); tests for every list form and error, sharing and copying, nesting, and lists in million-round loops under the memory limit.

- **Static types with inference** (Phase 2, step 14). The compiler works out whether every value is a number, text, or yes/no, so most programs need no annotations. See the new [Types chapter](docs/language-guide.md#9-types).
  - Variables take their kind from their `let` and keep it ("\"total\" is a number (made on line 1), but this is text."). `ask` and reading a file give text; `length of` gives a number; constants take the kind of their value.
  - Function inputs and results are inferred from how they're used and called; calling a function with different kinds, giving back two kinds, or using a function that gives back nothing as a value are friendly compile errors. When nothing decides a kind, the error suggests writing it.
  - Optional kinds in definitions: `to area with width (a number) and height (a number), giving back a number:` (`(text)`, `(yes or no)`, `giving back nothing`). The wording and the alternatives considered are in the guide.
  - Kind mistakes that used to stop the program (text in arithmetic, a condition that isn't yes or no, `and` between yes/no and text, comparing a number with text, ...) are now compile errors with the same wording plus the source line and carets. `and` is resolved before the program runs: logical for yes/no, joining for text.
  - A function that gives back a value but reaches its end without `give back` stops with "The function \"sign\" ended without giving anything back."
  - `examples/programs/kinds.es`; tests for inference, every new error, annotations, and conflicting calls; the constant/runtime cross-check test grew from 11 to 23 pairs.

- **Memory management with reference counting** (Phase 2, step 13). Text made while a program runs (joins, numbers turned into text, `ask` answers, file contents) is freed as soon as nothing uses it, so loops run in flat memory: the text benchmark went from 280 MB to 1.8 MB, and a million-round loop that makes text stays under 2 MB. Programs don't change; there's nothing to write. See [docs/memory.md](docs/memory.md).
  - Every heap object has a common header (kind and reference count), ready for lists and records. Text literals and constants are static objects marked immortal.
  - One ownership convention throughout: expressions produce owned values, operations and functions consume their arguments, reading a variable retains, storing releases the old value. Blocks release their names when they end, including through `stop the loop`, `skip this one`, `give back`, and `stop the program`.
  - `examples/programs/million_lines.es`.
- **Memory checks:** `ES_DEBUG_MEMORY=1` makes a program fail (exit 70) if anything is still alive at its end, and every test program now runs this way. `ES_MEMORY_LIMIT=N` fails a program whose live text goes over N bytes; `tests/memory/` runs million-round loops under a limit and checks their peak memory. A control test makes sure the check catches a missing release.
- **Continuous integration:** a GitHub Actions workflow (`.github/workflows/ci.yml`) runs `make`, `make test` and `make test-debug` on every push, on Ubuntu with GCC and on macOS with Clang.
- **Strict C check:** `make test` compiles the C generated for every test program, example and benchmark with `-Wall -Wextra -Wpedantic -Werror -O2`, using the system compiler.
- **Sanitized programs in `make test-debug`:** `ES_SANITIZE=1` makes the compiler build programs with AddressSanitizer and UBSan, so a use after free or double free is caught. On macOS the leak sweep now also covers the memory tests.

### Changed

- **A function's name is every word before `with`, `of`, `using` or `to`.** So `to greet someone:` now defines a function called `greet someone` with no inputs; write `to greet with someone:` for an input. Using `someone` in its body, or calling `greet "Paris"`, gives an error that explains this and shows the fix. The tests and docs use `with`.

- The macOS leak sweep leaves out programs that stop with a runtime error (they exit without clean-up by design, so `leaks` could report their variables depending on register allocation). Numbers shown in runtime error messages are kept reachable (`es_shown`).
- `it` is also the item of a `for each` loop, and `length of` also counts a list's items, so their error notes mention lists. The memory check's messages say "values" and "text and lists" instead of "text". `and` between text and a list joins them, like `followed by`.

- **Generated code uses plain C types:** numbers are `double`, yes/no is `bool`, text is `EsText *`, and a function that gives back nothing is `void`. The tagged value is gone from generated code entirely; only text is reference counted, so no retain or release is generated for numbers or yes/no. Nested loops went from 0.85 s to 0.10 s and counting from 0.07 s to 0.05 s; `mod` uses the integer remainder for whole numbers, and counting loops work out their last round once.
- `nothing` can no longer be used as a value (it's only what a function that gives back no value gives back), and comparing values of different kinds is an error instead of always "no". `give back nothing` is the same as `return`.
- The runtime's first file is now `runtime/es_memory.h` (it was `es_value.h`). Compile errors are listed in source order.

- **Faster function calls:** a value is now 16 bytes instead of 48, so it's passed in registers. Recursive fib(38) went from 0.52 s to 0.24 s.
- The runtime is split into `runtime/es_value.h` (values and memory) and `runtime/es_runtime.h` (operations); `tools/embed.c` joins them. The variables of the main program are now C locals of `main()`.

### Fixed

- A function with no inputs couldn't say what it gives back with a comma (`to roll, giving back a number:`), although the vocabulary showed it.
- The error audit (`tests/tools/error_coverage.py`) could take minutes on messages with several `%s`; each now only matches within a line.

- **Linux:** the compiler didn't link, because `consteval.c` needs the math library; the Makefile now links `-lm` (generated programs already did).
- **GCC:** every generated program printed a `-Wfree-nonheap-object` warning, because GCC couldn't tell that releasing immortal text never reaches `free()`. Freeing now happens in a separate function that is never inlined or specialized (`noipa` on GCC), so the free path is visibly separate from immortal objects. GCC's `-Winfinite-recursion` is turned off in generated programs: a function that only calls itself is reported by EasyScript while the program runs.
- `ES_MEMORY_LIMIT` now stops the program before allocating the text that would go over the limit, not after, so LeakSanitizer doesn't report that text as leaked.
- Text made while a program ran was only freed when it ended, so long loops that built text kept growing (a known issue in 0.1.0).

## [0.1.0] - 2026-10-10

**Phase 1 (core language) is complete.** EasyScript programs written in plain English sentences compile to C and run: variables and constants, numbers, text and yes/no values, arithmetic and comparisons in words or symbols, `if`/`otherwise`, loops, functions with recursion, output, input, and files, with friendly compile-time and runtime errors. Every error message is tested, every accepted phrase is documented, `make test-debug` finds no sanitizer errors or leaks, and `make bench` compares EasyScript with C, Go, and Python. Four planned items moved to Phase 2: reclaiming memory while programs run, type checking, runtime errors that show the source line, and `file ... exists`.

### Added

- **Leak checking everywhere:** `make test-debug` now also runs `tests/leaks.sh` on macOS (every test input, generated program, and shell session under the system `leaks` tool), where AddressSanitizer can't find leaks; Linux uses LeakSanitizer.

- **Larger examples** that use everything together: `fizzbuzz.es`, `guessing_game.es`, `notes.es`, `hanoi.es`, and `receipt.es` (all run by `make test`). A word counter is planned for Phase 2, once there are text tools to split text.
- **A better interactive shell:** earlier output isn't printed again, earlier `ask` questions aren't asked again (their answers are replayed), a line ending in `:` starts a block (if, loop, function) that ends at a blank line, and entries with errors are shown and dropped. `exit`, `quit`, or `stop the program` leaves. Tested by `tests/shell/`.
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
- In the shell, error line numbers count from the start of the session, and every entry reruns the whole session (so slow sessions get slower, and files are written again).

## 2024-07-06: First prototype

- The original EasyScript prototype: an uppercase command syntax (`MAKE A VARIABLE x ASSIGN 10`, `PRINT # x`, `FILE OPEN/WRITE/READ/CLOSE`) compiled to C, plus an interactive shell.
