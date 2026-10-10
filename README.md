# EasyScript

**Pseudocode that compiles.**

EasyScript is a programming language written in plain English sentences: the kind of step-by-step instructions you'd find in a textbook. The compiler turns those sentences into C, then into a fast native program.

## Vision

- **Reads like a textbook.** Lowercase, conversational sentences. A period or a new line ends a sentence, and a colon followed by an indented block groups sentences together.
- **Compiled to C for speed.** No interpreter and no virtual machine. EasyScript generates C and hands it to your system's C compiler (`cc -O2`).
- **Serious ambitions.** It starts with the core language. The goals after that are an EasyScript compiler written in EasyScript (self-hosting), then backend servers, and eventually an operating system written in EasyScript.

## A taste

This is what a small program is planned to look like:

```
note: Count down from 3, then greet someone.
set count to 3.
repeat while count is greater than 0:
    say count.
    subtract 1 from count.
say "Liftoff!".

to greet using name:
    say "Hello, " + name + "!".

greet using "Ada".
```

> **Coming soon:** none of these sentences run yet. The new lexer already reads this program, and the test suite checks that it does ([`examples/lexer/taste.es`](examples/lexer/taste.es)). The exact wording of each sentence may still change before it ships; [docs/vocabulary.md](docs/vocabulary.md) tracks the proposals.

## Status

EasyScript is at the start of **Phase 1 (core language)**. Here's exactly what exists today.

**Works today:**

| Piece | What you can do |
|---|---|
| New lexer | `easyscript tokens file.es` splits any file in the new syntax into words, numbers, text, symbols, and indentation, and reports friendly errors. See the [lexer examples](examples/lexer/). |
| Error reporting | Errors show the line, the source, carets under the problem, and a suggestion. |
| CLI | `run`, `build`, `emit`, and `tokens` commands. Generated files go in a temporary directory. |
| Legacy prototype | The original 2024 prototype syntax (`PRINT # "hi"`, `MAKE A VARIABLE x ASSIGN 10`, `FILE OPEN ...`) still compiles and runs end to end. It's being replaced and will be removed once the new parser works. See the [legacy examples](examples/legacy/). |
| Tests | Unit, lexer, program, example, and CLI tests run with `make test`, plus a sanitizer build with `make test-debug`. |

**Coming soon** (Phase 1): the parser and checker for the new syntax, C code generation from the new syntax, and then the language itself: variables, output, decisions, loops, functions, and files. Follow along in the [roadmap](docs/roadmap.md).

## Roadmap

| Phase | Focus | Status |
|---|---|---|
| 1 | **Core language:** variables, arithmetic, text, output, input, decisions, loops, functions, files, friendly errors | In progress (lexer done) |
| 2 | **Data and structure:** records, lists, maps, modules, static types | Coming soon |
| 3 | **Self-hosting:** the EasyScript compiler, written in EasyScript | Coming soon |
| 4 | **Real-world programs:** standard library, C interop, concurrency, backend servers | Coming soon |
| 5 | **Systems mode:** low-level control, no runtime, and an operating system | Coming soon |

Details: [docs/roadmap.md](docs/roadmap.md).

## Design rules

1. **Deterministic parsing, no guessing.** Every sentence has exactly one meaning, or it's an error. The compiler never picks "the most likely" reading.
2. **Ambiguity is an error with suggestions.** If a sentence could mean two things, you're told so, along with the valid ways to write it.
3. **Filler words are ignored.** `the`, `a`, and `an` are dropped, so `set the total to 0.` and `set total to 0.` are the same sentence.
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
./easyscript tokens file.es          # show how the new lexer reads a file
./easyscript run file.es             # compile and run (legacy syntax for now)
./easyscript build file.es -o app    # compile to an executable (legacy syntax for now)
./easyscript emit file.es            # print the generated C (legacy syntax for now)
./easyscript                         # interactive shell (legacy syntax)
```

Try it:

```bash
./easyscript tokens examples/lexer/taste.es
./easyscript run examples/legacy/print_text.es
```

## How the compiler works

```
source.es → lexer → parser → checker → C code generator → cc -O2 → native program
            tokens   syntax    meaning     generated C
                     tree      and names
```

The **front end** (lexer, parser, checker) understands EasyScript and reports errors. The **back end** turns the checked program into C. They only talk through shared data structures (tokens, the syntax tree, and the symbol table). Today the new lexer is finished and the rest of the pipeline is still the legacy prototype. See [docs/architecture.md](docs/architecture.md).

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
