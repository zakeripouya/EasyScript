# Roadmap

EasyScript grows in five phases. Each phase ends with working, tested, documented features, and nothing is listed as done until `make test` proves it.

| Phase | Focus | Status |
|---|---|---|
| [1](#phase-1-core-language) | Core language | **In progress** |
| [2](#phase-2-data-and-structure) | Records, lists, maps, modules, static types | Coming soon |
| [3](#phase-3-self-hosting) | Self-hosting | Coming soon |
| [4](#phase-4-real-world-programs) | Standard library, C interop, concurrency, servers | Coming soon |
| [5](#phase-5-systems-mode) | Systems mode and an operating system | Coming soon |

Alongside the phases:

| Feature | When | Status |
|---|---|---|
| [EasyScript Notebook](#easyscript-notebook) | After Phase 1; grows with Phase 4 | Planned |

## Phase 1: Core language

The goal is to write small, real programs in the new sentence syntax and compile them to fast native executables, with friendly errors.

**Foundation**

- [x] Test system: program, error, lexer, unit, example, and CLI tests (`make test`)
- [x] Sanitizer build (`make debug`, `make test-debug`)
- [x] CLI: `run`, `build`, `emit`, `tokens`, with generated files in a temp directory
- [x] Arena allocator, string builder, dynamic arrays, edit distance
- [x] Diagnostics: multiple errors per run, source line, carets, suggestions
- [x] Lexer: sentences, indentation blocks, words, numbers, text, symbols, comments, filler words

**Compiler**

- [x] Syntax tree (`src/common/ast.h`) and `easyscript ast`
- [x] Expression parser: arithmetic, comparisons in words and symbols, `and`/`or`/`not`, joining text, conversions, `length of`, `contents of file`, calls, with "Did you mean" and ambiguity errors
- [x] Statement parser for simple statements: variables, output, `ask`, files, `stop the program`, `please`
- [ ] Blocks: `if`/`otherwise`, loops, and function definitions
- [ ] Checker: names, scopes, "did you mean"
- [ ] C code generator from the new AST
- [ ] Remove the legacy prototype pipeline

**Language features**

- [ ] Variables: `let`/`set`/`change`, `add`, `subtract`, `multiply`, `divide`, `increase`, `decrease` (parsing done)
- [ ] Values: whole numbers, decimals, text, `yes`/`no`, `nothing`; arithmetic and joining text (parsing done; checking and running to do)
- [ ] Output and input: `say` (`print`, `show`, `display`, `write`), `ask … and call the answer` (parsing done)
- [ ] Decisions: `if`, `otherwise if`, `otherwise`; comparisons in words and symbols; `and`, `or`, `not`
- [ ] Loops: `repeat N times`, `repeat while`, `repeat until`, `for each ... from ... to`, `stop`, `skip`
- [ ] Functions: `to NAME using ...`, calls, `give back`
- [ ] Files: `write … to file`, `append … to file`, `read file … and call it` (parsing done), `file ... exists`
- [ ] `stop the program` (parsing done)

The proposed wording for each feature is in the [vocabulary](vocabulary.md).

### Planned examples

Each of these will be added to `examples/` with its expected output in the same commit as its feature:

| Example | Feature |
|---|---|
| `hello.es` | `say` |
| `variables.es` | `set`, arithmetic sentences |
| `text.es` | joining text with `and`/`followed by`, escapes in output |
| `ask_name.es` | `ask … and call the answer` (with test input) |
| `decisions.es` | `if` / `otherwise if` / `otherwise` |
| `conditions.es` | comparisons in words and symbols, `and`/`or`/`not` |
| `repeat.es` | `repeat N times` |
| `countdown.es` | `repeat while` (the README taste) |
| `for_each.es` | `for each ... from ... to`, `stop`, `skip` |
| `functions.es` | `to ... using`, calls, `give back` |
| `files.es` | `write … to file`, `append … to file`, `read file … and call it`, `file ... exists` |
| `err_unknown_name.es` | "did you mean" for misspelled names |
| `err_ambiguous.es` | an ambiguous sentence and its suggestions |

## Phase 2: Data and structure

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

It depends on Phase 1 (statements that run) and on the interactive shell being rebuilt on the new compiler.
