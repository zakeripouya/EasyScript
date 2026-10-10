# EasyScript

EasyScript is a programming language with English sentence syntax. The compiler is written in C11. It compiles `.es` source files to C, then builds the generated C with `cc -O2` and runs the result.

## Language syntax (target)

- Statements are lowercase conversational sentences. A **period or a newline** ends a statement.
- A **colon followed by an indented block** opens a block (loops, conditionals, functions). The block ends when indentation returns to the outer level.
- Keywords are **case-insensitive**.
- `the`, `a`, `an` are **filler words**: the lexer/parser ignores them everywhere outside string literals. So they can never be used as identifiers.
- **The old uppercase syntax (`MAKE A VARIABLE x ASSIGN 10`, `PRINT # x`, `FILE OPEN f`) is being replaced.** Don't add features to it or extend it. New work targets the sentence syntax. The README and the `.code` sample files still show the old syntax.

## Parser rules

- The parser must be **deterministic**. Never guess what the user meant, and never use heuristics, "best match", or silent fallbacks.
- If a sentence is ambiguous or doesn't match any grammar rule, **report an error**. Include the line and column, what was expected, and concrete suggestions (for example, the closest valid sentence forms). Don't pick one reading.
- Because a period ends a statement, it can't also be part of an identifier. The current lexer accepts `.` in identifiers so that `myfile.txt` works. That conflicts with the new syntax, so filenames have to become string literals.

## Compiler implementation rules

- **Arena allocator for all compiler memory** (tokens, AST nodes, strings, symbol tables). No per-object `malloc`/`free` and no `strdup`. Free the whole arena once at the end.
- **No fixed-size buffers.** No `char buf[256]`, `VarMap var_map[100]`, `char line[1024]`, and so on. Grow storage dynamically from the arena. The same goes for generated C: don't emit `char x[256]` for strings.
- Write portable C11 (`-std=c11 -Wall -Wextra`). Invoke the C compiler as `cc`, not `gcc`.
- Report compile errors with source positions rather than calling `exit(1)` deep inside the lexer or parser.

## Testing

- **Every feature needs tests in `tests/`.** That includes parser error cases: ambiguous input must produce the expected error and suggestions.
- **Never commit while any test is failing.** Run `make test` before every commit.
- `tests/run/NAME.es` plus `NAME.out`: the program is compiled and run with `easyscript run`. Stdout must match `NAME.out` byte for byte, and the exit status must be 0.
- `tests/errors/NAME.es` plus `NAME.err`: compiled with `easyscript emit`. It must exit non-zero, and stderr must match `NAME.err` exactly.
- CLI behavior (modes, usage errors, no files left in the cwd, temp-dir cleanup) is checked at the bottom of `tests/run.sh`. Add a check there when changing the CLI.
- `tests/run.sh` runs each test in an empty scratch directory and prints PASS/FAIL per test plus a summary. It exits 1 if anything fails.
- Expected files record current behavior, including known quirks (for example, the blank line after `FILE READ` in `run/file_io.out`). When fixing a quirk, update the expected file in the same commit.

## Keeping this file current

**Every change that affects layout, build, or behavior must update the "Current state" and "Build" sections below in the same commit.** Those sections describe the repo as it is, not as it should be.

## Current state of the repo

The code is an early prototype of the old syntax. It doesn't follow the rules above yet:

- `main.c`: the CLI. `run FILE` / `build FILE -o OUT` / `emit FILE`, `help`, and the REPL with no arguments. Unknown commands and bad arguments print usage and exit 2. Each invocation creates one `mkdtemp` directory under `$TMPDIR` (or `/tmp`) holding `program.c`, `program`, and the REPL's `session.es`, and an `atexit` handler removes it, including on compile errors. Nothing is written to the cwd except `build`'s `-o` output. `cc -O2` is invoked with `fork`/`execvp` (no shell). `run` returns the program's exit status (128+signal if it was killed). The REPL appends each line to the session file and reruns all of it through `argv[0] run`. Uses `getline`, so there's no line-length limit.
- `src/arena.{c,h}`: arena allocator (`arena_new`, `arena_alloc` returns zeroed, aligned memory, `arena_strdup`, `arena_sprintf`, `arena_free`). It grows in 64 KiB blocks and gives oversized requests their own block. So far only `main.c` uses it. The lexer, parser and codegen still use `malloc`/`strdup` and need migrating.
- `src/lexer.{c,h}`: case-sensitive uppercase keywords, `#` token, and `malloc(256)` buffers for identifiers, numbers, and strings (overflow risk). Error positions are byte offsets, not line:column.
- `src/parser.{c,h}`: recursive descent over a linked list of statements (`AST.right`). `AST_IF`, `AST_FOR_LOOP`, `AST_FUNCTION`, and `AST_CALL` are declared but not implemented. Errors call `exit(1)`.
- `src/codegen.{c,h}`: global fixed `var_map[100]`, and variables are renamed to `name_N`. Redeclaring a variable makes later reads resolve to the *first* declaration (lookup returns the first match). Generated string variables are `char[256]`.
- `Makefile`: `CC = gcc` for building the compiler, `-Wall -Wextra`, no `-std=c11`. Targets are `all`, `test`, and `clean`. It builds with zero warnings, so keep it that way. `main.c` defines `_XOPEN_SOURCE 700` and `_DARWIN_C_SOURCE` for `mkdtemp`, `getline`, `fork`, and so on.
- `tests/`: 4 run tests and 5 error tests, all written in the old syntax because that's all the compiler accepts today, plus CLI checks in `run.sh`. Rewrite them when the syntax changes.
- `old/`: old-syntax `.code` examples, kept for reference only. `attempt1.code` and `script.code` compile and run. `combined_code.code` fails on `PRINT # 3` (printing a literal number isn't supported), `attempt1_src.code` fails because `FILE WRITE myfile HelloWorld` treats `HelloWorld` as an undefined variable, and `logan.code` uses an unsupported `FOR … END FOR` form.
- `.gitignore` covers build outputs (`*.o`, `easyscript`) and `myfile*` (created when the examples are run from the repo root). Never commit these.

## Build

```sh
make                                  # builds ./easyscript (no warnings expected)
make test                             # builds, then runs tests/run.sh
./easyscript run old/script.code      # compile + run (generated C/binary go to a temp dir)
./easyscript build FILE.es -o OUT     # compile to an executable at OUT
./easyscript emit FILE.es             # print generated C to stdout
./easyscript                          # REPL (old syntax; type EXIT to quit)
make clean
```
