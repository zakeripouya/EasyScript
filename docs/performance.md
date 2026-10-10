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

The C versions keep their totals `volatile` so the compiler can't remove the loops. The timer's resolution is 10 ms, so times under about 0.05 s are rough.

## Latest results

Machine: Apple M4, 24 GB, macOS 15.3.1. Apple clang 17.0.0, Go 1.27.1, Python 3.14.7. EasyScript 0.1.0, 2026-10-10.

| Program | EasyScript | C | Go | Python |
|---|---|---|---|---|
| fib | 0.52 s, 1.2 MB | 0.07 s, 1.2 MB | 0.08 s, 3.8 MB | 2.78 s, 11.3 MB |
| count | 0.06 s, 1.2 MB | 0.02 s, 1.2 MB | 0.02 s, 3.9 MB | 4.12 s, 11.3 MB |
| nested | 0.87 s, 1.3 MB | 0.03 s, 1.2 MB | 0.03 s, 3.9 MB | 5.09 s, 11.4 MB |
| text | 0.03 s, **280.2 MB** | (none) | 0.01 s, 13.7 MB | 0.01 s, 11.5 MB |

## Reading the results

- **Against Python,** EasyScript is 5 to 60 times faster on loops and function calls, because it's compiled rather than interpreted.
- **Against C and Go,** it's 3 to 30 times slower. In Phase 1 every value is *tagged*: each `plus`, comparison, or `mod` is a runtime call that checks what kind of value it has before doing the work, so it can give a friendly error. `nested` shows this most, with several of those calls per round. **Static types arrive in Phase 2**: when the compiler knows a value is always a number, it can generate plain C arithmetic, which is where the gap to C should mostly close.
- **Memory is the known weak spot.** Text made while a program runs is only freed when the program ends. Building a 50,000-character text by adding to it 10,000 times keeps every in-between version, about 280 MB, where Go and Python use under 14 MB. Loops and recursion that don't build text use almost no memory. Reclaiming memory while a program runs is on the [roadmap](roadmap.md).

## History

One row per milestone, with EasyScript's numbers on that milestone's machine. **Never edit past rows; only add new ones**, so the history shows real progress (or regressions).

| Date | Version | Machine | fib | count | nested | text |
|---|---|---|---|---|---|---|
| 2026-10-10 | 0.1.0-dev (after `580a056`) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.3 MB | 0.07 s, 1.2 MB | 0.87 s, 1.3 MB | 0.04 s, 280.2 MB |
| 2026-10-10 | 0.1.0 (Phase 1 complete) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.2 MB | 0.06 s, 1.2 MB | 0.87 s, 1.3 MB | 0.03 s, 280.2 MB |
