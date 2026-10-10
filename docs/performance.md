# Performance

EasyScript compiles to C and then to native code, so it's meant to be fast. This page records how fast it actually is, measured with `make bench`.

## How it's measured

`make bench` (the script is [`benchmarks/run.sh`](../benchmarks/run.sh)) builds every program in [`benchmarks/`](../benchmarks/) in EasyScript and, where installed, C, Go, and Python. It runs each one three times under `/usr/bin/time`, and reports the fastest run's wall-clock time and peak memory (maximum resident set size). Build time isn't included. Every version must print the same result, or the run fails. `BENCH_RUNS=N make bench` changes the number of runs.

| Program | What it does |
|---|---|
| `fib` | Recursive Fibonacci of 38: about 126 million function calls |
| `count` | A counting loop from 1 to 100 million, adding each number to a total |
| `nested` | Two nested loops of 10,000 each (100 million rounds) with an `if` and `mod` inside |
| `text` | Builds a text of 50,000 characters by joining `"word "` 10,000 times |
| `sort` | Makes a list of a million pseudo-random numbers (the same in every language) and sorts it, smallest first. C uses the standard library's `qsort`, Go `sort.Float64s`, Python `list.sort()` |

The C versions keep their totals `volatile` so the compiler can't remove the loops. The timer's resolution is 10 ms, so times under about 0.05 s are rough.

## Latest results

Machine: Apple M4, 24 GB, macOS 15.3.1. Apple clang 17.0.0, Go 1.27.1, Python 3.14.7. EasyScript 0.1.0 plus reference counting, static types, lists and runtime stack traces, 2026-10-10.

| Program | EasyScript | C | Go | Python |
|---|---|---|---|---|
| fib | 0.24 s, 1.3 MB | 0.07 s, 1.2 MB | 0.08 s, 3.9 MB | 2.88 s, 11.8 MB |
| count | 0.05 s, 1.2 MB | 0.02 s, 1.2 MB | 0.02 s, 3.8 MB | 4.11 s, 11.8 MB |
| nested | 0.11 s, 1.2 MB | 0.03 s, 1.2 MB | 0.03 s, 4.0 MB | 4.94 s, 11.6 MB |
| text | 0.00 s, 1.6 MB | (none) | 0.01 s, 12.5 MB | 0.01 s, 11.5 MB |
| sort | 0.06 s, 16.5 MB | 0.08 s, 8.8 MB | 0.08 s, 34.8 MB | 0.31 s, 62.9 MB |

## Reading the results

- **Against Python,** EasyScript is 10 to 80 times faster, because it's compiled rather than interpreted.
- **Against C and Go,** loops are now 2.5 to 3.5 times slower, down from up to 30 times. With static types (step 14) the compiler knows every value's kind, so a number is a plain C `double` and `plus` is a C `+`; nothing checks kinds while the program runs. `nested` dropped from 0.85 s to 0.10 s: most of that came from `mod` using the integer remainder for whole numbers (the same answer as `fmod`, much faster), and from counting loops that work out their last round once instead of checking every round.
- **What's left** is the language's own rules. Numbers are doubles, not integers; equal and ordering comparisons allow for tiny rounding errors (`0.1 plus 0.2 is 0.3`); counting works out each number from the start so it never drifts; and every function call checks for runaway recursion so it can stop with a friendly error. `fib` shows the last two most: with both, the C compiler can't turn the recursion into a loop as it does for the C version (removing both made it 0.11 s, the speed of the same program in C with doubles).
- **Stack traces cost about 3% on calls.** Every function call pushes a frame (the function, where it was called, the values it was given) so a runtime error can show the chain of calls. Measured with the fastest of 15 runs, fib went from 235.6 ms to 243.2 ms (+3.2%); count, nested and sort stayed within noise (-0.5%, +1.7%, +0.7%). A first version, a linked list of frames, cost 19% on fib; an array of frames indexed by the depth counter that already existed, with a single input kept in the frame itself, brought it down to three stores per call.
- **Sorting is faster than C's `qsort`.** `sort` is a stable merge sort written once as a macro, so each kind of item gets its own copy with the comparison inlined, while `qsort` calls a comparison function for every pair. It uses more memory than C: the list grows by doubling as numbers are added (C allocates exactly a million), and merging needs a second buffer the size of the list.
- **Memory stays flat.** Text is freed as soon as nothing uses it ([how](memory.md)), so building a 50,000-character text by adding to it 10,000 times takes 1.6 MB at its peak, less than Go or Python (it was 280 MB before reference counting).

## History

One row per milestone, with EasyScript's numbers on that milestone's machine. **Never edit past rows' results; only add new rows**, so the history shows real progress (or regressions). A benchmark added later shows "(none)" in the rows from before it existed.

| Date | Version | Machine | fib | count | nested | text | sort |
|---|---|---|---|---|---|---|---|
| 2026-10-10 | 0.1.0-dev (after `580a056`) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.3 MB | 0.07 s, 1.2 MB | 0.87 s, 1.3 MB | 0.04 s, 280.2 MB | (none) |
| 2026-10-10 | 0.1.0 (Phase 1 complete) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.2 MB | 0.06 s, 1.2 MB | 0.87 s, 1.3 MB | 0.03 s, 280.2 MB | (none) |
| 2026-10-10 | 0.1.0 + reference counting (Phase 2, step 13) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.07 s, 1.2 MB | 0.86 s, 1.2 MB | 0.00 s, 1.8 MB | (none) |
| 2026-10-10 | 0.1.0 + static types (Phase 2, step 14) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.23 s, 1.2 MB | 0.05 s, 1.2 MB | 0.10 s, 1.2 MB | 0.00 s, 1.6 MB | (none) |
| 2026-10-10 | 0.1.0 + lists (Phase 2, step 15) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.23 s, 1.2 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 16.7 MB |
| 2026-10-10 | 0.1.0 + stack traces in runtime errors | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.3 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 16.5 MB |
