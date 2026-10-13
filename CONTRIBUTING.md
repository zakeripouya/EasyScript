# Contributing to EasyScript

Thanks for your interest! This page covers how to build and test the compiler, how the code is organized, and what a finished change looks like.

## Build and test

You need `cc` (Clang or GCC), `make`, and macOS or Linux (WSL on Windows).

```bash
make              # build ./easyscript and the unit tests (must produce no warnings)
make test         # run the whole suite
make debug        # build with AddressSanitizer and UBSan into build/debug/
make test-debug   # run the whole suite against the sanitizer build
make test-gcc     # build with GCC and -Werror, then run the whole suite (strict C with GCC too)
make bench        # time the benchmarks (see docs/performance.md)
make bench-check  # make bench, failing if any benchmark got more than 20% slower than docs/performance.md
make bless        # rewrite expected output files from the current output (see below)
make clean
```

**Both `make test` and `make test-debug` must pass before every commit,** and `make test-gcc` before every push. Never commit with a failing test. Before a release, and after any change that could affect speed (the runtime, code generation, a high-level form), run `make bench-check` on a quiet machine: it fails if a benchmark got more than 20% slower than the latest results recorded in `docs/performance.md`.

`make test-gcc` needs GCC: on macOS, `brew install gcc` (the Makefile picks the newest `gcc-N` from Homebrew; override it with `make test-gcc GCC=gcc-15`). It catches GCC-only warnings, in the compiler and in generated programs, before CI's Linux run does. On macOS GCC uses Apple's headers, which include each other more than Linux's do, so a missing `#include` (like `<stdint.h>` for `SIZE_MAX`) still compiles there; `make test`'s `audit/includes` (`tests/tools/includes.py`) catches that instead.

### Continuous integration

`.github/workflows/ci.yml` runs `make`, `make test` and `make test-debug` on every push, on two machines:

| Runner | Compiler | Leak checking in `make test-debug` |
|---|---|---|
| `ubuntu-latest` | GCC (Linux's `cc`) | LeakSanitizer, for the compiler and every generated program |
| `macos-latest` | Clang | `tests/leaks.sh` (the system `leaks` tool) |

Both runners also install Python 3 (for the audits at the end of `tests/run.sh`) and Go (so `make bench` can compare against the Go benchmarks); Linux also installs GNU `time` for the peak-memory checks in `tests/memory/`.

A change has to pass on both, so keep the compiler, the runtime and generated C portable:

- **Link the math library.** Anything that uses `<math.h>` functions needs `-lm` on Linux (`LDLIBS` in the Makefile; `src/main.c` passes it when building programs). macOS links it automatically, so a missing `-lm` only shows on Linux.
- **Generated C must be warning-free under both compilers.** `make test` compiles every generated program with `-Wall -Wextra -Wpedantic -Werror -O2` (the `strict-c/generated-programs` check). GCC and Clang warn about different things, and some GCC warnings only appear after inlining: GCC once warned that `free()` could be called on immortal text, which is why `es_free_object` in `runtime/es_memory.h` is kept out of line. Fix the cause in the runtime rather than turning a warning off; the only warnings generated programs turn off are about the user's program, not the runtime (`-Wunused-function`, `-Winfinite-recursion`).
- **No platform-only APIs** without a fallback, and nothing that depends on macOS's or glibc's behavior. If you only have one platform, push to a branch and let CI check the other.

## What a finished change includes

A feature isn't done until all of these are in the **same commit**:

1. **Code** that follows the rules below.
2. **Tests** in `tests/`, including error cases. For the parser, ambiguous input must produce the expected error and suggestions.
3. **Docs:**
   - the status in `README.md`
   - every new word, pattern, and synonym in `docs/vocabulary.md`
   - the matching chapter in `docs/language-guide.md`
4. **An example** in `examples/` with its expected output, checked by `make test`.
5. **A `CHANGELOG.md` entry** under `[Unreleased]`.
6. **Speed:** if it could make programs slower, `make bench-check` passes.

Docs must never show syntax as working before it does. Mark planned features "Coming soon".

## Where tests go

| Directory | Format | Checked with |
|---|---|---|
| `tests/unit/*.c` | C functions `static void test_x(TestContext *t)`, registered in the file's `*_tests` function | `CHECK`, `CHECK_SIZE`, `CHECK_STR` |
| `tests/tokens/` | `NAME.es` plus `NAME.out`, and `NAME.err` for error cases | `easyscript tokens` |
| `tests/ast/` | `NAME.es` plus `NAME.out`, and `NAME.err` for error cases | `easyscript ast` |
| `tests/data/` | `NAME.es` plus `NAME.out` (the errors as data), and an empty `NAME.err` when there are errors | `easyscript errors` |
| `tests/plan/` | `NAME.es` plus `NAME.out`: what each list question, filter and sort became | `easyscript plan` |
| `tests/run/` | `NAME.es` plus `NAME.out`; optional `NAME.in` (input), `NAME.args` (the words given to the program, separated by spaces) and `NAME.err` (runtime error) | `easyscript run` with `ES_DEBUG_MEMORY=1`: stdout must match; with `.err` it must exit 1 with exactly that stderr, otherwise exit 0 with empty stderr (and nothing alive at the end) |
| `tests/memory/` | `NAME.es` plus `NAME.out`; optional `NAME.in` and `NAME.err` | Built (from `emit`, with `-DES_HOOKS`), then run with `ES_DEBUG_MEMORY=1 ES_MEMORY_LIMIT=100000`: stdout must match, nothing may be alive at the end, and live text may never pass 100,000 bytes (peak memory under 16 MB where measurable). With `.err` it must hit the limit (exit 70) with exactly that stderr |
| `tests/errors/` | `NAME.es` plus `NAME.err` | `easyscript emit` (must fail to compile; exact stderr). Mostly checker errors |
| `tests/warnings/` | `NAME.es` plus `NAME.out` and `NAME.err` | `easyscript run`: must exit 0, with stdout matching `.out` and the warnings (stderr) matching `.err` |
| `examples/lexer/` | `NAME.es` plus `NAME.tokens`, and `NAME.err` for error cases | `easyscript tokens` |
| `examples/parser/` | `NAME.es` plus `NAME.ast`, and `NAME.err` for error cases | `easyscript ast` |
| `tests/modules/` | a folder `NAME/` with `main.es`, the files it uses, `main.out`, and optional `main.in`/`main.err` (a compile or runtime error) | `easyscript run main.es`, checked like `tests/run` |
| `examples/programs/` | like `tests/run` (and folders like `tests/modules`) | `easyscript run` |
| `tests/shell/` | `NAME.in` typed into the shell, plus `NAME.out` and `NAME.err` | `easyscript` with no arguments |
| CLI checks | the end of `tests/run.sh` | |

Two audits also run in `make test` (they need `python3`): every error message in the source must be produced by at least one test (`tests/tools/error_coverage.py`; messages that need the operating system itself to fail are listed there with a reason), and every word and phrase the parser accepts must appear in `docs/vocabulary.md` (`tests/tools/vocab_coverage.py`). A new message needs a test, and a new phrase needs documenting.

Expected-output files must match **byte for byte**. When you change a message on purpose, update its expected file in the same commit.

### Updating expected output: `make bless`

`make bless` reruns every golden test (`tests/run`, `tests/memory`, `tests/errors`, `tests/warnings`, `tests/tokens`, `tests/ast`, and all of `examples/`). It overwrites the expected `.out`/`.tokens`/`.ast`/`.err` files with the current output, then prints `git diff --stat` and any new expected files.
- If a test that used to fail now succeeds, its `.err` is removed.
- It refuses, and exits 1, when no expected file could make a test pass: a `tests/run` program that fails, a `tests/errors` file that now compiles, or a success that writes to stderr.

**Only bless after you've confirmed the new output is intended:**

1. Run `make test` and read every failing diff.
2. Run `make bless` only if every difference is a change you meant to make.
3. Review `git diff` line by line before committing.

Never bless just to make the suite go green. A blessed regression becomes the new "expected" output and stops being caught. Unit tests (`tests/unit`) have no golden files and aren't affected.

- `templates/NAME.es` plus `NAME.out` (and `NAME.in`/`NAME.args`): a starter template for `easyscript new`, run like an example; the CLI checks make sure `easyscript new` writes it exactly. A new template also needs its name in `TEMPLATES` in the Makefile and in `src/new.c`.
- `docs/cookbook/SECTOR/NAME.es` plus `NAME.out` (and `NAME.in` for typed answers): a cookbook recipe, run like an example in an empty folder holding that sector's csv files. Its page `NAME.md` must show the program, the csv files it reads and the output exactly (`tests/tools/cookbook_sync.py`), and the sector's `README.md` must list it.

## Language rules

- **New syntax follows [docs/design-principles.md](docs/design-principles.md),** or is added to its "Known exceptions" with the reason, in the same change. Unsure? Add it to "Open questions" there instead.
- **New phrases start as a proposal** in [docs/proposals/](docs/proposals/README.md) (copy its `TEMPLATE.md`), and are built only once accepted.
- **Don't rely on the order C works out a call's arguments in** (GCC on Linux goes right to left): when two arguments generate code, write plan notes or report errors, work each out into a local first, in written order.
- **English pass:** before adding a phrase, read [docs/english-pass.md](docs/english-pass.md) and follow its "Decided" section and design rules; a change of plan goes in its update log in the same change.
- **Deterministic parsing:** never guess what a sentence means. If it's ambiguous or doesn't match a pattern, report an error with the line, what was expected, and concrete suggestions.
- Keywords are case-insensitive, and `the`/`a`/`an` are ignored. No word is reserved; the parser decides meaning from context.
- Error messages are plain English, in the voice and words of [docs/voice.md](docs/voice.md) (its glossary says which word to use for each idea; `audit/plain-words` checks banned words and the reading level). See [docs/errors.md](docs/errors.md).
- Runtime errors are plain English too, with the line number ("Line 8: You divided by zero."). They're written in the `runtime/` files.
- The runtime is `runtime/es_memory.h` (text and list objects, and memory), `runtime/es_program.h` (starting and ending, output, the call stack, errors), `runtime/es_runtime.h` (operations), `runtime/es_list.h` (lists), `runtime/es_sort.h` (sorting), `runtime/es_text.h` (text tools), `runtime/es_record.h` (records), `runtime/es_map.h` (maps), `runtime/es_kind.h` (kinds with variants) and `runtime/es_closure.h` (functions as values), embedded into the compiler at build time in that order (`tools/embed.c` joins them into `build/gen/es_runtime_embed.c`; never edit that file). Modules written in EasyScript that the compiler adds by itself live in `lib/` (today `lib/charts.es`, embedded as `build/gen/es_charts_embed.c`); they follow the same rules as any program, and must work in every program they're joined to (call their own definitions with `with`, since a program that reads a table makes `the X of Y` a field). Generated programs must compile without warnings under `-Wall -Wextra -Wpedantic`.
- **The checker knows every kind** ([types](docs/language-guide.md#9-types)), so generated code uses `double`, `bool`, `EsText *`, `EsList *`, `EsRecord *` and `EsMap *`; the runtime never checks kinds. A new operation needs its typing rule in `src/front/check_expr.c` (with the runtime's wording, and a `tests/errors/type_*` test for each new message) and its C form in `src/back/codegen_expr.c`. Keep `src/front/consteval.c` in agreement and extend `tests/run/constant_matches_runtime.es`.
- **Generated programs use reference counting for text, lists, records and maps** ([docs/memory.md](docs/memory.md)). Follow the ownership convention exactly: text and list expressions produce owned values, runtime functions and EasyScript functions consume their text and list arguments, reading a variable retains, and storing goes through `es_set`/`es_list_set`. A new runtime function that takes text or a list must release what it's given. Codegen must also keep the order of anything that reads what's in a list next to a call (`cg_has_effects`), since the call could change the list. Every run test runs with `ES_DEBUG_MEMORY=1`, so a missed release fails `make test`; a double release or a missing retain shows up as a use after free in `make test-debug`. A loop that should run in flat memory belongs in `tests/memory/`.

## Code organization

The compiler is C11 and follows a pipeline: **front end** (lexer, parser, checker) → **middle end** (IR, later) → **back end** (C codegen). Stages only talk through shared data structures (tokens, the AST, and the symbol table). The parser never emits C, and codegen never looks at source text (`src/back/source_table.c` copies the source lines into the program for runtime errors, and codegen only passes that text along). See [docs/architecture.md](docs/architecture.md).

```
src/main.c        driver: runs the passes in order
src/front/        lexer.c, parse_expr.c, parse_stmt.c, check.c
src/back/         codegen_c.c
src/common/       arena.c, util.c, diag.c, ast.h
runtime/          support code linked into generated programs
```

Rules:

- **Memory (compiler):** allocate everything from the arena (`src/common/arena.h`). No `malloc`/`free` per object, no `strdup`, and **no fixed-size buffers**: use `StrBuf` and `Vec`. Generated C mustn't use fixed-size buffers either.
- **No global mutable state.** Pass context structs (`Compiler`, `Parser`, `Checker`, `Codegen`) explicitly.
- **Errors** go through `diag` with a source `Span`, and a stage keeps going after an error.
- **Group code by job**, not one file per function. Each module has a small public `.h`, and its private details stay in the `.c`. Prefer opaque structs when other modules don't need the fields.
- Helpers used in only one file are **`static`**.
- Keep files **under about 600 lines**, and split a file that covers two concerns. Keep functions short and doing one thing.
- Match the surrounding style: 4-space indents, `snake_case` functions, `PascalCase` types, and comments that explain *why*.
- Includes are relative to `src/`, for example `#include "common/diag.h"`.

## AI assistants

`CLAUDE.md` holds the same rules in a form for AI coding assistants, plus a "current state" section describing exactly what exists. If you change the layout, build, or behavior, update it too.

## Reporting problems

Open an issue with the `.es` file, the command you ran, what you expected, and what happened.

## Pull requests and the license

**Pull requests aren't accepted yet,** while the language settles toward v0.2.0. Issues, ideas and examples of what you tried to write are very welcome: open an issue.

When pull requests open, contributions are accepted under the same license as the project (inbound = outbound): by contributing, you agree that your contribution is licensed under the [Apache License 2.0](LICENSE) with the runtime exception at its end, like the rest of EasyScript.
