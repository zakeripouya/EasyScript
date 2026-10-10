# Changelog

All notable changes to EasyScript are recorded here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/). EasyScript has no numbered releases yet.

## [Unreleased]

### Added

- **Expression parser** (`src/front/parse_expr.c`) and **syntax tree** (`src/common/ast.h`). `easyscript ast FILE` prints the tree. For now a file is one expression per line (or several, separated by periods).
  - Precedence, loosest first: `or`, `and` (logical or text joining, decided later by the checker; stops before `and call`), `not`, comparisons, `plus`/`minus`/`followed by`, `times`/`multiplied by`/`divided by`/`mod`, unary minus, `as a number`/`as text`, then values.
  - Comparisons in words and symbols: `is`, `is not`, `isn't`, `equals`, `is equal to`, `is greater/more/bigger than`, `is above/over`, `is less/smaller than`, `is below/under`, `is at least`, `is at most`, the "or equal to" forms, `reaches`, `= == != < > <= >=`. Comparisons don't chain.
  - Values: numbers, text, `yes`/`no`/`true`/`false`, `nothing`, `it`, names, parentheses, `NAME using X, Y` calls, `length of X`, and `contents of file X`.
  - Errors with suggestions: phrases missing their last word ("Did you mean "is greater than"?"), `is not greater than`, chained comparisons, ambiguous call arguments, misspelled operators, unmatched parentheses, `.5`. The parser recovers at the next sentence.
- `==` is now a token.
- `examples/parser/` and `tests/ast/`, checked by `make test`.
- **Documentation:** a new README, `docs/` (language guide, vocabulary, errors, architecture, roadmap), `CONTRIBUTING.md`, and this changelog.
- **Examples:** `examples/lexer/` (the new syntax as read by the lexer) and `examples/legacy/` (runnable prototype programs). Every example is checked by `make test`.
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
- The old prototype examples moved to `old/` (reference only) and `examples/legacy/` (tested).

### Fixed

- `FILE` followed by an unknown action no longer uses an uninitialized pointer. It now reports an error.
- Compiler warnings: sign comparisons in the lexer and code generator. The lexer also no longer reads past the end of an empty file.

### Removed

- Committed build outputs (`*.o`, binaries, generated C) and empty scratch files. A `.gitignore` now covers them.

### Known issues

- The new syntax only goes as far as the lexer. Programs still compile through the legacy prototype pipeline (`PRINT #`, `MAKE A VARIABLE`, `FILE ...`).
- In the legacy pipeline, a word or text longer than 255 characters overflows a buffer, `FILE READ` prints an extra blank line, and redeclaring a variable makes later reads use the first declaration.

## 2024-07-06: First prototype

- The original EasyScript prototype: an uppercase command syntax (`MAKE A VARIABLE x ASSIGN 10`, `PRINT # x`, `FILE OPEN/WRITE/READ/CLOSE`) compiled to C, plus an interactive shell.
