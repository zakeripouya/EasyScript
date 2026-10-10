# Roadmap

EasyScript grows in five phases. Each phase ends with working, tested, documented features, and nothing is listed as done until `make test` proves it.

| Phase | Focus | Status |
|---|---|---|
| [1](#phase-1-core-language) | Core language | **Complete** (v0.1.0, 2026-10-10) |
| [2](#phase-2-data-and-structure) | Records, lists, maps, modules, static types | **In progress** |
| [3](#phase-3-self-hosting) | Self-hosting | Coming soon |
| [4](#phase-4-real-world-programs) | Standard library, C interop, concurrency, servers | Coming soon |
| [5](#phase-5-systems-mode) | Systems mode and an operating system | Coming soon |

Alongside the phases:

| Feature | When | Status |
|---|---|---|
| [EasyScript Notebook](#easyscript-notebook) | After Phase 1; grows with Phase 4 | Planned |

## Phase 1: Core language

**Complete as of v0.1.0 (2026-10-10).** Four items that were planned for Phase 1 moved to [Phase 2](#phase-2-data-and-structure) instead of holding up the release: reclaiming memory while programs run (since done), type checking, runtime errors that show the source line, and `file ... exists`. They're marked "moved to Phase 2" below.

The goal is to write small, real programs in the new sentence syntax and compile them to fast native executables, with friendly errors.

**Foundation**

- [x] Test system: program, error, lexer, unit, example, and CLI tests (`make test`)
- [x] Sanitizer build (`make debug`, `make test-debug`)
- [x] CLI: `run`, `build`, `emit`, `tokens`, `ast`, with generated files in a temp directory
- [x] Arena allocator, string builder, dynamic arrays, edit distance
- [x] Diagnostics: multiple errors per run, source line, carets, suggestions
- [x] Lexer: sentences, indentation blocks, words, numbers, text, symbols, comments, filler words

**Compiler**

- [x] Syntax tree (`src/common/ast.h`) and `easyscript ast`
- [x] Expression parser: arithmetic, comparisons in words and symbols, `and`/`or`/`not`, joining text, conversions, `length of`, `contents of file`, calls, with "Did you mean" and ambiguity errors
- [x] Statement parser for simple statements: variables, output, `ask`, files, `stop the program`, `please`
- [x] Checker for names: made before use, made once, "did you mean", "you make it later"
- [x] C code generator and runtime (tagged values, friendly runtime errors), embedded in the compiler
- [x] Legacy prototype pipeline removed
- [x] Blocks for `if` / `otherwise` (indentation, lining up, scoping)
- [x] Blocks for loops
- [x] Functions: definitions, `give back`, calls as sentences and values, recursion, use before definition
- [x] Runtime memory reclaimed during loops (moved to Phase 2, and done there: [reference counting](memory.md))
- Checker: types (moved to Phase 2's static types; block scopes are done)
- Runtime errors that show the source line (moved to Phase 2)

**Language features**

- [x] Variables: `let`/`set`/`change`, `add`, `subtract`, `multiply`, `divide`, `increase`, `decrease`
- [x] Values: whole numbers, decimals, text, `yes`/`no`, `nothing`; arithmetic, joining text, conversions, `length of`
- [x] Output and input: `say` (`print`, `show`, `display`, `write`), `ask … and call the answer`
- [x] Files: `write … to file`, `append … to file`, `read file … and call it`, `contents of file`
- [x] `stop the program`, `please`
- [x] Decisions: `if`, `otherwise if`, `otherwise` / `else`, one-line `if ..., S` and `if ... then S`; block scoping for names
- [x] Loops: `count from ... to ...` (up or down, `by`/`in steps of`, `as`), `count down`, `go from`, `for each`, `do this N times`/`repeat N times`, `while`/`as long as`/`repeat while`, `keep doing this until`/`repeat until`, `forever`; `stop the loop` and `skip this one`; `it`
- [x] Constants: `keep NAME as X`, worked out by the compiler, visible everywhere, never changed
- [x] Functions: `to NAME [with] A and B:`, `give back`/`return`, `greet "Paris".` / `call greet with ...`, `area of 3 and 4`; each function sees only its own names
- `file ... exists` (moved to Phase 2)

The proposed wording for each planned feature is in the [vocabulary](vocabulary.md).

### Examples

Done (in [`examples/programs/`](../examples/programs/), checked by `make test`): `hello.es`, `first_program.es`, `variables.es`, `text.es`, `logic.es`, `ask_name.es` (with test input), `files.es`, `decisions.es`, `loops.es`, `logan.es` (the 2024 prototype's loop example, ported), `factorial.es`, `fibonacci.es`, `tax.es` (a constant used by a function), `taste.es` (the README's bigger example), and the larger programs `fizzbuzz.es`, `guessing_game.es` (with test input), `notes.es`, `hanoi.es`, and `receipt.es`, `err_divide_by_zero.es`. The checker's "did you mean" error is tested in `tests/errors/misspelled_variable.es`.

Planned. Each will be added with its expected output in the same commit as its feature:

| Example | Feature |
|---|---|
| `file_exists.es` | `file ... exists` |
| `err_ambiguous.es` | an ambiguous block sentence and its suggestions |
| `word_counter.es` | Phase 2 (text tools): read a file and count its words. Needs a way to split text, which Phase 1 doesn't have |

## Phase 2: Data and structure

- [x] **Memory management:** reference counting. Text made while a program runs is freed as soon as nothing uses it, so loops run in flat memory (the text benchmark went from 280 MB to under 2 MB). Users never write anything about memory. See [memory.md](memory.md).
  - **Needed once records arrive:** reference counting can't free objects that refer to each other in a cycle. Text can't form cycles, but records (and lists of records) can, so they'll need **weak references or a cycle collector**.
- **Text tools:** splitting text into words and lines, and looking at its characters (needed for the planned `word_counter.es` example)
- **From Phase 1:** type checking (as part of static types), runtime errors that show the line of the program like compile errors do, and `file ... exists`
- **Records:** named groups of fields ("a point has an x and a y")
- **Lists:** ordered collections, with `for each item in list`
- **Maps:** look up values by key
- **Modules:** split programs across files and reuse code
- **Static types:** every value has a type the compiler checks before the program runs, inferred where possible, with English error messages when types don't match

## Phase 3: Self-hosting

Rewrite the EasyScript compiler in EasyScript and compile it with itself. This is the proof that the language can handle a large, real program. The C compiler stays as the bootstrap.

## Phase 4: Real-world programs

- **Standard library:** text, math, files, time, and collections
- **C interop:** call C libraries from EasyScript, and EasyScript from C
- **Concurrency:** doing several things at once, explained in plain words
- **Backend servers:** handle HTTP requests and build web services

## Phase 5: Systems mode

- **Systems mode:** a stricter subset with manual memory control and no runtime, suitable for kernels and embedded code
- **An operating system:** a small OS written in EasyScript, the long-term goal

## EasyScript Notebook

**Status: Planned.** It starts after Phase 1 and grows with Phase 4.

A chat-style notebook for EasyScript. You type a message, and the reply is its result.

- **Each message is an EasyScript statement that runs immediately.** Its results come back as replies: text at first, then tables and charts as the standard library grows in Phase 4. Later messages can use the variables made by earlier ones.
- **Messages that don't parse get friendly suggestions,** the way an assistant would reply. They come from the same deterministic errors the compiler gives ("Did you mean "say"?", "Write it like "let total be 0""), shown as a conversation instead of a compiler report.
- **An optional AI fallback.** For a vague request ("show me the biggest numbers"), an AI model can write EasyScript for it. The generated code is always **shown before it runs**, and nothing runs until you accept it. This follows the design rule that AI is optional: the notebook works fully without it, and turning it off changes nothing else.
- **The transcript exports as a runnable `.es` file:** the statements that ran, in order, with the conversation kept as `note:` comments. That file compiles and runs like any other program.
- **The Jupyter kernel protocol comes first.** EasyScript will first ship as a Jupyter kernel, so the notebook also works in Jupyter and VS Code from day one. The chat-style interface is then built on the same kernel.

Open questions:
- **Running one statement at a time:** EasyScript compiles to C, so it needs a way to run each statement while keeping the session's variables between messages (for example, recompiling the session, or a long-lived session process).
- **Rendering results:** how a result says it's a table or a chart, so the notebook can display it that way.

It builds on the interactive shell, which reruns the whole session for each entry (hiding output already shown and replaying earlier answers). That's fine for typing, but a notebook with tables and charts will want a real one-statement-at-a-time session.
