# Compiler architecture

The EasyScript compiler is written in C11 and has no dependencies beyond the C standard library and POSIX. It compiles `.es` files to C and builds the C with the system compiler (`cc -O2`).

## The pipeline

```
source.es → lexer → parser → checker → C codegen → cc -O2 → program
            tokens   AST      symbols   generated C
            \_______ front end _______/  \_ back end _/
```

Every stage reports errors to one `Diag` context, and the driver prints them all at the end.

| Stage | Input → output | Job |
|---|---|---|
| **Lexer** | source text → tokens | Splits text into words, numbers, text, symbols, and `NEWLINE`/`INDENT`/`DEDENT`. It drops comments and filler words and lowercases words. It doesn't know what any word means. |
| **Parser** | tokens → AST | Recognizes sentence patterns, deterministically. An ambiguous or unknown sentence is an error with suggestions; it never guesses. |
| **Checker** | AST → diagnostics | Today: resolves names (made before use, made once) with "did you mean" suggestions, and rejects what has no meaning yet (calls, `it`). Later: types. It runs only when parsing succeeded. |
| **Middle end** | (added later) | An intermediate representation and optimizations. |
| **C codegen** | checked AST → C source | Writes a self-contained C program: the embedded runtime, a static object per text literal, the functions, and `main()`, whose variables are C locals. It inserts every retain and release (see [Memory](#memory-in-generated-programs)). It never looks at source text and reports no errors. |
| **Runtime** | (inside the generated program) | `runtime/es_value.h` and `runtime/es_runtime.h`: the 16-byte tagged `EsValue` (nothing, number, text, yes/no), reference-counted heap objects, every operation, and friendly runtime errors (`Line N: ...`). |
| **Driver** | | `src/main.c`: runs the passes in order, prints diagnostics, invokes `cc`, and cleans up temp files. |

**Boundaries:** each stage only talks to the next through shared data structures (tokens, the AST, and the symbol table). The parser never emits C, and codegen never reads source text. Every stage reports problems through one `Diag` context, and the driver prints them all at the end.

## Folder layout

The target layout:

```
src/main.c            driver
src/front/            lexer.c, parse_expr.c, parse_stmt.c, check.c
src/back/             codegen_c.c
src/common/           arena.c, util.c, diag.c, ast.h
runtime/              support code linked into generated programs
tests/                unit, lexer, program, error, and CLI tests
examples/             small programs that back the docs (all tested)
docs/                 this documentation
```

What exists today:

| Path | Status |
|---|---|
| `src/common/arena.{c,h}` | Done. Arena allocator: all compiler memory comes from here and is freed at once. |
| `src/common/util.{c,h}` | Done. Growable string builder (`StrBuf`), dynamic arrays (`Vec`), edit distance. |
| `src/common/diag.{c,h}` | Done. Collects errors with source spans and notes, and prints them in the [standard format](errors.md). |
| `src/front/lexer.{c,h}` | Done. The new lexer, available through `easyscript tokens`. |
| `src/common/ast.{h,c}` | Done for expressions and simple statements. Tagged-union `Expr` and `Stmt` nodes from the arena, each with a `SourcePos` (span, line, column); statements also keep the verb as written. Blocks are lists of statements. Also includes the outline printer used by `easyscript ast`. |
| `src/front/parse.h`, `parse_internal.h` | The parser's public API (`parse_program`), and the internal `Parser` context and helpers shared by the parser files. |
| `src/front/parse_util.c` | Token helpers, phrase matching (multi-word operators, with "Did you mean" for a missing last word), and error helpers. |
| `src/front/parse_expr.c` | Done. Recursive-descent expression parser. |
| `src/front/parse_stmt.c` | A table of statement forms, names (with filler and reserved-word errors), and the statement loop (`parse_statements`), used for the whole program and for each block. |
| `src/front/parse_if.c` | `if` / `otherwise if` / `otherwise`: blocks, one-line forms, and errors for misplaced `otherwise`s. Also the shared block parser. |
| `src/front/parse_loop.c` | Every loop form, and `stop the loop` / `skip this one`. |
| `src/front/parse_func.c` | Function definitions, `give back`, call sentences, and the pre-scan that finds every `to NAME` first. |
| `src/front/consteval.{c,h}` | Works out constants' values before the program runs, with the same rules and messages as the runtime. |
| `src/front/check.{c,h}`, `check_const.c`, `check_internal.h` | Done for names. `check_program` walks the statements in order with a symbol table of the names made so far; each `if` block is a scope of its own. |
| `src/back/codegen_c.{c,h}`, `codegen_expr.c`, `codegen_internal.h` | Done. `codegen_c` writes the program (statements, loops, functions, constants in `codegen_c.c`; expressions in `codegen_expr.c`); see [Code generation](#code-generation). |
| `runtime/es_value.h` | Values, heap objects and reference counting, the memory checks, output, and errors. The first half of the header-only runtime copied to the top of every generated program. |
| `runtime/es_runtime.h` | The second half: what values can do (text, arithmetic, logic, comparisons, loops, functions, files, input). |
| `tools/embed.c` | A build tool that joins the runtime files into `build/gen/es_runtime_embed.c` (a byte array) so the compiler carries the runtime inside itself. |
| `src/main.c` | Done. The driver. |

## Code generation

Every EasyScript value is an `EsValue`, and every operation is a runtime call that receives the source line, so errors can say where they happened. For example, `say total minus 1` on line 4 becomes:

```c
es_say((es_t1 = es_v_total, es_sub(4, es_t1, es_num(1))));
```

- **`if`** becomes C `if (es_if(line, C)) { ... } else if (...) { ... } else { ... }`; `es_if` stops the program with a friendly error if the condition isn't yes or no.
- **Loops** become C loops inside their own `{ }`: a count is `EsCount es_cN = es_count_start(...)` plus `for (long long es_iN = 0; es_count_next(&es_cN, es_iN, &es_v_number); es_iN++)`, which works out each number from the start; `repeat N times` is a `for` over `es_times(...)`; `while`/`until` use `es_loop_condition`; `forever` is `for (;;)`. `stop the loop` and `skip this one` are C `break` and `continue`. `it` becomes the innermost count's variable, or `es_num(es_iN)` for a `times` loop.
- **Constants** become C static initializers, worked out by the compiler: `static const EsValue es_k_tax__rate = {ES_NUMBER, {.number = 0.2...}};` (text constants point to a static immortal text object). Even joined text is computed ahead of time, so constants cost nothing while the program runs.
- **Functions** become C functions named `es_f_` plus the name: `static EsValue es_f_area(int es_line, EsValue es_v_width, EsValue es_v_height)`. Prototypes come first, so calls work in any order and functions can call themselves. A function's temporaries and the names it makes are declared at its top (hoisted). It starts with `es_enter(es_line)`, which stops endless recursion with a friendly error, and every exit is `return es_leave(...)`. Calls evaluate their inputs into temporaries first: `(t1 = A, t2 = B, es_f_area(line, t1, t2))`.
- **Variables** become C locals of `main()` (or of their function) named `es_v_` plus the name, with `_` doubled and `'` written as `_q`, so names can't collide. Names made inside `if` and loop blocks, and loop numbers, are hoisted the same way: the checker makes sure each is only used inside its block, and the block's end releases it. Locals rather than globals keep their addresses from escaping, so the C compiler can keep numbers in registers.
- **Left to right:** the left side of every binary operation is stored in a temporary (`es_t1`, ...) before the right side is evaluated. C doesn't fix argument order, and this keeps evaluation, and so which error appears first, deterministic.
- **`and` and `or` short-circuit:** `(t = LEFT, es_is_no(t) ? t : es_and(line, t, RIGHT))`. `es_and` then decides at run time between logical and (two yes/no values) and joining text.
- **Literals:** numbers are re-printed with `%.17g` (so `007` and `08` are plain decimals), and text is a static immortal text object (`es_s1`, ...) holding a C string with octal escapes for non-ASCII bytes and `\?` for `?` (no trigraphs).
- **Self-contained:** the generated file starts with the runtime, so `cc -O2 program.c -o program -lm` is all it needs. Generated programs compile without warnings even under `-Wall -Wextra -Wpedantic`.
- **Runtime memory:** reference counting; text is freed as soon as nothing uses it. See [Memory in generated programs](#memory-in-generated-programs).

## Code rules

These are the rules every module follows (the full list is in [CONTRIBUTING.md](../CONTRIBUTING.md)):

- **Memory:** in the compiler, an arena for everything, no per-object `malloc`/`free`, and no fixed-size buffers (in the compiler or in generated C). The compiler has no leaks (checked with `leaks` on macOS; `make test-debug` keeps LeakSanitizer on where it's supported). Generated programs use reference counting instead (see above).
- **No global mutable state** in the compiler. Context structs (`Parser`, `Checker`, `Codegen`) are passed explicitly. (The runtime inside generated programs keeps a few statics by design: its memory counters, the function depth, and the shell settings.)
- **Small modules:** a small public header and private details in the `.c`. Opaque structs where other modules don't need the fields. Helpers are `static`. Files stay under about 600 lines, and functions are short and do one thing.
- **Errors go through `diag`,** with source spans, and a stage keeps going after an error, so one run reports everything.

## The CLI driver

| Command | Does |
|---|---|
| `easyscript run FILE` | Compile, build, run, and return the program's exit status |
| `easyscript build FILE -o OUT` | Compile and build an executable at `OUT` |
| `easyscript emit FILE` | Print the generated C |
| `easyscript tokens FILE` | Print the lexer's tokens (`KIND line:col text`), and any errors to stderr |
| `easyscript ast FILE` | Print the syntax tree as an indented outline (`node [line:col]`), and any errors to stderr |
| `easyscript` | Interactive shell (`src/shell.c`): type sentences one at a time; a line ending in `:` starts a block that ends at a blank line. Each entry is run together with everything kept so far, without repeating earlier output (the runtime skips `ES_SKIP_OUTPUT` bytes) or asking earlier `ask` questions again (answers are replayed from `ES_ANSWERS`). Entries with errors are shown and dropped. `exit`, `quit`, or `stop the program` leaves. |

`run`, `build`, and `emit` share one pipeline: lex, parse, check (only if parsing succeeded), print every error and exit 1 if there were any, otherwise generate C. `cc` is run as `cc -O2 FILE -o OUT -lm`. Generated C and binaries go in a fresh temporary directory that's removed on exit. Nothing is written next to your source except `build`'s `-o` output. `cc` is started directly (`fork`/`exec`), never through a shell.

## Testing

`make test` runs:

| Suite | Location | Checks |
|---|---|---|
| Unit tests | `tests/unit/*.c` | Arena, string builder, arrays, edit distance, diagnostic formatting, lexer and parser internals (spans, recovery, filler) |
| Lexer tests | `tests/tokens/` | Token output (`.out`) and exact error output (`.err`) |
| Parser tests | `tests/ast/` | Syntax tree output (`.out`) and exact error output (`.err`) |
| Program tests | `tests/run/` | Compile and run (stdin from `.in`), compare stdout; `.err` files are runtime errors (exit 1, exact stderr) |
| Compile-error tests | `tests/errors/` | Exact compiler error output from `emit` (checker errors, mostly) |
| Examples | `examples/programs/`, `examples/lexer/`, `examples/parser/` | Every example in the docs, with its expected output |
| Docs checks | end of `tests/run.sh` | The README's program blocks match their example files |
| CLI checks | end of `tests/run.sh` | Commands, usage errors, temp-file cleanup |

`make test-debug` runs the same suite against a build with AddressSanitizer and UndefinedBehaviorSanitizer. `make bless` rewrites the expected files from the current output, for intended changes only (see [CONTRIBUTING.md](../CONTRIBUTING.md)).

## Memory in generated programs

The compiler itself uses an arena. Generated programs use **reference counting**, described fully in [memory.md](memory.md). In short:

- Every heap object starts with a header (kind and reference count), so later kinds (lists, records, closures) reuse the same retain and release. Text is the only heap kind so far. Text literals and constants are static objects marked immortal; retain and release skip them.
- **Ownership convention:** expressions produce owned values; runtime operations and EasyScript functions consume their arguments; reading a variable retains it; storing (`es_set`) takes the new value and releases the old one after the new one is worked out; `give back` returns an owned value.
- **Scopes:** a block releases the names first made in it when it ends. `stop the loop`, `skip this one`, and `give back` release the names of every block they leave; the end of the program, or `stop the program` in the main program, releases every variable of `main()`.
- **Checks:** `ES_DEBUG_MEMORY=1` fails a program that ends with anything alive (every test program runs this way), `ES_MEMORY_LIMIT=N` bounds live heap text (`tests/memory/`), and `ES_SANITIZE=1` builds programs with AddressSanitizer and UBSan (`make test-debug`).
