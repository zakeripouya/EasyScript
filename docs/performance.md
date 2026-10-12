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
| `words` | Counts words with a map: two million words picked from a vocabulary of 20,000 (`"w0"` to `"w19999"`, the same picks in every language), each counted with `set the value for word in counts to (the value for word in counts, or 0) plus 1`. Go uses a `map[string]float64`, Python a `dict` with `counts.get(word, 0) + 1` |
| `pipeline` | Functions given to built-ins: a million pseudo-random numbers, ten times over, each doubled (`double applied to every number in numbers`), the ones divisible by three kept (`the numbers in doubled that pass wanted`), those added up (`kept combined using sum, starting from 0`), then each made one bigger (`apply (a function taking n that gives back n plus 1) to every number in numbers`). C calls through function pointers (kept `volatile` so they're really called), Go and Python pass closures to `map`/`filter`/`reduce`-style functions |
| `pipeline_loop` | The same work written as loops in EasyScript (`for each`, `if`, `add`, `set item i of`), to compare the built-ins with a loop you write yourself. EasyScript only |
| `sales` | Reads a csv file of 100,000 sales (written by its `setup.sh`: five columns, every tenth row with a quoted field holding a comma) into a list of rows, and totals the amounts, all together and per region with a map. Python uses `csv.DictReader` and a `dict`. Python reads the file one row at a time; EasyScript keeps every row in the list `read csv file` gives back |
| `sort` | Makes a list of a million pseudo-random numbers (the same in every language) and sorts it, smallest first. C uses the standard library's `qsort`, Go `sort.Float64s`, Python `list.sort()`. Its file `items` (1000000) makes `make bench` add a "bytes/item" column: the peak memory of the whole process divided by the number of items |

The C versions keep their totals `volatile` so the compiler can't remove the loops. The timer's resolution is 10 ms, so times under about 0.05 s are rough.

## Latest results

Machine: Apple M4, 24 GB, macOS 15.3.1. Apple clang 17.0.0, Go 1.27.1, Python 3.14.7. EasyScript 0.1.0 plus reference counting, static types, lists, runtime stack traces, leaner lists, text tools, records, maps, kinds with variants, functions as values, modules and the standard library, 2026-10-11.

| Program | EasyScript | C | Go | Python |
|---|---|---|---|---|
| fib | 0.25 s, 1.2 MB | 0.07 s, 1.2 MB | 0.08 s, 3.8 MB | 2.90 s, 11.5 MB |
| count | 0.05 s, 1.3 MB | 0.03 s, 1.2 MB | 0.02 s, 3.9 MB | 4.02 s, 11.8 MB |
| nested | 0.11 s, 1.2 MB | 0.03 s, 1.2 MB | 0.03 s, 3.8 MB | 5.06 s, 11.3 MB |
| text | 0.00 s, 1.3 MB | (none) | 0.01 s, 13.3 MB | 0.01 s, 11.7 MB |
| sort | 0.06 s, 9.0 MB (9.4 bytes/item) | 0.08 s, 8.8 MB (9.3 bytes/item) | 0.08 s, 28.6 MB (30.0 bytes/item) | 0.31 s, 61.1 MB (64.1 bytes/item) |
| build | 0.00 s, 6.2 MB | 0.00 s, 6.0 MB | 0.00 s, 22.8 MB | 0.05 s, 16.3 MB |
| words | 0.10 s, 3.4 MB | (none) | 0.05 s, 7.9 MB | 0.37 s, 13.4 MB |
| pipeline | 0.13 s, 64.2 MB | 0.05 s, 19.1 MB | 0.05 s, 58.1 MB | 1.28 s, 164.6 MB |
| pipeline_loop | 0.10 s, 16.5 MB | (none) | (none) | (none) |
| sales | 0.04 s, 57.8 MB | (none) | (none) | 0.09 s, 11.8 MB |

## Reading the results

- **Against Python,** EasyScript is 10 to 80 times faster, because it's compiled rather than interpreted.
- **Against C and Go,** loops are now 2.5 to 3.5 times slower, down from up to 30 times. With static types (step 14) the compiler knows every value's kind, so a number is a plain C `double` and `plus` is a C `+`; nothing checks kinds while the program runs. `nested` dropped from 0.85 s to 0.10 s: most of that came from `mod` using the integer remainder for whole numbers (the same answer as `fmod`, much faster), and from counting loops that work out their last round once instead of checking every round.
- **What's left** is the language's own rules. Numbers are doubles, not integers; equal and ordering comparisons allow for tiny rounding errors (`0.1 plus 0.2 is 0.3`); counting works out each number from the start so it never drifts; and every function call checks for runaway recursion so it can stop with a friendly error. `fib` shows the last two most: with both, the C compiler can't turn the recursion into a loop as it does for the C version (removing both made it 0.11 s, the speed of the same program in C with doubles).
- **Stack traces cost about 3% on calls.** Every function call pushes a frame (the function, where it was called, the values it was given) so a runtime error can show the chain of calls. Measured with the fastest of 15 runs, fib went from 235.6 ms to 243.2 ms (+3.2%); count, nested and sort stayed within noise (-0.5%, +1.7%, +0.7%). A first version, a linked list of frames, cost 19% on fib; an array of frames indexed by the depth counter that already existed, with a single input kept in the frame itself, brought it down to three stores per call.
- **Sorting is faster than C's `qsort`, in C's memory.** Numbers are sorted in place with pdqsort, written once as a macro so the comparison is inlined, while `qsort` calls a comparison function for every pair. The sort benchmark's peak went from 16.5 MB (17.3 bytes per item: the stable merge sort it used before needed a second buffer the size of the list) to 9.0 MB (9.4 bytes per item; C is 9.3), at the same speed. Text and sorting by length still use the stable merge sort.
- **Growing by 1.5x was tried and dropped.** On macOS, a realloc that has to move a big block copies it lazily (the new pages share the old ones until written), and with 1.5x growth the last move of the million-item list happened that way, so the sort, the first big write afterwards, paid for the copy: the peak was 14.3 MB. With doubling, realloc grew the block in place every time and the peak stayed at 9.0 MB, so lists still double. (Linux's realloc moves big blocks without copying, so it isn't affected either way.)
- **Building text grows it in place.** `set text to text followed by "word "` used to make a new text each time, copying everything so far, so building a text of n pieces took time in proportion to n squared: the `build` benchmark (a million pieces, 5 MB of text) took 129.7 s and 147 MB. Now, when nothing else holds the text, it grows where it is, doubling its room when it's full, as C's buffer does: about 5 ms (C: 2.5 ms, both including starting the process; timed with the fastest of 20 runs, since `make bench` only sees 10 ms steps) and 6.0 MB, the same peak as C. Text also keeps its count of characters once it's been worked out, so `length of` and the text tools don't count again.
- **Counting words with a map is 4 times faster than Python's `dict`, in under half Go's memory.** Each round looks the word up twice (`the value for word in counts, or 0`, then `set the value for word in counts to ...`), hashing it each time. The map keeps its entries in an array in the order they were added, with a separate index of 4-byte places, so 20,000 entries take about 1 MB. Go is twice as fast.
- **Telling variants apart costs no more than reading a record's field.** Going through a million shapes (circles and squares) 20 times, adding up each one's radius or side with `check which kind s is:` and `its radius`, took 0.13 s (fastest of 10 runs), exactly what the same loop over records took with `if s's round:` and `s's size`; the shapes took 98 MB at the peak, the records 106 MB. A value of a kind is laid out like a record, and `when it's a circle` compares its type pointer, so no separate benchmark was added.
- **Built-ins that take a function cost about 1.4 times a loop you write yourself, and run 9 times faster than Python's `map`, `filter` and `reduce`.** `pipeline` (0.13 s) does the same work as `pipeline_loop` (0.09 s) with four built-ins in each round. Each use becomes a C function that goes through the list; a function defined with `to` is called directly (the C compiler can inline it), and a function value through its closure's code pointer. What's left is the call itself (each pushes a frame for the call stack), and the new list each value form makes: the loop never makes one. A closure that uses names from around it could change the list while it's being gone through, so then the built-in goes through the list as it was; one that uses no names (`a function taking n that gives back n plus 1`) can't, and doesn't pay for that copy (keeping it took the peak from 85 MB to 59 MB). Peak memory on macOS varies a lot from run to run here (36 to 100 MB for the same program), because freed blocks of a few megabytes aren't always given back at once; C and the loop keep fewer lists alive at once.
- **Reading a csv file is twice as fast as Python's `csv` module, in five times its memory.** `read csv file` parses the whole file in C and gives back every row at once, a map per row holding each field as its own text (about 580 bytes a row here); Python's `DictReader` hands over one row at a time, so only one is alive at once. Splitting fields, quotes and line endings are a single pass over the file's bytes.
- **Memory stays flat.** Text is freed as soon as nothing uses it ([how](memory.md)), so building a 50,000-character text by adding to it 10,000 times takes 1.3 MB at its peak, less than Go or Python (it was 280 MB before reference counting).

## Questions about lists against hand-written loops

The English pass's speed rule: every high-level form must take at most 20% longer than the same work written as a loop, and print the same result. `make bench` checks each pair in [`benchmarks/forms/`](../benchmarks/forms/) (`NAME.es` against `NAME_loop.es`) and fails on a mismatch or a form more than 20% slower. Each program builds a million pseudo-random numbers (or 200,000 records), then asks its question many times; the times include building the list.

| Form | Work | Form | By hand | Ratio |
|---|---|---|---|---|
| `how many numbers are above 500` | 300 times over a million numbers | 0.35 s | 0.47 s | 0.74 |
| `any number is above 5000` (no item passes, so every item is looked at) | 300 times | 0.15 s | 0.28 s | 0.54 |
| `the total of numbers` | 300 times | 0.15 s | 0.27 s | 0.56 |
| `the average of numbers` | 300 times | 0.20 s | 0.33 s | 0.61 |
| `the top 10 numbers` (by hand: sorted, then the first 10) | 30 times | 0.03 s | 0.87 s | 0.03 |
| `the 10 most common numbers` (by hand: a map, then sorted) | 30 times | 0.23 s | 0.38 s | 0.61 |
| `the entry with the highest value` | 300 times over 200,000 records | 0.10 s | 0.20 s | 0.50 |
| `the total value of entries per group` (by hand: a map with `, or 0`) | 100 times | 0.29 s | 0.47 s | 0.62 |
| `how many entries whose value is over 500` (the filter in the same pass) | 300 times | 0.10 s | 0.22 s | 0.45 |

Apple M4, macOS 15.3.1, Apple clang 17.0.0, best of 3, 2026-10-12. Every form is faster than its loop: a `for each` loop goes through a copy of the list it can't prove nothing changes (`es_list_snapshot`), while a question that calls no definition can't change the list and reads it directly; the top 10 keep a heap of 10 instead of sorting a million numbers; and `per` finds each group's place in the map once instead of looking it up twice.

## History

One row per milestone, with EasyScript's numbers on that milestone's machine. **Never edit past rows' results; only add new rows**, so the history shows real progress (or regressions). A benchmark added later shows "(none)" in the rows from before it existed.

| Date | Version | Machine | fib | count | nested | text | sort | build | words | pipeline | pipeline_loop | sales |
|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 2026-10-10 | 0.1.0-dev (after `580a056`) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.3 MB | 0.07 s, 1.2 MB | 0.87 s, 1.3 MB | 0.04 s, 280.2 MB | (none) | (none) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 (Phase 1 complete) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.52 s, 1.2 MB | 0.06 s, 1.2 MB | 0.87 s, 1.3 MB | 0.03 s, 280.2 MB | (none) | (none) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 + reference counting (Phase 2, step 13) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.07 s, 1.2 MB | 0.86 s, 1.2 MB | 0.00 s, 1.8 MB | (none) | (none) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 + static types (Phase 2, step 14) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.23 s, 1.2 MB | 0.05 s, 1.2 MB | 0.10 s, 1.2 MB | 0.00 s, 1.6 MB | (none) | (none) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 + lists (Phase 2, step 15) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.23 s, 1.2 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 16.7 MB | (none) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 + stack traces in runtime errors | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.3 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 16.5 MB | (none) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 + leaner lists (pdqsort, reuse, exact sizes) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.6 MB | 0.06 s, 9.0 MB (9.4 bytes/item; was 17.3) | (none) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 + text tools (Phase 2, step 16) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.05 s, 1.2 MB | 0.11 s, 1.2 MB | 0.00 s, 1.3 MB | 0.06 s, 9.0 MB (9.5 bytes/item) | 0.00 s, 6.0 MB (was 129.7 s, 147 MB) | (none) | (none) | (none) | (none) |
| 2026-10-10 | 0.1.0 + records and maps (Phase 2, steps 17 and 18) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.2 MB | 0.05 s, 1.3 MB | 0.11 s, 1.2 MB | 0.00 s, 1.3 MB | 0.06 s, 8.9 MB (9.4 bytes/item) | 0.00 s, 6.0 MB | 0.10 s, 3.6 MB | (none) | (none) | (none) |
| 2026-10-11 | 0.1.0 + kinds with variants and functions as values (Phase 2, steps 19 and 20) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.24 s, 1.3 MB | 0.05 s, 1.3 MB | 0.11 s, 1.3 MB | 0.00 s, 1.3 MB | 0.06 s, 9.0 MB (9.4 bytes/item) | 0.00 s, 6.0 MB | 0.09 s, 3.4 MB | 0.13 s, 59.4 MB | 0.09 s, 16.5 MB | (none) |
| 2026-10-11 | 0.1.0 + modules and the standard library (Phase 2, step 21) | Apple M4, macOS 15.3.1, Apple clang 17.0.0 | 0.25 s, 1.2 MB | 0.05 s, 1.3 MB | 0.11 s, 1.2 MB | 0.00 s, 1.3 MB | 0.06 s, 9.0 MB (9.4 bytes/item) | 0.00 s, 6.2 MB | 0.10 s, 3.4 MB | 0.13 s, 64.2 MB | 0.10 s, 16.5 MB | 0.04 s, 57.8 MB |
