# Security

## Reporting a problem

Please report security issues privately, through GitHub's **Report a vulnerability** button on the repository's Security tab, rather than in a public issue. Include the `.es` file (or the steps), the command you ran, and what happened. You'll get an answer as soon as possible, and a fix will be credited to you unless you'd rather it wasn't.

## What the compiler guarantees

- **No shell.** The compiler never runs a command line through a shell (`system`, `popen`, `sh -c`). It starts the C compiler (`cc`) and, for `run`, the program itself with `fork` and `execvp`, passing each argument separately, so file names and program text are never interpreted as shell syntax.
- **Escaped output.** Everything from the source program that ends up in the generated C is escaped or rebuilt: text values and the source lines kept for runtime errors are written as C string literals with every quote, backslash, `?` and non-printing byte escaped (octal escapes for bytes outside printable ASCII), and names become C identifiers made only of letters, digits and `_`. Comments in the generated C hold only line numbers. A program can't inject C code.
- **Private temporary files.** Each command makes its own directory with `mkdtemp` (under `$TMPDIR`, or `/tmp`), readable only by you, holds the generated C and the program there, and removes it when it finishes, including after errors.
- **No environment hooks in built programs.** The runtime has test hooks that read environment variables (`ES_SKIP_OUTPUT`, `ES_ANSWERS`, `ES_DEBUG_MEMORY`, `ES_MEMORY_LIMIT`). They're compiled in only when the compiler is asked for them (`ES_HOOKS=1` with `easyscript run`, which only the test suite and the interactive shell set). A program made with `easyscript build` never reads them, so nobody running it can make it skip output, read answers from a file, or append to one. `make test` checks this (`cli/no-env-hooks-in-built-programs`).
- **Runtime errors, not crashes.** Generated programs check list and text positions, division by zero, map keys, records that can be nothing, recursion depth, and every allocation, and stop with a plain-English error instead of reading or writing memory they don't own. The test suite runs every program under AddressSanitizer and UndefinedBehaviorSanitizer (`make test-debug`).

## What it doesn't do

- EasyScript programs can read and write any file the user running them can (`write ... to file`, `read file ...`). Don't run a program you don't trust.
- The compiler trusts the C compiler it finds as `cc` on your `PATH`.
