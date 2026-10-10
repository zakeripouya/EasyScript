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

## A taste of what's coming

This is what a slightly bigger program is planned to look like:

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

> **Coming soon:** functions (`to greet using name`) don't run yet; everything above them does. The lexer already reads this program, and the test suite checks that it does ([`examples/lexer/taste.es`](examples/lexer/taste.es)). The wording of planned sentences may still change before they ship; [docs/vocabulary.md](docs/vocabulary.md) tracks the proposals.

## Status

EasyScript is in **Phase 1 (core language)**. Programs written in the sentence syntax compile to C and run. Here's exactly what exists today.

**Works today:**

| Piece | What you can do |
|---|---|
| Variables | `let total be 0`, `set`/`change ... to`, and `add 5 to total`, `subtract`, `increase`/`decrease`/`multiply`/`divide ... by` |
| Values and expressions | Numbers, text, yes/no, nothing; arithmetic in words or symbols; comparisons (`is at least`, `>=`, ...); `and`/`or`/`not`; joining text with `and` or `followed by`; `as a number`, `as text`, `length of`, `contents of file` |
| Output and input | `say` (or `print`, `show`, `display`, `write`), `ask "..." and call the answer name` |
| Files | `write ... to file`, `append ... to file`, `read file ... and call it ...` |
| Decisions | `if ... :` with indented blocks, `otherwise if`, `otherwise` (or `else`), and one-line `if x is 5, say "hi".` / `if x is 5 then say "hi".` |
| Loops | `count from 1 to 10:` (up or down, `by`/`in steps of`, `as n`), `for each`, `repeat 3 times:`, `while`/`as long as`, `keep doing this until`, `forever`, with `stop the loop` and `skip this one`; `it` is the loop's number |
| Other | `stop the program`; any sentence can start with `please` |
| Friendly errors | Compile errors show the line, the source, carets, and a suggestion ("Did you mean "total"? You made it on line 1."). Runtime errors say what happened and where: "Line 8: You divided by zero." |
| Tools | `run`, `build`, `emit`, `tokens`, `ast`, and an interactive shell. Generated programs are self-contained C. |
| Tests | Unit, lexer, parser, program, runtime-error, example, and CLI tests with `make test`, plus a sanitizer build with `make test-debug` |

See the runnable [example programs](examples/programs/).

**Coming soon** (Phase 1): your own functions, and a few more built-ins. Follow along in the [roadmap](docs/roadmap.md).

## Roadmap

| Phase | Focus | Status |
|---|---|---|
| 1 | **Core language:** variables, arithmetic, text, output, input, decisions, loops, functions, files, friendly errors | In progress (variables, expressions, decisions, loops, output, input, and files run; functions next) |
| 2 | **Data and structure:** records, lists, maps, modules, static types | Coming soon |
| 3 | **Self-hosting:** the EasyScript compiler, written in EasyScript | Coming soon |
| 4 | **Real-world programs:** standard library, C interop, concurrency, backend servers | Coming soon |
| 5 | **Systems mode:** low-level control, no runtime, and an operating system | Coming soon |

Also planned: **EasyScript Notebook** (after Phase 1, growing with Phase 4). It's a chat-style notebook where each message is a statement that runs immediately, with results (text, tables, charts) shown as replies. Messages that don't parse get friendly suggestions. An optional AI fallback writes EasyScript for vague requests and always shows the code before running it. A whole conversation exports as a runnable `.es` file. It will ship as a Jupyter kernel first, so it also works in Jupyter and VS Code.

Details: [docs/roadmap.md](docs/roadmap.md).

## Design rules

1. **Deterministic parsing, no guessing.** Every sentence has exactly one meaning, or it's an error. The compiler never picks "the most likely" reading.
2. **Ambiguity is an error with suggestions.** If a sentence could mean two things, you're told so, along with the valid ways to write it.
3. **Filler words are ignored.** `the`, `a`, and `an` are dropped, so `let the total be 0.` and `let total be 0.` are the same sentence.
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
- [Architecture](docs/architecture.md): how the compiler is built
- [Roadmap](docs/roadmap.md): the phases in detail
- [Examples](examples/): small programs, every one checked by `make test`
- [Changelog](CHANGELOG.md)

## Contributing

Contributions are welcome. [CONTRIBUTING.md](CONTRIBUTING.md) explains how to build, test, and organize code. In short: every feature needs tests, docs, and an example, and `make test` must pass.

## License

License: TBD. No license has been chosen yet. Until one is, all rights are reserved by the author.
