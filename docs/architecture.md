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
| **Checker** | AST → diagnostics, kinds | Resolves names (made before use, made once, in scope) with "did you mean" suggestions, checks functions and constants, and infers the kind (type) of every value, writing it into the AST for codegen (see [Types](#types)). It runs only when parsing succeeded. |
| **Middle end** | (added later) | An intermediate representation and optimizations. |
| **C codegen** | checked AST → C source | Writes a self-contained C program: the embedded runtime, a static object per text literal, the functions, and `main()`, whose variables are C locals. It inserts every retain and release (see [Memory](#memory-in-generated-programs)). It never looks at source text and reports no errors. |
| **Runtime** | (inside the generated program) | `runtime/es_memory.h` and `runtime/es_runtime.h`: reference-counted text objects (`EsText`), the operations on numbers (`double`), yes/no (`bool`) and text, and friendly runtime errors (`Line N: ...`) for what kinds can't catch. There's no tagged value. |
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
| `src/front/parse_expr.c` | Done. Recursive-descent expression parser: the operator levels (`every`, `or`, `and`, `not`, comparisons, `followed by`, arithmetic, unary minus, postfix conversions and list operations) and `only`'s comparison. |
| `src/front/parse_value.c` | Done. The tightest level: literals, names, calls (with their argument rules), groups, `length of`/`contents of file`, and the dispatch to the list forms that start with a word. |
| `src/front/parse_stmt.c` | A table of statement forms, names (with filler and reserved-word errors), and the statement loop (`parse_statements`), used for the whole program and for each block. |
| `src/front/parse_if.c` | `if` / `otherwise if` / `otherwise`: blocks, one-line forms, and errors for misplaced `otherwise`s. Also the shared block parser. |
| `src/front/parse_loop.c` | Every loop form, and `stop the loop` / `skip this one`. |
| `src/front/parse_func.c` | Function definitions, `give back`, call sentences, and the pre-scan that finds every `to NAME` first. |
| `src/front/parse_list.c` | Lists: `a list of`, `an empty list of`, items and positions, the list sentences (`add ... to the end of`, `insert`, `remove`, `set item`, `sort`), and kinds of values (`parser_parse_kind`, shared with definitions). `every`, `only`, `contains`, `is empty`, `added together` and `reversed` depend on precedence, so they're in `parse_expr.c`. |
| `src/front/consteval.{c,h}` | Works out constants' values before the program runs, with the same rules and messages as the checker and runtime. |
| `src/front/check.{c,h}`, `check_expr.c`, `check_types.c`, `check_const.c`, `check_list.c`, `check_internal.h` | `check_program` checks constants, then function bodies, then the main program, with a symbol table of the names made so far (each block is a scope of its own). `check_expr.c` types expressions and calls; `check_types.c` is the inference engine; `check_list.c` types list values and sentences. |
| `src/back/codegen_c.{c,h}`, `codegen_expr.c`, `codegen_list.c`, `codegen_internal.h` | Done. `codegen_c` writes the program (statements, loops, functions, constants in `codegen_c.c`; expressions in `codegen_expr.c`; lists, `for each`, and the helper functions for `every`/`only` in `codegen_list.c`); see [Code generation](#code-generation). |
| `runtime/es_memory.h` | Heap objects (text and lists) and reference counting, the memory checks, output, and errors. The first part of the header-only runtime copied to the top of every generated program. |
| `runtime/es_runtime.h` | The second part: what values can do (text, arithmetic, logic, comparisons, loops, functions, files, input). |
| `runtime/es_list.h` | The third part: lists (making, items, questions, changes, a stable merge sort, and printing them in English). |
| `tools/embed.c` | A build tool that joins the runtime files into `build/gen/es_runtime_embed.c` (a byte array) so the compiler carries the runtime inside itself. |
| `src/main.c` | Done. The driver. |

## Types

The checker infers the kind of every value: number, text, yes/no, a list of one kind, or nothing (only what a function gives back). See the [language guide](language-guide.md#9-types) for the rules users see.

- **Types are numbers:** `Type` (in `ast.h`) is the innermost kind plus `TYPE_LIST` per level of list, so "a list of lists of text" is `2 * TYPE_LIST + TYPE_TEXT`, and two types are the same exactly when they're equal. `ast_type_name` turns one into "a list of lists of text".

- **Type variables:** every value gets a variable (`check_types.c`), joined with union-find when two values must have the same kind (a variable and what's stored in it, an input and what it's given, the two sides of `is`). A variable gets its kind as soon as anything decides it, remembering the line, for notes like `"x" is a number because of line 2.`
- **Functions:** each input and result is a variable too, so its kind comes from the body and from every call; annotations (`(a number)`, `giving back text`) fix it up front. A function with no `give back X` gives back nothing, decided before inference.
- **Lists:** a list's type variable is "a list of" another variable (`TypeSlot.item`), so `item 1 of xs` has the kind of xs's items before anything decides it; joining two lists joins their items. An occurs check stops a list from holding itself (`add xs to the end of xs`).
- **Waiting operators:** `and` (logical or joining), ordering comparisons (numbers or text), `as a number` (from a number or text), and operations that take one of a few kinds (`length of` and `is empty` take text or a list; `sort`, `biggest of` and `smallest of` take numbers or text, or text or lists by length) depend on their sides' kinds. They wait in a list and are settled when the sides are known, going round until nothing changes.
- **Order:** constants first, then every function body, then the main program, so a bad call is reported at the call; errors are then sorted into source order (`diag_sort`). A kind nothing decides is reported (with a suggested annotation) only when there are no other errors, since it's usually a side effect of one.
- **Errors** use the wording the runtime used before kinds existed, now with the line and carets. `TYPE_ERROR` stands in after a mistake, so one mistake gives one message.
- **Constants** are typed by their worked-out value; `consteval.c` reports kind mistakes in a constant with the same wording.

## Code generation

The checker has decided every kind, so generated code uses plain C types: a number is a `double`, yes/no a `bool`, text an `EsText *`, a list an `EsList *`, and a function that gives back nothing returns `void`. There is no tagged value. Runtime calls that can fail receive the source line, so errors can say where they happened. For example, `say total minus 1` on line 4 becomes:

```c
es_say(es_number_text((es_vn_total - 1.0)));
```

- **Arithmetic and logic** are plain C: `+ - *`, `&&`, `||`, `!`. Division and `mod` are `es_div`/`es_mod`, which stop on zero (`es_mod` uses the integer remainder for whole numbers, which gives the same answer as `fmod`, much faster). Number equality and ordering use `es_close` (the 1e-12 tolerance); text uses `es_text_same`/`es_text_order`.
- **`and`** was resolved by the checker: `&&` between yes/no values, `es_join` when it joins text.
- **`if`** becomes C `if (C) { ... } else if (...) { ... } else { ... }`.
- **Loops** become C loops inside their own `{ }`: a count is `EsCount es_cN = es_count_start(...)`, which works out the last round once, plus `for (long long es_iN = 0; es_iN <= es_cN.last; es_iN++)` with `es_vn_number = es_count_value(&es_cN, es_iN)`, so each number is worked out from the start (no drifting); `repeat N times` is a `for` over `es_times(...)`; `while`/`until` are C `while`; `forever` is `for (;;)`. `stop the loop` and `skip this one` are C `break` and `continue`. `it` becomes the innermost count's variable, or `((double)es_iN)` for a `times` loop.
- **Constants** become C static initializers, worked out by the compiler: `static const double es_k_tax__rate = 0.2...;`, `static const bool ...`, or `static EsText *const es_k_shop = &es_s3;` (a static immortal text object). Even joined text is computed ahead of time, so constants cost nothing while the program runs.
- **Functions** become C functions named `es_f_` plus the name, with C types from their kinds: `static double es_f_area(int es_line, double es_vn_width, double es_vn_height)`. Prototypes come first, so calls work in any order and functions can call themselves. A function's temporaries and the names it makes are declared at its top (hoisted). It starts with `es_enter(es_line)`, which stops endless recursion with a friendly error, and every exit calls `es_leave()`. One that gives back a value and reaches its end calls `es_no_result`, a runtime error.
- **Lists** call the runtime in `es_list.h`, with one function per kind of item where it matters (`es_get_n`, `es_add_t`, ...): `a list of 1 and 2` is `es_list_from(ES_ITEMS_NUMBER, 2, (double[]){1.0, 2.0})`, and `item 2 of scores` is `es_get_n(line, es_list_retain(es_vl1n_scores), ES_AT, 2.0)`. Slices are `es_slice_range`, `es_slice_end`, `es_slice_without` and `es_slice_every`, each making a new list. `for each` holds the list it goes through in a hidden variable (`es_list_snapshot`, a copy only if something else holds the list) and loops over its items. `every` and `only` each become a small C function before the program's functions (`static EsList *es_every1(EsList *es_list, double es_vn_bonus)`), whose parameters are the variables the expression reads, since C has no closures.
- **Variables** become C locals of `main()` (or of their function) named by kind and name: `es_vn_` (number), `es_vt_` (text), `es_vb_` (yes/no), or `es_vl` plus the list depth and innermost kind (`es_vl1n_`, `es_vl2t_`) plus the name, with `_` doubled and `'` written as `_q`. Blocks side by side can make the same name with different kinds. Names made inside `if` and loop blocks, and loop numbers, are hoisted the same way: the checker makes sure each is only used inside its block, and the block's end releases text. Locals rather than globals keep their addresses from escaping, so the C compiler can keep numbers in registers.
- **Left to right:** when both operands of an operation (or two or more call arguments, list items, or parts of a list sentence) can fail or have effects (calls, file reads, dividing, `as a number` on text, and anything that reads what's in a list, since a call could change the list), the left ones go into temporaries first: `(es_t1 = LEFT, ... RIGHT ...)`. C doesn't fix the order of operands, and this keeps evaluation, and so which error appears first, deterministic. Operands without effects are used directly.
- **Literals:** numbers are re-printed with `%.17g` (so `007` and `08` are plain decimals), and text is a static immortal text object (`es_s1`, ...) holding a C string with octal escapes for non-ASCII bytes and `\?` for `?` (no trigraphs).
- **Self-contained:** the generated file starts with the runtime, so `cc -O2 program.c -o program -lm` is all it needs. Generated programs compile without warnings even under `-Wall -Wextra -Wpedantic`, with both GCC and Clang (checked by `make test`).
- **Runtime memory:** reference counting; text and lists are freed as soon as nothing uses them. See [Memory in generated programs](#memory-in-generated-programs).

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
| Strict C | end of `tests/run.sh` | Every generated program compiles with `-Werror` under the system compiler |
| CLI checks | end of `tests/run.sh` | Commands, usage errors, temp-file cleanup |

`make test` also compiles every generated program with `-Wall -Wextra -Wpedantic -Werror`. `make test-debug` runs the same suite against a build with AddressSanitizer and UndefinedBehaviorSanitizer. GitHub Actions (`.github/workflows/ci.yml`) runs `make`, `make test` and `make test-debug` on every push, on Ubuntu with GCC and on macOS with Clang. `make bless` rewrites the expected files from the current output, for intended changes only (see [CONTRIBUTING.md](../CONTRIBUTING.md)).

## Memory in generated programs

The compiler itself uses an arena. Generated programs use **reference counting**, described fully in [memory.md](memory.md). In short:

- Every heap object starts with a header (kind and reference count), so all kinds (text, lists, and later records) share the same retain and release. Text literals and constants are static objects marked immortal; retain and release skip them.
- **Only text and lists are counted:** numbers and yes/no values are plain C values that own nothing, so no retain or release is ever generated for them. A list owns the text and lists in it.
- **Ownership convention:** text and list expressions produce owned values; runtime operations and EasyScript functions consume their text and list arguments; reading a variable retains it; storing (`es_set`, `es_list_set`) takes the new value and releases the old one after the new one is worked out; `give back` returns an owned value.
- **Scopes:** a block releases the text and list variables first made in it when it ends. `stop the loop`, `skip this one`, and `give back` release the names of every block they leave; the end of the program, or `stop the program` in the main program, releases every variable of `main()`.
- **No cycles:** a list can't hold a list of its own kind, so lists can never refer to themselves, and reference counting frees everything.
- **Checks:** `ES_DEBUG_MEMORY=1` fails a program that ends with anything alive (every test program runs this way), `ES_MEMORY_LIMIT=N` bounds live text and lists (`tests/memory/`), and `ES_SANITIZE=1` builds programs with AddressSanitizer and UBSan (`make test-debug`).
