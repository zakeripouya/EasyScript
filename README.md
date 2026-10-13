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
say "Hello, [name]! Your total is [total].".
```

```
$ ./easyscript run hello.es
Hello, Ada! Your total is 15.
```

## A bigger example

A loop and a function, also running today:

```
note: Count down from 3, then greet someone.
count down from 3 to 1:
    say the number.
say "Liftoff!".

to greet some name:
    say "Hello,", the name, "!".

greet "Ada".
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
| Sentences | End with a period, `?`, `!` or a new line; a line ending with a comma, `then` or `and` goes on to the next line |
| Variables | `let total be 0`, `set`/`change ... to`, and `add 5 to total`, `subtract`, `increase`/`decrease`/`multiply`/`divide ... by` |
| Names | A word (`total`, `clientName`) or several (`let late fee be 25`, `keep hourly rate as 150`, `to add tax to amount:`), recognized everywhere once made; matched without caring about capitals |
| Constants | `keep tax rate as 0.2`, fixed lists (`keep weekdays as a list of "Mon", "Tue" and "Wed"`) and fixed maps (`keep prices as a map of "coffee" to 3, "tea" to 2`): worked out by the compiler, usable everywhere (including inside functions), and never changed |
| Values and expressions | Numbers, text, yes/no; arithmetic in words or symbols (and `half of`, `twice`, `20 percent of`, `the remainder of 17 divided by 5`, `x divided by 2, rounded down`, `is infinite`); comparisons (`is at least`, `>=`, ...); `and`/`or`/`not`; joining text with commas in `say` (`say "Hello,", name`), brackets (`"Hello, [name]!"`) or `followed by`; `as a number`, `as text`, `as exact text`, `length of`, `contents of file` |
| Output and input | `say` (or `print`, `show`, `display`, `write`), `ask "..." and call the answer name` |
| Files | `write ... to file` (or `put ... in file`), `append ... to file` (or `add ... to the end of file`), `read file ... and call it ...`, `file ... exists` |
| Several files | `use helpers.` reads `helpers.es`; files that are used only define things, `privately` keeps a definition to its own file, `helpers's double` says whose; errors name the file ([chapter](docs/language-guide.md#17-programs-in-several-files)) |
| Standard library | Math (`the square root of`, `to the power of`, `rounded to 2 decimal places`, `round price to 2 decimals.`, `pi`), random numbers (`a random number from 1 to 6`, `pick one of`, `shuffle`, `roll a die`), dates (`today`, `the time now`, `3 days ago`, `today's weekday`, `is before`, `is in this month`, kept in lists, maps and records), `wait 2 seconds`, files and folders (`make a folder called`, `rename`/`copy`/`move`/`delete the file`, `the files in`), csv files (`read csv file ... and call it rows`, `save ... as csv file`) and `clear the screen` ([stdlib.md](docs/stdlib.md)) |
| Decisions | `if ... :` with indented blocks, `otherwise if`, `otherwise` (or `else`), and one-line `if x is 5, say "hi".` / `if x is 5 then say "hi".`; `unless`; leaving a function early with `give back 0 if ...` and `stop here if ...` |
| Loops | `count from 1 to 10:` (`count down from`, `by`/`in steps of`, `as n`), `for each` (and `for each score:` for the list `scores`), `repeat 3 times:`, `while`/`as long as`, `keep doing this until`, `forever`, with `stop the loop` and `skip this one`; `it` is the loop's number |
| Lists | `a list of 72, 95 and 88`, `an empty list of text`, `item 2 of scores`, `the first item in`/`the last item of`; `add 5 to the end of scores`, `insert`, `remove item 2 from`, `set item 2 of scores to 100`, `sort` (also `biggest first`, `by their length`); `contains`, `is empty`, `the number of items in`; `for each score in scores:`; slices (`items 2 to 4 of scores`, `the first 3 items in`, `the last 2 items in`, `scores without the first item`, `scores without item 2`, `every second item of`); `each score in scores plus 5`, `the scores that are at least 70`, `the dogs whose age is over 3`, `the scores where it is below 50`, `added together`, `the biggest of`, `reversed`, `scores sorted`, `a copy of`, joining (`morning slots followed by evening slots`); nested lists; printed in English ("72, 95 and 88") |
| Questions and totals | `how many scores are above 70`, `how many guests there are`, `if any score is above 90`, `if every order's amount is above 0`, `if no score is below 0`, `all scores are ...`, `the total of scores`, `the average age of dogs`, `the median of scores`, `the top 3 scores`, `the lowest 2 orders by amount`, `the 10 most common words`, `the customer with the highest total`, `the total amount of orders per region` (and `per month`), maps sorted by their values; each one pass over the list, stopping early where it can, shown by `easyscript plan` ([chapter](docs/language-guide.md#21-questions-and-totals)) |
| Choices without if | `the nickname, or "friend" if there isn't one`, `add 1 to the value for word in counts, starting at 0`, `a customer is loyal if ...` then `if rex is loyal` and `the loyal customers`, `an order's profit is its amount minus its cost.`, `the best customer is the one with the highest profit.`, tables (`the shipping cost depends on the total:` / `5 when it's over 50.` / `10 otherwise.`), rules (`an age must be between 0 and 150.`), `remove the tasks that are done.`, `set every monster's color to "blue".`, events (`every 2 seconds:`, `every Monday at 9am:`, `whenever score is over 100:`, `whenever a key is pressed:`) ([chapter](docs/language-guide.md#22-choices-without-if)) |
| Talking to it | `what is total?`, `is total above 100?`, `which customer has the highest total?`, `show me the big orders`, `can you ...`, steps in a row (`read the table in "sales.csv", then total the amount per month, then sort it biggest first, then save it as "best months.csv"`), tables read from csv files with their columns checked, `ask for a number`, `as money`, `the total is always ...`, `try to ... if that fails`, `the text below:`, examples as tests (`for example, double with 3 is 6.`, `easyscript test`), `as a table`, `sorted by amount`, charts (`save a bar chart of sales per month as "sales.svg"`, drawn by a module written in EasyScript) ([chapter](docs/language-guide.md#23-talking-to-it)) |
| Maps | `a map of "coffee" to 3, "tea" to 2`, `an empty map from text to numbers` (keys are text or numbers, values any kind); `set the value for "Paris" in ages to 19`, `the value for "Paris" in ages` (a missing key is a friendly runtime error naming it), `the value for "Bob" in ages, or 0`; `ages has "Paris"`, `the number of entries in ages`, `is empty`, `the keys of ages`, `the values of ages`; `remove "Paris" from ages`, `ages without "Paris"`, `add 1 to the value for "Paris" in ages`; `for each name and age in ages:`, always in the order keys were added; maps of lists, lists of maps, maps in records; `a copy of`; a fast hash table; printed readably (`"Paris" is 19 and "Junie" is 24`) |
| Records | `a dog has a name (text) and an age (a number).` (kinds can be left out and are worked out from use; `a dog is a kind of thing with:` works too), `a new dog with name "Rex" and age 3`, `rex's name`, fields that can be nothing, methods (`a dog can bark:`, or `to bark, for a dog:`) |
| Text | Characters with the list forms (`character 3 of word`, `the first 3 characters of word`, `characters 2 to 5 of word`, `word without the first character`, `every other character of word`, `the characters of word`), counted as Unicode characters (`"café"` has 4); `contains`, `starts with`, `ends with`, `the position of "x" in word` (a number, or nothing if it isn't there); `the words of`, `the lines of`, `split by ","`, `joined with ", "`; `in uppercase`, `in lowercase`, `without extra spaces`, `with "a" replaced by "b"`; `is a letter`, `is a digit`, `is a space`, `the code of`, `the character with code 65`; text built with `set line to line followed by ...` grows in place |
| Functions | `to make tea:` / `to greet with someone:` / `to area with width and height:` / `to add tax to amount:` / `to pause for seconds:` / `to busiest artist from names:`, `give back` / `return`, calls as sentences (`greet "Paris".`, `call greet with "Paris".`, `add tax to 100.`) or values (`area of 3 and 4`); recursion, and calling a function before it's defined |
| Functions as values | `the function double`, `a function taking x that gives back x times 2` (or with a block below it), `call f with 3` / `f with 3`, kinds like `a function taking a number and giving back text`, functions in lists, maps and records, given and given back; functions made inside others share the names they use (a counter keeps counting); built-ins that take one: `double applied to every number in scores`, `the numbers in scores where it is above 50`, `... that pass even`, `scores combined using sum, starting from 0`, `names sorted by length`, and the sentences `apply ...`, `keep only ...`, `sort ... by ...` |
| Interfaces | `something that can speak is anything with:` and ways like `a way to speak, giving back text.`; any kind of record (or with variants) with matching methods fits, without saying so; `(something that can speak)` as an input, `an empty list of things that can speak` holding dogs, cats and birds together, and `speak animal` running each one's own method |
| Kinds with variants | `a point is made of an x (a number) and a y (a number).` (values compared by their fields); `a shape is one of:` with variants like `a circle with a radius (a number).` (recursive kinds too), `a new circle with radius 3`, `check which kind s is:` with `when it's a circle:` (a missing variant is a compile error), `if s is a circle:`, and fields used only once the variant is checked |
| Types | Every value is a number, text, yes/no, a list of one kind, a record, a value of a kind with variants, or a map (and anything but a list or map can be "or nothing", checked before use) with one kind of key and value, worked out by the compiler (no annotations needed); kind mistakes are caught before the program runs; optional kinds in definitions: `to area with width (a number) and height (a number), giving back a number:`, `(a list of numbers)`, `(a map from text to numbers)` |
| System tools | `the words given to the program` (`easyscript run greet.es Ada` or `./greet Ada`), `word 1 given to the program`, `the text sent to the program` and `the lines sent to the program` (piped input), `the setting called "HOME"` (text or nothing), `run "git" with "status" and call the result r` with `r's output`, `r's error output`, `r's exit code` and `if r worked:` (never through a shell), `complain "..."` (the error output), `stop the program, saying it failed` and `stop the program with code 3` ([stdlib.md](docs/stdlib.md#system-tools)) |
| Keyboard | `wait for a key and call it key` and `the key being pressed` (or nothing), a key at a time without Enter, with plain names (`"up"`, `"enter"`, `"escape"`, `"a"`); the terminal is always put back, even after Ctrl+C; macOS and Linux. Snake example ([chapter](docs/language-guide.md#20-the-keyboard)) |
| Other | `stop the program`; any sentence can start with `please` (also after `then`, `first`, `next`) |
| Memory | Nothing to manage: text, lists, records and maps are freed as soon as nothing uses them, so loops that make them run in flat memory ([how](docs/memory.md)) |
| Friendly errors | Compile errors show the line, the source, carets, and a suggestion ("Did you mean "total"? You made it on line 1."); a file's name without quotes, or a request EasyScript can't do yet (email, web pages, the internet, Excel files), gets one plain error saying so. Kind mistakes are compile errors too ("I can't add text to a number."). Runtime errors show the line with carets too, and the chain of function calls that led there, with the values they were given ("while working out "average" (count was 0)", "called from line 8: ...") |
| Tools | `run`, `build`, `emit`, `tokens`, `ast`, `errors`, `plan` (what each list question became), and an interactive shell. Generated programs are self-contained C. |
| Tests | Unit, lexer, parser, program, runtime-error, warning, memory, example, and CLI tests with `make test` (every program also runs with a memory check), plus a sanitizer build with `make test-debug`; GitHub Actions runs both on Linux (GCC) and macOS (Clang) on every push |

See the runnable [example programs](examples/programs/).

**Phase 2 has started.** Done so far: **memory management** (text and lists are freed as soon as nothing uses them, so long loops run in flat memory; [how it works](docs/memory.md)), **static types** (the compiler works out whether every value is a number, text, yes/no, or a list, catches kind mistakes before the program runs, and generates plain C numbers; [Types](docs/language-guide.md#9-types)), **lists** (shared by reference, with items by position from 1, `for each`, `every`/`only`, sorting, and friendly errors; [Lists](docs/language-guide.md#10-lists)), **text tools** (characters, searching, splitting into words and lines, joining, changing case, character codes, and fast text building; [Text](docs/language-guide.md#11-text)), and **records** (`a dog has a name (text) and an age (a number).`, or `a dog is a kind of thing with:` and its fields, `a new dog with name "Rex" and age 3`, `rex's name`, fields that can be nothing and must be checked first, and methods like `to bark, for a dog:`, which different kinds of record can share by name; [Records](docs/language-guide.md#12-records)), and **maps** (lookup tables from text or number keys to any kind of value, in the order keys were added, with `the value for "Paris" in ages, or 0` for a default and fixed maps made with `keep`; [Maps](docs/language-guide.md#13-maps)), and **kinds with variants** (`a shape is one of:` a circle, a rectangle or a point, each with its own fields, told apart with `check which kind s is:`, where every variant must be covered; [Kinds with variants](docs/language-guide.md#14-kinds-with-variants)). Runtime errors now show the source line and the chain of calls. Programs can now be split into **several files** (`use helpers.`; [chapter](docs/language-guide.md#17-programs-in-several-files)), and a **standard library** works in every program: math, random numbers, dates, waiting, files and folders (including `file ... exists`, moved from Phase 1), csv files and clearing the screen ([stdlib.md](docs/stdlib.md)). Programs can use the **words they're started with**, piped input, settings, other programs (never through a shell) and exit codes, and read the **keyboard** a key at a time, as in the Snake example ([chapter 19](docs/language-guide.md#19-programs-and-the-system), [chapter 20](docs/language-guide.md#20-the-keyboard)); a JSON parser written in EasyScript tests the language on a real program. The **English pass** made programs read like plain English: questions and totals (`how many orders are big`), choices without `if`, steps in a row, tables read from csv files, charts, and definitions with their inputs in the middle (`give every loyal customer a discount of 10 percent.`); see [english-pass.md](docs/english-pass.md). **Coming next:** the rest of "reads like text", user testing, then v0.2.0, and weak references for records that point to each other. Follow along in the [roadmap](docs/roadmap.md).

## Performance

Time (and peak memory) for each benchmark, measured with `make bench`:

| Program | EasyScript | C | Go | Python |
|---|---|---|---|---|
| Recursive fib(38) | 0.25 s | 0.07 s | 0.08 s | 2.90 s |
| Count to 100 million | 0.05 s | 0.03 s | 0.02 s | 4.04 s |
| Nested loops (100 million rounds) | 0.11 s | 0.03 s | 0.03 s | 4.99 s |
| Build a 50,000-character text | 0.00 s, 1.3 MB | (none) | 0.01 s, 12 MB | 0.01 s, 12 MB |
| Sort a million numbers | 0.06 s, 9.0 MB | 0.08 s (`qsort`), 8.8 MB | 0.08 s, 34 MB | 0.31 s, 65 MB |
| Add to a text a million times | 0.00 s, 6.0 MB | 0.00 s, 6.0 MB | 0.00 s, 22 MB | 0.04 s, 16 MB |
| Count two million words with a map | 0.10 s, 3.6 MB | (none) | 0.05 s, 8.0 MB | 0.37 s, 14 MB |
| Map, filter and reduce a million numbers, ten times | 0.13 s, 61 MB (a hand-written loop: 0.09 s) | 0.05 s, 19 MB | 0.04 s, 59 MB | 1.22 s, 180 MB |
| Read a csv file of 100,000 rows and total it | 0.04 s, 60 MB | (none) | (none) | 0.09 s (`csv` module), 12 MB |

Apple M4, macOS 15.3.1, EasyScript 0.1.0 plus Phase 2 through step 22 and the English pass, 2026-10-12. Every question about a list (`how many`, `the total of`, `the top 10`, `per region`, `sorted by amount`, ...) runs at least as fast as the same work written as a loop. The compiler knows every value's kind, so numbers are plain C doubles and loops run close to C. Sorting beats C's `qsort` because the comparison is inlined instead of called through a function pointer, and sorts numbers in place, in the same memory as C. Text that only one variable holds grows in place, so adding to it a million times takes about 5 ms, in the same memory as C (it took over two minutes before). Counting words with a map is 4 times faster than Python's dictionaries, in less memory than Go. Recursion is slower than C because every call checks for runaway recursion and records itself for runtime errors' call chain (so it can stop with a friendly error that says how it got there), and numbers compare with a small tolerance. Details and history: [docs/performance.md](docs/performance.md).

## Roadmap

| Phase | Focus | Status |
|---|---|---|
| 1 | **Core language:** variables, constants, arithmetic, text, output, input, decisions, loops, functions, files, friendly errors | **Complete** (v0.1.0) |
| 2 | **Data and structure:** memory management, static types, records, lists, maps, kinds with variants, functions as values, interfaces, modules, text tools | **In progress** (memory management, static types, lists, text tools, records, maps, kinds with variants, functions as values, interfaces, modules, a standard library start, system tools and the keyboard done) |
| 3 | **Self-hosting:** the EasyScript compiler, written in EasyScript | Coming soon |
| 4 | **Real-world programs:** standard library, C interop, concurrency, backend servers | Coming soon |
| 5 | **Systems mode:** low-level control, no runtime, and an operating system | Coming soon |

Also planned: **EasyScript Notebook** (after Phase 1, growing with Phase 4). It's a chat-style notebook where each message is a statement that runs immediately, with results (text, tables, charts) shown as replies. Messages that don't parse get friendly suggestions. An optional AI fallback writes EasyScript for vague requests and always shows the code before running it. A whole conversation exports as a runnable `.es` file. It will ship as a Jupyter kernel first, so it also works in Jupyter and VS Code.

Details: [docs/roadmap.md](docs/roadmap.md). The **English pass** (plain-English questions, totals, choices without `if`, text templates) is done, and "reads like text" is in progress; both are in [docs/english-pass.md](docs/english-pass.md).

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
make test-gcc     # builds with GCC (-Werror) and runs the full suite with it
make bench        # times the benchmarks against C, Go, and Python
```

### Commands

```bash
./easyscript run file.es             # compile and run
./easyscript test file.es            # run, then say whether its examples ("for example, ...") are right
./easyscript build file.es -o app    # compile to an executable
./easyscript emit file.es            # print the generated C
./easyscript tokens file.es          # show how the lexer reads a file
./easyscript ast file.es             # show the syntax tree
./easyscript errors file.es          # check a file and print its errors as data (JSON, one per line)
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
