# Roadmap

EasyScript grows in five phases. Each phase ends with working, tested, documented features, and nothing is listed as done until `make test` proves it.

| Phase | Focus | Status |
|---|---|---|
| [1](#phase-1-core-language) | Core language | **Complete** (v0.1.0, 2026-10-10) |
| [2](#phase-2-data-and-structure) | Memory, static types, records, lists, maps, kinds with variants, functions as values, interfaces, a standard library start, the English pass | **In progress** (memory, static types, lists, text tools, records, maps, kinds with variants, functions as values, interfaces, modules and the standard library start done) |
| [3](#phase-3-self-hosting) | Self-hosting | Coming soon |
| [4](#phase-4-real-world-programs) | Standard library, C interop, concurrency, servers | Coming soon |
| [5](#phase-5-systems-mode) | Systems mode and an operating system | Coming soon |

Alongside the phases:

| Feature | When | Status |
|---|---|---|
| [EasyScript Notebook](#easyscript-notebook) | After Phase 1; grows with Phase 4 | Planned |
| [Browser playground](#browser-playground) | Right after v0.2.0 | Planned |
| [Going public](#going-public) | Before the first public release | Planned |

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
- [x] C code generator and runtime (tagged values in Phase 1, typed since Phase 2; friendly runtime errors), embedded in the compiler
- [x] Legacy prototype pipeline removed
- [x] Blocks for `if` / `otherwise` (indentation, lining up, scoping)
- [x] Blocks for loops
- [x] Functions: definitions, `give back`, calls as sentences and values, recursion, use before definition
- [x] Runtime memory reclaimed during loops (moved to Phase 2, and done there: [reference counting](memory.md))
- [x] Checker: types (moved to Phase 2, and done there: [static types](language-guide.md#9-types); block scopes are done)
- [x] Runtime errors that show the source line (moved to Phase 2, and done there, with the chain of calls)

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
- [x] `file ... exists` (moved to Phase 2, done in step 21)

The proposed wording for each planned feature is in the [vocabulary](vocabulary.md).

### Examples

Done (in [`examples/programs/`](../examples/programs/), checked by `make test`): `hello.es`, `first_program.es`, `variables.es`, `text.es`, `logic.es`, `ask_name.es` (with test input), `files.es`, `decisions.es`, `loops.es`, `logan.es` (the 2024 prototype's loop example, ported), `factorial.es`, `fibonacci.es`, `tax.es` (a constant used by a function), `taste.es` (the README's bigger example), and the larger programs `fizzbuzz.es`, `guessing_game.es` (with test input), `notes.es`, `hanoi.es`, and `receipt.es`, `err_divide_by_zero.es`. Phase 2 added `million_lines.es` (memory), `kinds.es` (types), `grade_statistics.es` and `top_scores.es` (lists), `word_counter.es` (text tools and maps: reads a file, counts its words with a map, and reports the most common ones), `linked_list.es` and `binary_tree.es` (records), `pets.es` (methods of the same name for different kinds of record), `graph.es` (maps: shortest paths with breadth-first search), `shapes.es`, `calculator.es` and `lambda.es` (kinds with variants: an area calculator, a calculator with precedence, and a lambda calculus interpreter), and `counter.es`, `event_handlers.es` and `pipeline.es` (functions as values: closures that share state, a map from event names to functions, and a sales report built with apply, keep only and combine), and `zoo.es` (an interface with dogs, cats and birds in one list). The checker's "did you mean" error is tested in `tests/errors/misspelled_variable.es`.

Planned. Each will be added with its expected output in the same commit as its feature:

| Example | Feature |
|---|---|
| `err_ambiguous.es` | an ambiguous block sentence and its suggestions |

## Phase 2: Data and structure

- [x] **Memory management:** reference counting. Text (and, since lists arrived, lists) made while a program runs is freed as soon as nothing uses it, so loops run in flat memory (the text benchmark went from 280 MB to under 2 MB). Users never write anything about memory. See [memory.md](memory.md).
  - **Records can form cycles** (two records that point to each other), which reference counting can't free. Since records arrived, the memory check (`ES_DEBUG_MEMORY=1`) finds and explains them at the end of the program. **Still to come: weak references** (or a cycle collector) so they're freed while the program runs. See [memory.md](memory.md#cycles).
- [x] **Text tools:** characters taken with the list forms (`character 3 of word`, `the first 3 characters of word`, `every other character of word`, `the characters of word`), counted as Unicode characters; `contains`, `starts with`, `ends with`, `the position of`; `the words of`, `the lines of`, `split by`, `joined with`; `in uppercase`, `in lowercase`, `without extra spaces`, `with ... replaced by`; `is a letter`/`is a digit`/`is a space`, `the code of`, `the character with code`; text built piece by piece grows in place (a million pieces: 130 s before, about 5 ms after). See [Text](language-guide.md#11-text).
- [x] **Static types:** the compiler works out whether every value is a number, text, or yes/no, inferring function inputs and results from how they're used and called, with optional kinds in definitions (`to area with width (a number), giving back a number:`). Kind mistakes are compile errors with the runtime's old wording plus carets; generated code uses plain C `double`/`bool` and only text is reference counted. See [Types](language-guide.md#9-types).
- [x] **From Phase 1: runtime errors with the source line,** shown like compile errors, plus an English stack trace: the function being worked out with the values it was given, and each call back to the main program (repeated calls collapsed).
- [x] **From Phase 1:** `file ... exists` (step 21)
- [x] **Records:** kinds of record with named fields (`a dog is a kind of thing with:`), `a new dog with name "Rex" and age 3` (missing fields are compile errors unless they have a starting value), fields with `'s` (`the order's customer's name`), fields that can be nothing and have to be checked before use, sharing by reference with shallow `a copy of`, methods (`to bark, for a dog:`), records in lists and lists in records, and readable printing. Linked list and binary tree examples. See [Records](language-guide.md#12-records).
- [x] **Lists:** ordered collections of one kind, shared by reference (`a copy of` makes a separate one), with 1-based positions, `for each score in scores:`, `every`/`only`, totals, a stable sort, nested lists, and friendly errors for positions that aren't there. Reference counted like text; a list can't hold itself, so there are no cycles. See [Lists](language-guide.md#10-lists).
- [x] **Maps:** lookup tables from text or number keys to values of any kind (`a map of "coffee" to 3, "tea" to 2`, `an empty map from text to numbers`), `the value for "tea" in prices` (a missing key is a friendly runtime error naming it) and `, or 0` for a default, `has`, `the number of entries in`, `the keys of`, `the values of`, `remove ... from`, `add 1 to the value for ...`, `for each name and age in ages:` in the order keys were added, fixed maps with `keep`, maps of lists and records and lists and records of maps, sharing with `a copy of`, and readable printing. A hash table with reference-counted keys and values; a `words` benchmark against Python and Go. Word counter and graph (shortest path) examples. See [Maps](language-guide.md#13-maps).
- [x] **Kinds with variants:** `a shape is one of:` with variants and their fields (`a circle with a radius (a number).`), recursive kinds, `a new circle with radius 3`, `check which kind s is:` with `when it's a circle:` (every variant covered, or a compile error; an `otherwise` that can never run is a warning), `s is a circle`, fields usable only once the variant is checked, values that never change and compare by value, and kinds in lists, maps, records, functions and methods. Shapes, calculator and lambda calculus examples. See [Kinds with variants](language-guide.md#14-kinds-with-variants).
- [x] **Functions as values:** `the function double`, functions made inside values (`a function taking x that gives back x times 2`, or with a block), `call f with 3`, kinds of functions, functions in lists, maps, records and variants, and functions that share the names they use from around them (closures), and built-ins that take a function (`applied to every`, `where`, `that pass`, `combined using`, `sorted by`, with `apply`, `keep only` and `sort ... by` sentences). Counter, event handler and pipeline examples; a `pipeline` benchmark. See [Functions as values](language-guide.md#15-functions-as-values).
- [x] **Interfaces:** `something that can speak is anything with:` and its ways; any kind with matching methods fits, without saying so; interfaces as kinds of inputs, lists, maps and fields; ways called on them pick the method by the value's kind. Zoo example. See [Interfaces](language-guide.md#16-interfaces).
- [x] **Modules (step 21):** `use helpers.` splits a program across files; used files only define things, `privately` keeps a definition to its file, `helpers's double` says which file's, circles of use are errors, and errors name the file. Multi-file example. See [Programs in several files](language-guide.md#17-programs-in-several-files).
- [x] **The standard library (step 21):** math, random numbers, dates and waiting, files and folders, csv files (`read csv file F and call it NAME`, quoted fields, each row a map from column name to value) and clearing the screen, all without `use`. Dice, countdown, tidy folder and sales report examples; a csv benchmark against Python. See [stdlib.md](stdlib.md).
- **Planned, step 22:** raw keyboard input (a key at a time, without Enter), with a terminal Snake example.
- **Planned, the English pass** (after step 22): questions and totals, choices without `if`, plain words, and the rough spots. See [english-pass.md](english-pass.md).
- **Planned:** the first non-coder test (3 to 5 people), then **v0.2.0**, which freezes the core grammar: after it, only additions.

## Phase 3: Self-hosting

Rewrite the EasyScript compiler in EasyScript and compile it with itself. This is the proof that the language can handle a large, real program. The C compiler stays as the bootstrap.

- **Planned, after the bootstrap:** generics.

## Phase 4: Real-world programs

- **Standard library:** text, math, files, time, and collections
- **C interop:** call C libraries from EasyScript, and EasyScript from C
- **Concurrency:** doing several things at once, explained in plain words
- **Backend servers:** handle HTTP requests and build web services
- **Planned, numbers and bytes** (moved up from Phase 5): fixed-size whole numbers, bitwise operations, raw bytes, and big numbers.
- **Planned, native code:** an ARM64 backend, then x86-64, with the C backend kept as the reference; a Julia-style JIT on top (Apple Silicon needs `MAP_JIT` and a JIT entitlement); its own linker, so no C compiler is needed; a WebAssembly backend.
- **Planned, tools:** Language Server Protocol support for editors, a formatter, and a package manager.
- **Planned, a middle end that keeps intents:** high-level operations reach it as themselves, so it can pick a strategy for each and explain it ("explain how").
- **Planned, networking from scratch:** DNS, HTTP/1.1, DEFLATE, crypto basics, TLS 1.3, certificates, cookies, forms and logins, HTTP/2, WebSockets. The homemade TLS is for learning; production code uses a wrapped, audited library.
- **Planned, more of the library:** persistence (values `kept in` a file), a web kit, xlsx files, email, and games with raylib.

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

Decided:
- **Running one statement at a time:** the JIT (Phase 4) runs each message in a long-lived session, which keeps the session's names between messages.

Open questions:
- **Rendering results:** how a result says it's a table or a chart, so the notebook can display it that way.

It builds on the interactive shell, which reruns the whole session for each entry (hiding output already shown and replaying earlier answers). That's fine for typing, but a notebook with tables and charts will want a real one-statement-at-a-time session.

## Browser playground

**Status: Planned,** right after v0.2.0. The C compiler is compiled to WebAssembly, so EasyScript runs in a browser tab with nothing to install.

- **Templates** to start from
- **"Explain this program"** and **"why is X N?"**, answered from the program itself
- **Values shown as the program runs**
- **One-key fixes** for errors that come with a suggestion
- **Charts and tables** for results
- **Learning levels** that show more of the language as you go

## Going public

**Status: Planned,** before the first public release.

- **Installing:** an install script, `easyscript --version`, and a Homebrew tap
- **A license** (still to be chosen)
- **Windows:** documented through WSL now; then a native port of `src/main.c` and `src/shell.c` (processes and temp directories), checked by CI on `windows-latest`

