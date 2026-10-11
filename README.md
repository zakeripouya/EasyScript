# EasyScript

**Pseudocode that compiles.**

EasyScript is a programming language written in plain English sentences: the kind of step-by-step instructions you'd find in a textbook. The compiler turns those sentences into C, then into a fast native program.

## Vision

- **Reads like a textbook.** Lowercase, conversational sentences. A period or a new line ends a sentence, and a colon followed by an indented block groups sentences together.
- **Compiled to C for speed.** No interpreter and no virtual machine. EasyScript generates C and hands it to your system's C compiler (`cc -O2`).
- **Serious ambitions.** It starts with the core language. The goals after that are an EasyScript compiler written in EasyScript (self-hosting), then backend servers, and eventually an operating system written in EasyScript.

## A first program

This runs today:

```
let name be "Ada".
let total be 0.
add 5 to total.
increase total by 10.
say "Hello, " and name and "! Your total is " and total and ".".
```

```
$ ./easyscript run hello.es
Hello, Ada! Your total is 15.
```

## A bigger example

A loop and a function, also running today:

```
note: Count down from 3, then greet someone.
let count be 3.
repeat while count is greater than 0:
    say count.
    subtract 1 from count.
say "Liftoff!".

to greet using name:
    say "Hello, " and name and "!".

greet using "Ada".
```

```
$ ./easyscript run examples/programs/taste.es
3
2
1
Liftoff!
Hello, Ada!
```

`make test` runs this program ([`examples/programs/taste.es`](examples/programs/taste.es)) and checks its output, so this page can't drift from what the compiler does.

## Status

**Phase 1 (core language) is complete: version 0.1.0.** Programs written in the sentence syntax compile to C and run. Here's exactly what exists today.

**Works today:**

| Piece | What you can do |
|---|---|
| Variables | `let total be 0`, `set`/`change ... to`, and `add 5 to total`, `subtract`, `increase`/`decrease`/`multiply`/`divide ... by` |
| Names | A word (`total`, `clientName`) or several (`let late fee be 25`, `keep hourly rate as 150`, `to add tax to amount:`), recognized everywhere once made; matched without caring about capitals |
| Constants | `keep tax rate as 0.2`, fixed lists (`keep weekdays as a list of "Mon", "Tue" and "Wed"`) and fixed maps (`keep prices as a map of "coffee" to 3, "tea" to 2`): worked out by the compiler, usable everywhere (including inside functions), and never changed |
| Values and expressions | Numbers, text, yes/no; arithmetic in words or symbols; comparisons (`is at least`, `>=`, ...); `and`/`or`/`not`; joining text with `and` or `followed by`; `as a number`, `as text`, `length of`, `contents of file` |
| Output and input | `say` (or `print`, `show`, `display`, `write`), `ask "..." and call the answer name` |
| Files | `write ... to file`, `append ... to file`, `read file ... and call it ...` |
| Decisions | `if ... :` with indented blocks, `otherwise if`, `otherwise` (or `else`), and one-line `if x is 5, say "hi".` / `if x is 5 then say "hi".` |
| Loops | `count from 1 to 10:` (up or down, `by`/`in steps of`, `as n`), `for each`, `repeat 3 times:`, `while`/`as long as`, `keep doing this until`, `forever`, with `stop the loop` and `skip this one`; `it` is the loop's number |
| Lists | `a list of 72, 95 and 88`, `an empty list of text`, `item 2 of scores`, `the first item in`/`the last item of`; `add 5 to the end of scores`, `insert`, `remove item 2 from`, `set item 2 of scores to 100`, `sort` (also `biggest first`, `by their length`); `contains`, `is empty`, `the number of items in`; `for each score in scores:`; slices (`items 2 to 4 of scores`, `the first 3 items in`, `the last 2 items in`, `scores without the first item`, `scores without item 2`, `every second item of`); `every score in scores plus 5`, `only the scores that are at least 70`, `added together`, `the biggest of`, `reversed`, `scores sorted`, `a copy of`, joining (`morning slots followed by evening slots`); nested lists; printed in English ("72, 95 and 88") |
| Maps | `a map of "coffee" to 3, "tea" to 2`, `an empty map from text to numbers` (keys are text or numbers, values any kind); `set the value for "Paris" in ages to 19`, `the value for "Paris" in ages` (a missing key is a friendly runtime error naming it), `the value for "Bob" in ages, or 0`; `ages has "Paris"`, `the number of entries in ages`, `is empty`, `the keys of ages`, `the values of ages`; `remove "Paris" from ages`, `ages without "Paris"`, `add 1 to the value for "Paris" in ages`; `for each name and age in ages:`, always in the order keys were added; maps of lists, lists of maps, maps in records; `a copy of`; a fast hash table; printed readably (`"Paris" is 19 and "Junie" is 24`) |
| Text | Characters with the list forms (`character 3 of word`, `the first 3 characters of word`, `characters 2 to 5 of word`, `word without the first character`, `every other character of word`, `the characters of word`), counted as Unicode characters (`"café"` has 4); `contains`, `starts with`, `ends with`, `the position of "x" in word` (0 if it isn't there); `the words of`, `the lines of`, `split by ","`, `joined with ", "`; `in uppercase`, `in lowercase`, `without extra spaces`, `with "a" replaced by "b"`; `is a letter`, `is a digit`, `is a space`, `the code of`, `the character with code 65`; text built with `set line to line followed by ...` grows in place |
| Functions | `to make tea:` / `to greet with someone:` / `to area with width and height:` / `to add tax to amount:` / `to pause for seconds:` / `to busiest artist from names:`, `give back` / `return`, calls as sentences (`greet "Paris".`, `call greet with "Paris".`, `add tax to 100.`) or values (`area of 3 and 4`); recursion, and calling a function before it's defined |
| Types | Every value is a number, text, yes/no, a list of one kind, a record, or a map with one kind of key and value, worked out by the compiler (no annotations needed); kind mistakes are caught before the program runs; optional kinds in definitions: `to area with width (a number) and height (a number), giving back a number:`, `(a list of numbers)`, `(a map from text to numbers)` |
| Other | `stop the program`; any sentence can start with `please` |
| Memory | Nothing to manage: text, lists, records and maps are freed as soon as nothing uses them, so loops that make them run in flat memory ([how](docs/memory.md)) |
| Friendly errors | Compile errors show the line, the source, carets, and a suggestion ("Did you mean "total"? You made it on line 1."). Kind mistakes are compile errors too ("I can't add text to a number."). Runtime errors show the line with carets too, and the chain of function calls that led there, with the values they were given ("while working out "average" (count was 0)", "called from line 8: ...") |
| Tools | `run`, `build`, `emit`, `tokens`, `ast`, and an interactive shell. Generated programs are self-contained C. |
| Tests | Unit, lexer, parser, program, runtime-error, memory, example, and CLI tests with `make test` (every program also runs with a memory check), plus a sanitizer build with `make test-debug`; GitHub Actions runs both on Linux (GCC) and macOS (Clang) on every push |

See the runnable [example programs](examples/programs/).

**Phase 2 has started.** Done so far: **memory management** (text and lists are freed as soon as nothing uses them, so long loops run in flat memory; [how it works](docs/memory.md)), **static types** (the compiler works out whether every value is a number, text, yes/no, or a list, catches kind mistakes before the program runs, and generates plain C numbers; [Types](docs/language-guide.md#9-types)), **lists** (shared by reference, with items by position from 1, `for each`, `every`/`only`, sorting, and friendly errors; [Lists](docs/language-guide.md#10-lists)), **text tools** (characters, searching, splitting into words and lines, joining, changing case, character codes, and fast text building; [Text](docs/language-guide.md#11-text)), and **records** (`a dog is a kind of thing with:` and its fields, `a new dog with name "Rex" and age 3`, `rex's name`, fields that can be nothing and must be checked first, and methods like `to bark, for a dog:`, which different kinds of record can share by name; [Records](docs/language-guide.md#12-records)), and **maps** (lookup tables from text or number keys to any kind of value, in the order keys were added, with `the value for "Paris" in ages, or 0` for a default and fixed maps made with `keep`; [Maps](docs/language-guide.md#13-maps)). Runtime errors now show the source line and the chain of calls. **Coming next:** modules, weak references for records that point to each other, plus `file ... exists` (moved from Phase 1). Follow along in the [roadmap](docs/roadmap.md).

## Performance

Time (and peak memory) for each benchmark, measured with `make bench`:

| Program | EasyScript | C | Go | Python |
|---|---|---|---|---|
| Recursive fib(38) | 0.24 s | 0.07 s | 0.08 s | 2.85 s |
| Count to 100 million | 0.05 s | 0.02 s | 0.02 s | 4.07 s |
| Nested loops (100 million rounds) | 0.11 s | 0.03 s | 0.03 s | 5.16 s |
| Build a 50,000-character text | 0.00 s, 1.3 MB | (none) | 0.01 s, 11 MB | 0.01 s, 12 MB |
| Sort a million numbers | 0.06 s, 8.9 MB | 0.08 s (`qsort`), 8.9 MB | 0.08 s, 35 MB | 0.31 s, 63 MB |
| Add to a text a million times | 0.00 s, 6.0 MB | 0.00 s, 6.0 MB | 0.00 s, 18 MB | 0.04 s, 16 MB |
| Count two million words with a map | 0.10 s, 3.6 MB | (none) | 0.05 s, 8.1 MB | 0.39 s, 13 MB |

Apple M4, macOS 15.3.1, EasyScript 0.1.0 plus reference counting, static types, lists, stack traces, text tools, records and maps, 2026-10-10. The compiler knows every value's kind, so numbers are plain C doubles and loops run close to C. Sorting beats C's `qsort` because the comparison is inlined instead of called through a function pointer, and sorts numbers in place, in the same memory as C. Text that only one variable holds grows in place, so adding to it a million times takes about 5 ms, in the same memory as C (it took over two minutes before). Counting words with a map is 4 times faster than Python's dictionaries, in less memory than Go. Recursion is slower than C because every call checks for runaway recursion and records itself for runtime errors' call chain (so it can stop with a friendly error that says how it got there), and numbers compare with a small tolerance. Details and history: [docs/performance.md](docs/performance.md).

## Roadmap

| Phase | Focus | Status |
|---|---|---|
| 1 | **Core language:** variables, constants, arithmetic, text, output, input, decisions, loops, functions, files, friendly errors | **Complete** (v0.1.0) |
| 2 | **Data and structure:** memory management, static types, records, lists, maps, modules, text tools | **In progress** (memory management, static types, lists, text tools, records and maps done) |
| 3 | **Self-hosting:** the EasyScript compiler, written in EasyScript | Coming soon |
| 4 | **Real-world programs:** standard library, C interop, concurrency, backend servers | Coming soon |
| 5 | **Systems mode:** low-level control, no runtime, and an operating system | Coming soon |

Also planned: **EasyScript Notebook** (after Phase 1, growing with Phase 4). It's a chat-style notebook where each message is a statement that runs immediately, with results (text, tables, charts) shown as replies. Messages that don't parse get friendly suggestions. An optional AI fallback writes EasyScript for vague requests and always shows the code before running it. A whole conversation exports as a runnable `.es` file. It will ship as a Jupyter kernel first, so it also works in Jupyter and VS Code.

Details: [docs/roadmap.md](docs/roadmap.md).

## Design rules

1. **Deterministic parsing, no guessing.** Every sentence has exactly one meaning, or it's an error. The compiler never picks "the most likely" reading.
2. **Ambiguity is an error with suggestions.** If a sentence could mean two things, you're told so, along with the valid ways to write it.
3. **Filler words are ignored.** `the`, `a`, and `an` are dropped, so `let the total be 0.` and `let total be 0.` are the same sentence. The one exception is `is a letter` (and `is a digit`, `is a space`), which needs its `a`, so that `x is letter` can still compare with a variable called `letter`.
4. **Keywords are case-insensitive**, and words are contextual: a word like `count` can still be a variable name.
5. **Friendly English errors.** Every message says what went wrong in plain words, points at it, and suggests a fix. See [docs/errors.md](docs/errors.md).
6. **AI is optional.** The compiler is ordinary deterministic software. Nothing needs an AI model or a network connection to compile.

## Getting started

You need a C compiler (`cc`: Clang or GCC), `make`, and a POSIX system (macOS or Linux; on Windows, use WSL).

```bash
git clone https://github.com/zakeripouya/EasyScript.git
cd EasyScript
make              # builds ./easyscript
make test         # runs the full test suite, including every example
make debug        # builds a copy with AddressSanitizer and UBSan in build/debug/
make test-debug   # runs the full suite against the sanitizer build
make bench        # times the benchmarks against C, Go, and Python
```

### Commands

```bash
./easyscript run file.es             # compile and run
./easyscript build file.es -o app    # compile to an executable
./easyscript emit file.es            # print the generated C
./easyscript tokens file.es          # show how the lexer reads a file
./easyscript ast file.es             # show the syntax tree
./easyscript                         # interactive shell
```

Try it:

```bash
./easyscript run examples/programs/hello.es
./easyscript run examples/programs/text.es
./easyscript ast examples/parser/variables.es
```

## How the compiler works

```
source.es → lexer → parser → checker → C code generator → cc -O2 → native program
            tokens   syntax    meaning     generated C
                     tree      and names
```

The **front end** (lexer, parser, checker) understands EasyScript and reports errors. The **back end** turns the checked program into C. They only talk through shared data structures (tokens, the syntax tree, and the symbol table). Generated programs include a small runtime, embedded in the compiler, so they need nothing else to build. Today the checker resolves names; type checking comes later. See [docs/architecture.md](docs/architecture.md).

## Documentation

- [Language guide](docs/language-guide.md): learn EasyScript from zero, chapter by chapter
- [Vocabulary](docs/vocabulary.md): every keyword, sentence pattern, and synonym, with its status
- [Errors](docs/errors.md): what error messages look like and why
- [Design principles](docs/design-principles.md): the rules every phrasing follows, and the known exceptions
- [Ideas](docs/ideas.md): possible features that aren't planned yet
- [Security](SECURITY.md): how to report a problem, and what the compiler guarantees
- [Architecture](docs/architecture.md): how the compiler is built
- [Roadmap](docs/roadmap.md): the phases in detail
- [Performance](docs/performance.md): benchmarks against C, Go, and Python, with history
- [Examples](examples/): small programs, every one checked by `make test`
- [Changelog](CHANGELOG.md)

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) explains how to build, test, and organize code. In short: every feature needs tests, docs, and an example, and `make test` must pass.

## License

License: TBD. No license has been chosen yet. Until one is, all rights are reserved by the author.
