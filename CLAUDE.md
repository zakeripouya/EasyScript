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
- **Never commit while any test is failing.** Run the full suite before every commit.
- Note: `tests/` and a test runner don't exist yet. The first feature work should add them, plus a `make test` target.

## Keeping this file current

**Every change that affects layout, build, or behavior must update the "Current state" and "Build" sections below in the same commit.** Those sections describe the repo as it is, not as it should be.

## Current state of the repo

The code is an early prototype of the old syntax. It doesn't follow the rules above yet:

- `main.c`: reads a file, then lexes, parses, and generates code. It writes `output.c` to the current directory and runs `gcc output.c -o output_program` (no `-O2`). With no arguments it starts a REPL that appends each line to `persistent_code.code` and shells out to `./easyscript`.
- `src/lexer.{c,h}`: case-sensitive uppercase keywords, `#` token, and `malloc(256)` buffers for identifiers, numbers, and strings (overflow risk).
- `src/parser.{c,h}`: recursive descent over a linked list of statements (`AST.right`). `AST_IF`, `AST_FOR_LOOP`, `AST_FUNCTION`, and `AST_CALL` are declared but not implemented. Errors call `exit(1)`.
- `src/codegen.{c,h}`: global fixed `var_map[100]`, and variables are renamed to `name_N`.
- `Makefile`: `CC = gcc`, `-Wall -Wextra`. It has no `-O2`, `-std=c11`, or `test` target. It builds with zero warnings, so keep it that way.
- `old/`: old-syntax `.code` examples, kept for reference only. `attempt1.code` and `script.code` compile and run. `combined_code.code` fails on `PRINT # 3` (printing a literal number isn't supported), `attempt1_src.code` fails because `FILE WRITE myfile HelloWorld` treats `HelloWorld` as an undefined variable, and `logan.code` uses an unsupported `FOR … END FOR` form.
- `.gitignore` covers build outputs (`*.o`, `easyscript`), files that running a program generates (`output.c`, `output_program`, `myfile*`), and the REPL's `persistent_code.code`. Never commit these.
- No `tests/` directory yet.

## Build

```sh
make                          # builds ./easyscript (no warnings expected)
./easyscript old/script.code  # compile + run a file; writes output.c/output_program in the cwd
./easyscript                  # REPL (old syntax; type EXIT to quit)
make clean
```
