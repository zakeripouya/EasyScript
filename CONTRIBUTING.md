# Contributing to EasyScript

Thanks for your interest! This page covers how to build and test the compiler, how the code is organized, and what a finished change looks like.

## Build and test

You need `cc` (Clang or GCC), `make`, and macOS or Linux (WSL on Windows).

```bash
make              # build ./easyscript and the unit tests (must produce no warnings)
make test         # run the whole suite
make debug        # build with AddressSanitizer and UBSan into build/debug/
make test-debug   # run the whole suite against the sanitizer build
make clean
```

**Both `make test` and `make test-debug` must pass before every commit.** Never commit with a failing test.

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

Docs must never show syntax as working before it does. Mark planned features "Coming soon".

## Where tests go

| Directory | Format | Checked with |
|---|---|---|
| `tests/unit/*.c` | C functions `static void test_x(TestContext *t)`, registered in the file's `*_tests` function | `CHECK`, `CHECK_SIZE`, `CHECK_STR` |
| `tests/tokens/` | `NAME.es` plus `NAME.out`, and `NAME.err` for error cases | `easyscript tokens` |
| `tests/ast/` | `NAME.es` plus `NAME.out`, and `NAME.err` for error cases | `easyscript ast` |
| `tests/run/` | `NAME.es` plus `NAME.out` | `easyscript run` (stdout, exit status 0) |
| `tests/errors/` | `NAME.es` plus `NAME.err` | `easyscript emit` (must fail; exact stderr) |
| `examples/lexer/` | `NAME.es` plus `NAME.tokens`, and `NAME.err` for error cases | `easyscript tokens` |
| `examples/parser/` | `NAME.es` plus `NAME.ast`, and `NAME.err` for error cases | `easyscript ast` |
| `examples/legacy/` | `NAME.es` plus `NAME.out` | `easyscript run` |
| CLI checks | the end of `tests/run.sh` | |

Expected-output files must match **byte for byte**. When you change a message on purpose, update its expected file in the same commit.

## Language rules

- **Deterministic parsing:** never guess what a sentence means. If it's ambiguous or doesn't match a pattern, report an error with the line, what was expected, and concrete suggestions.
- Keywords are case-insensitive, and `the`/`a`/`an` are ignored. No word is reserved; the parser decides meaning from context.
- Error messages are plain English. See [docs/errors.md](docs/errors.md).
- The legacy uppercase syntax (`MAKE A VARIABLE`, `PRINT #`) is being replaced. Don't extend it.

## Code organization

The compiler is C11 and follows a pipeline: **front end** (lexer, parser, checker) → **middle end** (IR, later) → **back end** (C codegen). Stages only talk through shared data structures (tokens, the AST, and the symbol table). The parser never emits C, and codegen never looks at source text. See [docs/architecture.md](docs/architecture.md).

```
src/main.c        driver: runs the passes in order
src/front/        lexer.c, parse_expr.c, parse_stmt.c, check.c
src/back/         codegen_c.c
src/common/       arena.c, util.c, diag.c, ast.h
runtime/          support code linked into generated programs
```

Rules:

- **Memory:** allocate everything from the arena (`src/common/arena.h`). No `malloc`/`free` per object, no `strdup`, and **no fixed-size buffers**: use `StrBuf` and `Vec`. Generated C mustn't use fixed-size buffers either.
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
