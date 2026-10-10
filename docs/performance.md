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
| `build` | Builds a text of 5 million characters by adding `"word "` to it a million times (`set text to text followed by "word "`). C grows a buffer by doubling, Go uses `strings.Builder`, Python `text += "word "` inside a function (at the top level CPython copies every time: over two minutes) |
| `sort` | Makes a list of a million pseudo-random numbers (the same in every language) and sorts it, smallest first. C uses the standard library's `qsort`, Go `sort.Float64s`, Python `list.sort()`. Its file `items` (1000000) makes `make bench` add a "bytes/item" column: the peak memory of the whole process divided by the number of items |

The C versions keep their totals `volatile` so the compiler can't remove the loops. The timer's resolution is 10 ms, so times under about 0.05 s are rough.

## Latest results

Machine: Apple M4, 24 GB, macOS 15.3.1. Apple clang 17.0.0, Go 1.27.1, Python 3.14.7. EasyScript 0.1.0 plus reference counting, static types, lists, runtime stack traces, leaner lists and text tools, 2026-10-10.

| Program | EasyScript | C | Go | Python |
|---|---|---|---|---|
| fib | 0.24 s, 1.2 MB | 0.07 s, 1.2 MB | 0.08 s, 4.0 MB | 2.83 s, 11.6 MB |
| count | 0.05 s, 1.2 MB | 0.03 s, 1.2 MB | 0.02 s, 3.9 MB | 4.07 s, 11.7 MB |
| nested | 0.11 s, 1.2 MB | 0.03 s, 1.2 MB | 0.03 s, 3.8 MB | 5.07 s, 11.7 MB |
| text | 0.00 s, 1.3 MB | (none) | 0.01 s, 11.9 MB | 0.01 s, 11.7 MB |
| sort | 0.06 s, 9.0 MB (9.5 bytes/item) | 0.08 s, 8.8 MB (9.3 bytes/item) | 0.08 s, 33.8 MB (35.5 bytes/item) | 0.32 s, 62.7 MB (65.7 bytes/item) |
| build | 0.00 s, 6.0 MB | 0.00 s, 6.0 MB | 0.00 s, 18.6 MB | 0.04 s, 16.3 MB |

## Reading the results

- **Against Python,** EasyScript is 10 to 80 times faster, because it's compiled rather than interpreted.
- **Against C and Go,** loops are now 2.5 to 3.5 times slower, down from up to 30 times. With static types (step 14) the compiler knows every value's kind, so a number is a plain C `double` and `plus` is a C `+`; nothing checks kinds while the program runs. `nested` dropped from 0.85 s to 0.10 s: most of that came from `mod` using the integer remainder for whole numbers (the same answer as `fmod`, much faster), and from counting loops that work out their last round once instead of checking every round.
- **What's left** is the language's own rules. Numbers are doubles, not integers; equal and ordering comparisons allow for tiny rounding errors (`0.1 plus 0.2 is 0.3`); counting works out each number from the start so it never drifts; and every function call checks for runaway recursion so it can stop with a friendly error. `fib` shows the last two most: with both, the C compiler can't turn the recursion into a loop as it does for the C version (removing both made it 0.11 s, the speed of the same program in C with doubles).
- **Stack traces cost about 3% on calls.** Every function call pushes a frame (the function, where it was called, the values it was given) so a runtime error can show the chain of calls. Measured with the fastest of 15 runs, fib went from 235.6 ms to 243.2 ms (+3.2%); count, nested and sort stayed within noise (-0.5%, +1.7%, +0.7%). A first version, a linked list of frames, cost 19% on fib; an array of frames indexed by the depth counter that already existed, with a single input kept in the frame itself, brought it down to three stores per call.
- **Sorting is faster than C's `qsort`, in C's memory.** Numbers are sorted in place with pdqsort, written once as a macro so the comparison is inlined, while `qsort` calls a comparison function for every pair. The sort benchmark's peak went from 16.5 MB (17.3 bytes per item: the stable merge sort it used before needed a second buffer the size of the list) to 9.0 MB (9.4 bytes per item; C is 9.3), at the same speed. Text and sorting by length still use the stable merge sort.
- **Growing by 1.5x was tried and dropped.** On macOS, a realloc that has to move a big block copies it lazily (the new pages share the old ones until written), and with 1.5x growth the last move of the million-item list happened that way, so the sort, the first big write afterwards, paid for the copy: the peak was 14.3 MB. With doubling, realloc grew the block in place every time and the peak stayed at 9.0 MB, so lists still double. (Linux's realloc moves big blocks without copying, so it isn't affected either way.)
- **Building text grows it in place.** `set text to text followed by "word "` used to make a new text each time, copying everything so far, so building a text of n pieces took time in proportion to n squared: the `build` benchmark (a million pieces, 5 MB of text) took 129.7 s and 147 MB. Now, when nothing else holds the text, it grows where it is, doubling its room when it's full, as C's buffer does: about 5 ms (C: 2.5 ms, both including starting the process; timed with the fastest of 20 runs, since `make bench` only sees 10 ms steps) and 6.0 MB, the same peak as C. Text also keeps its count of characters once it's been worked out, so `length of` and the text tools don't count again.
- **Memory stays flat.** Text is freed as soon as nothing uses it ([how](memory.md)), so building a 50,000-character text by adding to it 10,000 times takes 1.3 MB at its peak, less than Go or Python (it was 280 MB before reference counting).

## History

One row per milestone, with EasyScript's numbers on that milestone's machine. **Never edit past rows' results; only add new rows**, so the history shows real progress (or regressions). A benchmark added later shows "(none)" in the rows from before it existed.

| Date | Version | Machine | fib | count | nested | text | sort | build |
|---|---|---|---|---|---|---|---|---|
| 2026-10-10 | 0.1.0-dev (after `580a056`) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.3 MB | 0.07 s, 1.2 MB | 0.87 s, 1.3 MB | 0.04 s, 280.2 MB | (none) | (none) |
| 2026-10-10 | 0.1.0 (Phase 1 complete) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.2 MB | 0.06 s, 1.2 MB | 0.87 s, 1.3 MB | 0.03 s, 280.2 MB | (none) | (none) |
| 2026-10-10 | 0.1.0 + reference counting (Phase 2, step 13) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.07 s, 1.2 MB | 0.86 s, 1.2 MB | 0.00 s, 1.8 MB | (none) | (none) |
| 2026-10-10 | 0.1.0 + static types (Phase 2, step 14) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.23 s, 1.2 MB | 0.05 s, 1.2 MB | 0.10 s, 1.2 MB | 0.00 s, 1.6 MB | (none) | (none) |
| 2026-10-10 | 0.1.0 + lists (Phase 2, step 15) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.23 s, 1.2 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 16.7 MB | (none) |
| 2026-10-10 | 0.1.0 + stack traces in runtime errors | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.3 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 16.5 MB | (none) |
| 2026-10-10 | 0.1.0 + leaner lists (pdqsort, reuse, exact sizes) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 9.0 MB (9.4 bytes/item; was 17.3) | (none) |
| 2026-10-10 | 0.1.0 + text tools (Phase 2, step 16) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.3 MB | 0.06 s, 9.0 MB (9.5 bytes/item) | 0.00 s, 6.0 MB (was 129.7 s, 147 MB) |
