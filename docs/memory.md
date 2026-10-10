# How EasyScript manages memory

## For people writing EasyScript

You never have to think about memory. There's nothing to write, nothing to free, and nothing to close.

Text made while a program runs (joining with `followed by`, turning numbers into text, answers to `ask`, the contents of a file) takes up memory only while something still uses it. As soon as nothing does, the memory is given back, right then, not at some later clean-up. So a loop that makes a new text a million times uses no more memory than one that makes it once:

```
let last be ""
repeat 1000000 times:
    set last to "item " followed by it as text
say last
```

Only the latest text is kept; every earlier one is freed the moment `last` stops holding it. Numbers, yes and no, and `nothing` don't take extra memory at all, and text written in your program (like `"item "`) is part of the program itself.

Two things to know:

- **Text you keep growing really does grow.** Adding to the same text over and over (`set story to story followed by more`) keeps the whole text, as it should. A 50,000-character text takes about 50 KB.
- **Memory goes back to the computer when the program ends,** including when it stops with an error or with `stop the program`.

## For contributors

### Reference counting

Every value is an `EsValue` (`runtime/es_value.h`): a kind (nothing, number, text, yes/no) and, in a union, a number, a yes/no, or a pointer to a text object. That's 16 bytes, so values travel in registers.

Text lives in a **text object**. Every heap object starts with the same small header, a kind and a reference count, so the lists, records, and closures planned for Phase 2 will reuse the same machinery (`es_retain`, `es_release`, `es_free_object`):

```c
typedef struct { EsObjectKind kind; size_t refs; } EsObject;
typedef struct { EsObject header; size_t len; const char *chars; } EsTextObject;
```

- **Heap text** (made while the program runs) starts with a count of 1. Its characters sit right after the object, in the same allocation.
- **Static text** (literals in the program, text constants, and the runtime's own words like `yes` and `infinity`) is a static `EsTextObject` whose count is `ES_IMMORTAL`. Retain and release skip it, so it's never counted or freed.

### Ownership

One rule keeps the generated code simple: **every value is owned by exactly one place, and whoever receives a value is responsible for it.**

| Situation | Rule |
|---|---|
| An expression | Produces an *owned* value (a new reference). |
| Reading a variable | Retains it (`es_retain(es_v_name)`): the reader gets its own reference. |
| A text literal or constant | Immortal, so it's used directly, with no retain. |
| A runtime operation (`es_join`, `es_eq`, `es_say`, `es_length`, ...) | **Consumes** its arguments: it releases them, or hands them back in its result. |
| Calling an EasyScript function | The function **owns** its arguments and releases them when it ends. |
| `give back` | Returns an owned value. It's worked out first, then the function releases its inputs and names. |
| Storing into a variable (`let`, `set`, `add ... to`, `ask`, `read file`) | `es_set(&var, value)` takes the new value and releases the old one *after* the new one is worked out, so `set t to t followed by "x"` is safe. |
| A block ends | The names first made in it are released (`es_drop`). |
| `stop the loop`, `skip this one` | Release the names of every block they leave. |
| `give back`, or the end of a function | Release all of the function's inputs and names. |
| The end of the program | Releases every variable of `main()`. `stop the program` in the main program jumps there (`goto es_end`). Inside a function, it releases that function's names and exits. |

Because operations consume their arguments, nested expressions need no bookkeeping: in `"a" followed by x followed by "c"`, the inner join's result is consumed (and freed) by the outer join. Temporaries are never released by hand.

Arithmetic, `not`, `or`, conditions and counting only succeed on numbers and yes/no values, which own nothing, so they don't release anything. Anything else stops the program with an error.

The variables of `main()` are C locals of `main()`, not globals. Their addresses never escape, so the C compiler can keep numbers in registers even though a release might call `free()`. That keeps loops as fast as they were before reference counting.

### What isn't cleaned up

- **Runtime errors** print their message and exit straight away, in the middle of a sentence. The operating system reclaims everything.
- **`stop the program` inside a function** can't reach the variables of `main()` or of the functions that called it, or values they were still working out. Again, the operating system reclaims them as the program ends.

### Cycles

A reference count can't free objects that refer to each other in a loop. With text as the only heap kind, that can't happen yet. When records arrive in Phase 2, EasyScript will need weak references or a cycle collector (see the [roadmap](roadmap.md)).

### Checking it

- **`ES_DEBUG_MEMORY=1`**: every generated program counts its live heap objects. With this set, the program fails at the end (exit 70) if any are still alive: `Memory check: 1 text value was still alive at the end of the program (39 bytes).` `make test` runs every program this way, and a control test in `tests/run.sh` checks that the check really catches a missing release.
- **`ES_MEMORY_LIMIT=N`**: fail (exit 70) as soon as live heap text takes more than N bytes. `tests/memory/` runs loops of up to a million rounds under a 100,000-byte limit, and also checks that the program's peak memory stays under 16 MB where `/usr/bin/time` can measure it.
- **`ES_SANITIZE=1`**: the compiler builds programs with AddressSanitizer and UBSan, so a use after free or a double free stops with a report. `make test-debug` sets it for the whole suite.
- **`make test-debug`** also checks for leaks: LeakSanitizer on Linux, and on macOS `tests/leaks.sh` runs every test program (and the compiler) under the system `leaks` tool.

### Adding a heap kind

1. Add an `ES_OBJ_...` kind and a struct that starts with `EsObject header`.
2. Free it (and release anything it holds) in `es_free_object`.
3. Make `es_counted` recognise values of the new kind.
4. Follow the ownership table: operations consume their arguments, and storing goes through `es_set`.
