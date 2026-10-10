# How EasyScript manages memory

## For people writing EasyScript

You never have to think about memory. There's nothing to write, nothing to free, and nothing to close.

Text made while a program runs (joining with `followed by`, turning numbers into text, answers to `ask`, the contents of a file) and lists take up memory only while something still uses them. As soon as nothing does, the memory is given back, right then, not at some later clean-up. So a loop that makes a new text a million times uses no more memory than one that makes it once:

```
let last be ""
repeat 1000000 times:
    set last to "item " followed by it as text
say last
```

Only the latest text is kept; every earlier one is freed the moment `last` stops holding it. Lists work the same way: a list is freed, with everything in it, when no variable (and no other list) holds it any more. Numbers, yes and no, and `nothing` don't take extra memory at all, and text written in your program (like `"item "`) is part of the program itself.

Two things to know:

- **Text you keep growing really does grow.** Adding to the same text over and over (`set story to story followed by more`) keeps the whole text, as it should. A 50,000-character text takes about 50 KB. The same goes for a list you keep adding to: a million numbers take about 8 MB.
- **Memory goes back to the computer when the program ends,** including when it stops with an error or with `stop the program`.

## For contributors

### Reference counting

The compiler knows the kind of every value ([types](language-guide.md#9-types)), so generated code uses plain C types: a number is a `double`, yes/no is a `bool`, text is an `EsText *`, and a list is an `EsList *` (`runtime/es_memory.h`). Numbers and yes/no values own nothing, so **only text and lists are reference counted**; no retain or release is ever generated for anything else.

Every heap object starts with the same small header, a kind and a reference count, so text, lists, and the records and closures planned for later share the same machinery (`es_object_release`, `es_free_object`, with typed wrappers `es_retain`/`es_release` for text and `es_list_retain`/`es_list_release` for lists):

```c
typedef struct { EsObjectKind kind; size_t refs; } EsObject;
typedef struct { EsObject header; size_t len; const char *chars; } EsText;
typedef struct EsList { EsObject header; EsItems items; size_t len, cap; union { double *n; ... } at; } EsList;
```

- **Heap text** (made while the program runs) starts with a count of 1. Its characters sit right after the object, in the same allocation.
- **Static text** (literals in the program, text constants, and the runtime's own words like `yes` and `infinity`) is a static `EsText` whose count is `ES_IMMORTAL`. Retain and release skip it, so it's never counted or freed.
- **A list** holds its items in an array that grows (doubling), allocated separately from the `EsList`. Its items are all one kind (`EsItems`: numbers, yes/no, text, or lists); text and lists in it are references the list owns, released when the item is removed or replaced or when the list is freed. Every list made while the program runs is a heap list: list literals make a new list each time they run, because lists can change. The only static list is `es_no_list`, what a list variable holds before it's given a list (never read, never changed).

### Ownership

One rule keeps the generated code simple: **every text is owned by exactly one place, and whoever receives a text is responsible for it.**

| Situation | Rule |
|---|---|
| A text or list expression | Produces an *owned* value (a new reference). |
| Reading a text or list variable | Retains it (`es_retain(es_vt_name)`, `es_list_retain(es_vl1n_scores)`): the reader gets its own reference. |
| A text literal or constant | Immortal, so it's used directly, with no retain. |
| A runtime operation (`es_join`, `es_text_same`, `es_say`, `es_get_n`, `es_add_t`, `es_sort`, ...) | **Consumes** its text and list arguments: it releases them, or hands them back in its result. An item it gives back (`es_get_t`) is retained first. |
| Putting a value into a list (`a list of`, `add`, `insert`, `set item`) | The list takes the reference; a replaced or removed item is released. |
| Calling an EasyScript function | The function **owns** its text and list arguments and releases them when it ends. |
| `give back` | Returns an owned value. It's worked out first, then the function releases its text inputs and variables. |
| Storing into a variable (`let`, `set`, `ask`, `read file`) | `es_set(&var, text)` (or `es_list_set`) takes the new value and releases the old one *after* the new one is worked out, so `set t to t followed by "x"` is safe. Numbers and yes/no are plain assignments. |
| A block ends | The text and list variables first made in it are released (`es_drop`, `es_list_drop`). |
| `stop the loop`, `skip this one` | Release the text and list variables of every block they leave. |
| `give back`, or the end of a function | Release all of the function's text and list inputs and variables. |
| A function called as a sentence gives back text or a list | It's released straight away. |
| `for each N in L` | The loop holds the list it goes through in a hidden variable of the loop's scope (`es_list_snapshot` gives a copy only if something else holds the list), so leaving the loop in any way releases it. Its item variable holds a reference to the current item. |
| Slices (`items 2 to 4 of L`, `the first 3 items in L`, ...) | A new list (`es_list_pick`): text items are retained (text never changes), lists inside are copied, and the list it came from is released. |
| `every` and `only` | Each becomes a C function that takes the list (owned) and the variables it reads (borrowed: the caller holds them while it runs), and gives back a new list. |
| The end of the program | Releases every text and list variable of `main()`. `stop the program` in the main program jumps there (`goto es_end`). Inside a function, it releases that function's text and lists and exits. |

Because operations consume their arguments, nested expressions need no bookkeeping: in `"a" followed by x followed by "c"`, the inner join's result is consumed (and freed) by the outer join, and in `item 1 of (every s in scores plus 1)` the new list is consumed by `es_get_n`. Temporaries are never released by hand.

The variables of `main()` are C locals of `main()`, not globals. Their addresses never escape, so the C compiler can keep numbers in registers even though a release might call `free()`.

### What isn't cleaned up

- **Runtime errors** print their message and exit straight away, in the middle of a sentence. The operating system reclaims everything.
- **`stop the program` inside a function** can't reach the variables of `main()` or of the functions that called it, or values they were still working out. Again, the operating system reclaims them as the program ends.

### Cycles

A reference count can't free objects that refer to each other in a loop. With text and lists, that can't happen: a list holds one kind of item, and a list of some kind can never hold a list of its own kind (a list of numbers holds numbers, a list of lists of numbers holds lists of numbers), so no list can ever contain itself, directly or through other lists. The checker enforces it: `add grid to the end of grid` is a kind error. When records arrive in Phase 2, a record will be able to refer to itself, so EasyScript will need weak references or a cycle collector then (see the [roadmap](roadmap.md)).

### Checking it

- **`ES_DEBUG_MEMORY=1`**: every generated program counts its live heap objects. With this set, the program fails at the end (exit 70) if any are still alive: `Memory check: 1 value was still alive at the end of the program (39 bytes).` `make test` runs every program this way, and a control test in `tests/run.sh` checks that the check really catches a missing release.
- **`ES_MEMORY_LIMIT=N`**: fail (exit 70) as soon as live text and lists would take more than N bytes (it's checked before allocating, including when a list grows). `tests/memory/` runs loops of up to a million rounds under a 100,000-byte limit, and also checks that the program's peak memory stays under 16 MB where `/usr/bin/time` can measure it.
- **`ES_SANITIZE=1`**: the compiler builds programs with AddressSanitizer and UBSan, so a use after free or a double free stops with a report. `make test-debug` sets it for the whole suite.
- **`make test-debug`** also checks for leaks: LeakSanitizer on Linux, and on macOS `tests/leaks.sh` runs every test program (and the compiler) under the system `leaks` tool. Programs that stop with a runtime error are left out of the macOS sweep: they exit without releasing anything by design, so `leaks` only finds their variables if the C compiler happened to keep a pointer around.

### Adding a heap kind

Lists were added this way, so they're a worked example:

1. Add an `ES_OBJ_...` kind and a struct that starts with `EsObject header` (`ES_OBJ_LIST`, `EsList`).
2. Free it (and release anything it holds) in `es_free_object`. Keep that function out of line (its attributes keep GCC from inlining or specializing it): otherwise GCC sees `free()` reachable from releases of static immortal objects and warns.
3. Give it a type in the checker and a C type in codegen (`cg_ctype`), and make `type_counted` true for it, so retains, releases and drops are generated for it as for text (`cg_retain`, `cg_release`, `cg_set`, `cg_drop`, `cg_empty`).
4. Follow the ownership table: operations consume their arguments, and storing goes through a setter like `es_set` or `es_list_set`.
5. Make sure it can't form cycles, or plan for them (see above).
