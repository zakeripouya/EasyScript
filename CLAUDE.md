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

## Code organization (target)

The compiler follows this pipeline:

- **Front end:** lexer → parser → checker.
- **Middle end:** IR and optimizations (added later).
- **Back end:** C codegen.

Each stage talks to the next only through shared data structures: tokens, the AST, and the symbol table. **The parser never emits C. Codegen never looks at source text.**

Target layout:

```
src/main.c            driver: runs the passes in order
src/front/            lexer.c, parse_expr.c, parse_stmt.c, check.c
src/back/             codegen_c.c
src/common/           arena.c, util.c, diag.c, ast.h
runtime/              runtime support linked into generated programs
```

Rules:

- **Group code by job**, not one file per function. Each module has a small public `.h` and keeps its private details in the `.c`.
- **Keep files under ~600 lines.** Split a file when it passes that, or when it covers two different concerns.
- **Helpers that aren't used elsewhere must be `static`.**
- **No global mutable state.** Pass context structs (`Compiler`, `Parser`, `Checker`, `Codegen`) explicitly.
- **Prefer opaque structs** when other modules don't need the fields.
- **Functions stay short and do one thing.**

Don't move code into this layout separately. Create it as later steps rewrite each stage, and update "Current state" as each piece moves.

## Testing

- **Every feature needs tests in `tests/`.** That includes parser error cases: ambiguous input must produce the expected error and suggestions.
- **Never commit while any test is failing.** Run `make test` and `make test-debug` (sanitizers) before every commit.
- `tests/unit/*.c`: C unit tests for `src/common` (and later, other modules), linked into `build/*/unit_tests`. Each test is `static void test_x(TestContext *t)`, registered with `unit_run` in that file's `*_tests(TestRunner *)` function. Declare that function in `unit.h` and call it from `main` in `unit.c`. Use `CHECK`, `CHECK_SIZE`, and `CHECK_STR`. `t->arena` is fresh for each test.
- `tests/run/NAME.es` plus `NAME.out`: the program is compiled and run with `easyscript run`. Stdout must match `NAME.out` byte for byte, and the exit status must be 0.
- `tests/errors/NAME.es` plus `NAME.err`: compiled with `easyscript emit`. It must exit non-zero, and stderr must match `NAME.err` exactly.
- CLI behavior (modes, usage errors, no files left in the cwd, temp-dir cleanup) is checked at the bottom of `tests/run.sh`. Add a check there when changing the CLI.
- `tests/run.sh` runs the unit binary, then each program test in an empty scratch directory, then the CLI checks. It prints PASS/FAIL per test plus one summary and exits 1 if anything fails. The `ES` and `UNIT` env vars choose which binaries are tested.
- Expected files record current behavior, including known quirks (for example, the blank line after `FILE READ` in `run/file_io.out`). When fixing a quirk, update the expected file in the same commit.

## Keeping this file current

**Every change that affects layout, build, or behavior must update the "Current state" and "Build" sections below in the same commit.** Those sections describe the repo as it is, not as it should be.

## Current state of the repo

The code is an early prototype of the old syntax. It doesn't follow the rules above yet:

- **Layout:** partly migrated. `src/common/` exists (arena, util, diag). `main.c` is still at the repo root, and the old `lexer`, `parser`, and `codegen` sit flat in `src/`. There's no `front/`, `back/`, `runtime/`, checker, or `ast.h` yet. The AST is defined in `parser.h`.
- **Stage boundaries:** there's no checker, so codegen does name resolution itself (looking up undefined variables and files). `main.c` emits the C prologue and epilogue (`#include`, `int main() {`) rather than leaving that to codegen.
- **Error reporting:** `diag` exists but nothing uses it yet. The old lexer, parser, and codegen still print one `Error: ...` line and `exit(1)`. The `tests/errors/*.err` files expect that old format, so update them when each stage switches to `diag`.
- **Global mutable state:** `src/codegen.c` has `var_map[100]`, `var_map_index`, and `var_counter`. `main.c` has file-scope `arena` and `temp_*` path pointers, which exist because the `atexit` cleanup can't take arguments. Both should move into context structs. For `main.c`, that requires errors to return to the driver instead of calling `exit(1)`, so cleanup can run explicitly. (A hard crash such as a sanitizer abort skips `atexit` and leaves the temp dir behind.)
- **Non-static helpers:** `lexer.c`'s `get_id` and `codegen.c`'s `sanitize_filename`/`get_var_id`/`is_var_string`/`add_var` are extern without being in a header. The `Lexer`, `Parser`, and `AST` structs are all public.

- `main.c`: the CLI. `run FILE` / `build FILE -o OUT` / `emit FILE`, `help`, and the REPL with no arguments. Unknown commands and bad arguments print usage and exit 2. Each invocation creates one `mkdtemp` directory under `$TMPDIR` (or `/tmp`) holding `program.c`, `program`, and the REPL's `session.es`, and an `atexit` handler removes it, including on compile errors. Nothing is written to the cwd except `build`'s `-o` output. `cc -O2` is invoked with `fork`/`execvp` (no shell). `run` returns the program's exit status (128+signal if it was killed). The REPL appends each line to the session file and reruns all of it through `argv[0] run`. Uses `getline`, so there's no line-length limit.
- `src/common/arena.{c,h}`: arena allocator (`arena_new`, `arena_alloc` returns zeroed, aligned memory, `arena_strdup`, `arena_sprintf`/`arena_vsprintf`, `arena_free`). It grows in 64 KiB blocks and gives oversized requests their own block. Used by `main.c`, `util`, and `diag`. The lexer, parser and codegen still use `malloc`/`strdup` and need migrating.
- `src/common/util.{c,h}`: `StrBuf` string builder (`sb_init`, `sb_append`, `sb_append_n`, `sb_append_char`, `sb_append_repeat`, `sb_appendf`, `sb_vappendf`). It grows from the arena and `data` is always NUL-terminated. Also `Vec(T)`, `vec_push(arena, &v, x)`, and `vec_last(&v)` dynamic arrays (a zero-initialized `Vec` is empty), `edit_distance` (byte-wise, case-sensitive Levenshtein), and `PRINTF_LIKE` for format checking.
- `src/common/diag.{c,h}`: opaque `Diag` context made by `diag_new(arena, source, len)`. Report with `diag_error(d, span, fmt, ...)`, then `diag_note(d, fmt, ...)` for help lines attached to the latest error. `Span` is a byte `{offset, length}`. `diag_line` gives the 1-based line, `diag_count` the number of errors, and `diag_render` (to a `StrBuf`) / `diag_print` (to a `FILE *`) output every error in report order, separated by blank lines, in this format: `Line N: message`, then the source line indented 4 spaces, then carets under the span, then the notes. Carets are clamped to the first line of the span, with at least one. Tabs are copied into the padding, UTF-8 is counted per character, and a trailing `\r` is dropped. "Did you mean" suggestions are written by the caller as notes.
- `src/lexer.{c,h}`: case-sensitive uppercase keywords, `#` token, and `malloc(256)` buffers for identifiers, numbers, and strings. A token longer than 255 characters overflows the heap. ASan confirms this in `get_id`, and the release build corrupts memory silently. Error positions are byte offsets, not line:column.
- `src/parser.{c,h}`: recursive descent over a linked list of statements (`AST.right`). `AST_IF`, `AST_FOR_LOOP`, `AST_FUNCTION`, and `AST_CALL` are declared but not implemented. Errors call `exit(1)`.
- `src/codegen.{c,h}`: global fixed `var_map[100]`, and variables are renamed to `name_N`. Redeclaring a variable makes later reads resolve to the *first* declaration (lookup returns the first match). Generated string variables are `char[256]`.
- `Makefile`: `CC = cc`, `-Wall -Wextra -Isrc`, no `-std=c11` yet. The old lexer and parser use `strdup`, which strict C11 hides on glibc, so add the flag once they're rewritten. The new `src/common` and `tests/unit` code is already clean under `-std=c11 -Wpedantic`. Includes are written relative to `src/`, for example `#include "common/diag.h"`. Objects go under `build/release/` (`-O2`) or `build/debug/` (`-g -O1 -fsanitize=address,undefined -fno-sanitize-recover=all`), mirroring the source path. Header dependencies come from `-MMD`. `./easyscript` is linked at the repo root. Add new `.c` files to `COMMON_SRC`/`COMPILER_SRC`; files in `tests/unit/` are picked up by wildcard. It builds with zero warnings in both modes, so keep it that way. `main.c` defines `_XOPEN_SOURCE 700` and `_DARWIN_C_SOURCE` for `mkdtemp`, `getline`, `fork`, and so on.
- `tests/`: 22 unit tests (arena, StrBuf, Vec, edit distance, diag formatting), 4 run tests and 5 error tests, plus CLI checks in `run.sh`. The run and error tests are written in the old syntax because that's all the compiler accepts today. Rewrite them when the syntax changes.
- `old/`: old-syntax `.code` examples, kept for reference only. `attempt1.code` and `script.code` compile and run. `combined_code.code` fails on `PRINT # 3` (printing a literal number isn't supported), `attempt1_src.code` fails because `FILE WRITE myfile HelloWorld` treats `HelloWorld` as an undefined variable, and `logan.code` uses an unsupported `FOR … END FOR` form.
- `.gitignore` covers build outputs (`build/`, `easyscript`, stray `*.o`/`*.d`) and `myfile*` (created when the examples are run from the repo root). Never commit these.

## Build

```sh
make                                  # builds ./easyscript and build/release/unit_tests (no warnings expected)
make test                             # builds, then runs tests/run.sh (unit + program + error + CLI tests)
make debug                            # ASan/UBSan build in build/debug/
make test-debug                       # full suite against the sanitizer build (leak checks off for now)
./easyscript run old/script.code      # compile + run (generated C/binary go to a temp dir)
./easyscript build FILE.es -o OUT     # compile to an executable at OUT
./easyscript emit FILE.es             # print generated C to stdout
./easyscript                          # REPL (old syntax; type EXIT to quit)
make clean
```
