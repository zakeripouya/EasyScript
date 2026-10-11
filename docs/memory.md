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

Only the latest text is kept; every earlier one is freed the moment `last` stops holding it. Lists, records and maps work the same way: a list, record or map is freed, with everything in it, when nothing (no variable, list, record or map) holds it any more. Numbers, yes and no, and `nothing` don't take extra memory at all, and text written in your program (like `"item "`) is part of the program itself.

Two things to know:

- **Text you keep growing really does grow.** Adding to the same text over and over (`set story to story followed by more`) keeps the whole text, as it should. A 50,000-character text takes about 50 KB. The same goes for a list you keep adding to: a million numbers take about 8 MB.
- **Memory goes back to the computer when the program ends,** including when it stops with an error or with `stop the program`.
- **Records in a circle are never freed while the program runs.** If two people are each other's friend, each one holds the other, so neither is ever "unused". For most programs this doesn't matter (the memory goes back when the program ends), but a loop that keeps making such circles keeps using more memory. Break the circle when you're done with it, by setting one of its fields to nothing (`set ann's friend to nothing`). With `ES_DEBUG_MEMORY=1` the program reports any circle left at the end.

## For contributors

### Reference counting

The compiler knows the kind of every value ([types](language-guide.md#9-types)), so generated code uses plain C types: a number is a `double`, yes/no is a `bool`, text is an `EsText *`, a list is an `EsList *`, a record is an `EsRecord *` and a map is an `EsMap *` (`runtime/es_memory.h`). Numbers and yes/no values own nothing, so **only text, lists, records and maps are reference counted**; no retain or release is ever generated for anything else.

Every heap object starts with the same small header, a kind and a reference count, so text, lists, and the records and closures planned for later share the same machinery (`es_object_release`, `es_free_object`, with typed wrappers `es_retain`/`es_release` for text and `es_list_retain`/`es_list_release` for lists):

```c
typedef struct { EsObjectKind kind; size_t refs; } EsObject;
typedef struct { EsObject header; size_t len; const char *chars; size_t count, cap; } EsText;
typedef struct EsList { EsObject header; EsItems items; size_t len, cap; union { double *n; ... } at; } EsList;
```

- **Heap text** (made while the program runs) starts with a count of 1. Its characters sit right after the object, in the same allocation, with room for `cap` bytes. `count` is how many characters (UTF-8 code points) it has, worked out the first time it's needed and kept (0 until then); text whose `count` equals its `len` is plain ASCII, so the text tools work on it byte by byte.
- **Growing in place.** `set T to T followed by X` becomes `es_append(&T, X)`: when the text's count is 1 (only this variable holds it), it's `realloc`ed to double its room when full and X is copied onto its end, so building text piece by piece takes time in proportion to its length (a million pieces: 130 s and 147 MB before, about 5 ms and 6 MB after). Shared text is copied as before, so every other holder still sees the old text. Codegen does this only when the parts added don't read T (`set t to t followed by t` is a plain join).
- **Static text** (literals in the program, text constants, and the runtime's own words like `yes` and `infinity`) is a static `EsText` whose count is `ES_IMMORTAL`. Retain and release skip it, so it's never counted or freed. Literals carry their character count, written by the compiler. A single ASCII character made while the program runs (`character 2 of word`, `the characters of word`) is one of 128 static immortal texts too (`es_ascii_text`), so taking ASCII text apart allocates nothing but the list.
- **A list** holds its items in an array that grows (doubling), allocated separately from the `EsList`. Lists whose size is known when they're made (written out, slices, copies, `every`) get exactly that size, and `only`'s result is shrunk to fit. A list nobody else holds (a value just worked out, with a count of 1) is reused instead of copied by slices and `a copy of`, and `reversed` and loops already did the same. Its items are all one kind (`EsItems`: numbers, yes/no, text, or lists); text and lists in it are references the list owns, released when the item is removed or replaced or when the list is freed. Every list made while the program runs is a heap list: list literals make a new list each time they run, because lists can change. The only static list is `es_no_list`, what a list variable holds before it's given a list (never read, never changed).

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
| `L followed by M` (two lists) | A new list of exactly the right size (`es_list_join`); if nothing else holds L, L grows into it. Text items are retained, lists inside are copied, and M is released. |
| Slices (`items 2 to 4 of L`, `the first 3 items in L`, ...) | A new list (`es_list_pick`): text items are retained (text never changes), lists inside are copied, and the list it came from is released. |
| A function's frame (for runtime errors' call chain) | Retains its text inputs when the function starts and releases them when it ends, so an error can show the values the function was given. |
| `every` and `only` | Each becomes a C function that takes the list (owned) and the variables it reads (borrowed: the caller holds them while it runs), and gives back a new list. |
| The end of the program | Releases every text and list variable of `main()`. `stop the program` in the main program jumps there (`goto es_end`). Inside a function, it releases that function's text and lists and exits. |

Because operations consume their arguments, nested expressions need no bookkeeping: in `"a" followed by x followed by "c"`, the inner join's result is consumed (and freed) by the outer join, and in `item 1 of (every s in scores plus 1)` the new list is consumed by `es_get_n`. Temporaries are never released by hand.

The variables of `main()` are C locals of `main()`, not globals. Their addresses never escape, so the C compiler can keep numbers in registers even though a release might call `free()`.

### What isn't cleaned up

- **Runtime errors** print their message and exit straight away, in the middle of a sentence. The operating system reclaims everything.
- **`stop the program` inside a function** can't reach the variables of `main()` or of the functions that called it, or values they were still working out. Again, the operating system reclaims them as the program ends.

### Records

A record (`EsRecord`, `runtime/es_record.h`) is a header (kind, count, its `EsRecordType`, and two links) followed by its fields, laid out by a C struct codegen writes per kind (`es_R0`). The `EsRecordType` lists each field's name, kind (`n b t l r`) and offset, so the runtime can release, copy and print any record without knowing its struct. A field that can be nothing holds `NULL`; every record retain and release is NULL-safe. Records in lists are counted items (`ES_ITEMS_RECORD`) and are shared, never copied, by list copies and slices.

**Freeing doesn't recurse.** Releasing the first node of a million-node chain frees a million records. `es_free_object` frees text at once, but a list or record whose count reaches zero while another is being freed is put on `es_dying`, a growing array, and freed in a loop, so the C stack never grows with the chain (`tests/run/records_long_chain.es`).

### Maps

A map (`EsMap`, `runtime/es_map.h`) holds its entries in an array in the order they were added (`EsEntry`: the key's hash, the key and the value, as an untagged `EsValue`; the map knows its key kind, `ES_ITEMS_TEXT` or `ES_ITEMS_NUMBER`, and its value kind), plus an index of `uint32_t` places once it has more than 8 entries. Both grow by doubling and are counted against `ES_MEMORY_LIMIT` before they grow. A removed entry stays in place with hash 0 until the entries are packed (when they're full and a quarter or more are removed, or the index is rebuilt). Text keys and counted values are references the map owns, released when the entry is removed or replaced or the map is freed; setting an existing key releases the new key and keeps the old one. Maps follow the same ownership rules as lists: map expressions are owned, runtime functions consume the map and the key (`es_map_get_n`, `es_map_put_t`, `es_map_has`, `es_map_remove`), `es_map_at` (for `add 1 to the value for K in M`) only looks at the map, and `for each K and V in M` holds `es_map_snapshot(M)` in a hidden variable. `a copy of M` copies the lists and maps in it (`es_copy_inner`), like a list's copy; text and records are shared. A fixed map (`keep`) is made when `main()` starts and marked immortal (`es_map_fix`), which takes it out of the live counts; the only static map is `es_no_map`.

### Cycles

A reference count can't free objects that refer to each other in a loop. Text, lists and maps can't: a list or map of some kind can never hold a list or map of its own kind, so none can contain itself (the checker enforces it). **Records can:** two people who are each other's friend, or a node whose `next` leads back to it. Such a circle is never freed while the program runs.

For now, EasyScript finds them instead of freeing them. With `ES_DEBUG_MEMORY=1`, every record is linked into a list of live records (`es_memory.records`, using `EsRecord.prev` and `next`). If anything is alive at the end, `es_report_cycles` takes each live record's count, subtracts the references from other live records (directly, or through lists and maps only they hold), and treats records with references left over as held from outside. Everything those lead to is alive for a reason; the rest only keep each other alive. It prints up to three of them and how to break the circle, and exits 70:

```
Memory check: 4 records were still alive at the end of the program, because they point to each other in a circle:
    a person with name "Ann" and friend (a person with ...)
    a person with name "Bo" and friend (a person with ...)
    a person with name "Cy" and friend (a person with ...)
    ...and 1 more
Records in a circle are never freed, because each one keeps the next alive. Before the program ends, set a field in the circle to nothing, or take a record out of the list or map that holds it.
```

**Plan:** weak references come later: a field written as weak (the wording isn't decided) wouldn't count, so a back-link (a child's parent, a doubly linked list's `previous`) wouldn't keep anything alive. A cycle collector (finding circles while the program runs, as the check above does at the end) is the alternative if weak references turn out too hard to explain. See the [roadmap](roadmap.md).

### Checking it

The first two are read only by programs built with the runtime's test hooks: `ES_HOOKS=1 ES_DEBUG_MEMORY=1 easyscript run program.es`. Programs made with `easyscript build` never read them (see [SECURITY.md](../SECURITY.md)).

- **`ES_DEBUG_MEMORY=1`**: every generated program counts its live heap objects. With this set, the program fails at the end (exit 70) if any are still alive: `Memory check: 1 value was still alive at the end of the program (39 bytes).` `make test` runs every program this way, and a control test in `tests/run.sh` checks that the check really catches a missing release.
- **`ES_MEMORY_LIMIT=N`**: fail (exit 70) as soon as live text, lists, records and maps would take more than N bytes (it's checked before allocating, including when a list or map grows). `tests/memory/` runs loops of up to a million rounds under a 100,000-byte limit, and also checks that the program's peak memory stays under 16 MB where `/usr/bin/time` can measure it.
- **`ES_SANITIZE=1`**: the compiler builds programs with AddressSanitizer and UBSan, so a use after free or a double free stops with a report. `make test-debug` sets it for the whole suite.
- **`make test-debug`** also checks for leaks: LeakSanitizer on Linux, and on macOS `tests/leaks.sh` runs every test program (and the compiler) under the system `leaks` tool. Programs that stop with a runtime error are left out of the macOS sweep: they exit without releasing anything by design, so `leaks` only finds their variables if the C compiler happened to keep a pointer around.

### Adding a heap kind

Lists were added this way, so they're a worked example:

1. Add an `ES_OBJ_...` kind and a struct that starts with `EsObject header` (`ES_OBJ_LIST`, `EsList`).
2. Free it (and release anything it holds) in `es_free_object`. Keep that function out of line (its attributes keep GCC from inlining or specializing it): otherwise GCC sees `free()` reachable from releases of static immortal objects and warns.
3. Give it a type in the checker and a C type in codegen (`cg_ctype`), and make `type_counted` true for it, so retains, releases and drops are generated for it as for text (`cg_retain`, `cg_release`, `cg_set`, `cg_drop`, `cg_empty`).
4. Follow the ownership table: operations consume their arguments, and storing goes through a setter like `es_set` or `es_list_set`.
5. Make sure it can't form cycles, or plan for them (see above).

Records followed these steps too, plus `es_dying` for freeing without recursion.
