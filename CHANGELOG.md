# Changelog

All notable changes to EasyScript are recorded here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow [Semantic Versioning](https://semver.org/).

## [Unreleased]

### Breaking

- **`wait`, `shuffle`, `save` and `put` start sentences** (the standard library), so they can no longer be the one-word name of a function. `tests/ast/err_func_header.es` renamed its `to wait for:` to `to pause for:`. `pi`, `today`, `tomorrow` and `yesterday` are still names a program can make; `rename`, `copy`, `move`, `delete`, `make` and `clear` only start library sentences with `the file`, `the folder` or `the screen` after them.

- **`apply` starts sentences** (`apply double to every number in scores.`), so it can no longer be the one-word name of a function (`to apply with f:`). A name of several words starting with it still works. `lambda.es` renamed its `apply` to `evaluate application`.

- **`the position of X in T` (and `in L`) is `a number or nothing`,** and gives **nothing** instead of 0 when X isn't there, following the design rule that "not found" results use `or nothing`. Using it as a number now needs a check first: `if at is not nothing:`, `if at is nothing, ...`, or `check which kind at is:`. Programs that compared it with 0 (`if the position of "@" in address is 0`) write `is nothing` instead. The exception for it is gone from the design principles.

### Changed

- **`easyscript run FILE WORDS...` gives the words after the file to the program** (`the words given to the program`), so `easyscript run a.es b.es` no longer says "Give one .es file at a time." (`emit`, `build`, `tokens` and `ast` still do). Generated programs' `main` takes `argc`/`argv` and returns the program's code.

### Added

- **System tools** (Phase 2, step 22). See [stdlib.md](docs/stdlib.md#system-tools) and the new [chapter 19](docs/language-guide.md#19-programs-and-the-system).
  - **What the program is given:** `the words given to the program`, `the number of words given to the program`, `word N given to the program` (with "There's no word 2: the program was given 1 word."), `the text sent to the program` and `the lines sent to the program` (piped input, read once, after whatever `ask` has read).
  - **Settings:** `the setting called X`, text or nothing.
  - **Running other programs:** `run P with A, B and C.` (output shown; the program stops if it doesn't work) and `... and call the result r` (or `and call it r`), a `program result` with `r's output`, `r's error output` and `r's code`, and the tests `r worked` / `r didn't work`. The words after `with` are text, numbers or lists of text, passed as a list of words with `fork`/`execvp`, never through a shell; a missing program is "I couldn't find a program called "gti".". `program result` is a kind of record the language adds to programs that use it.
  - **Finishing:** `complain X` writes to the error output; `stop the program, saying it failed` (code 1) and `stop the program with code N` (0 to 255).
  - `run` and `complain` only start these sentences where the program hasn't defined a function with that name.
- **The keyboard** (step 22; [chapter 20](docs/language-guide.md#20-the-keyboard)): `wait for a key and call it K` and `the key being pressed` (text or nothing), with plain names (`"up"`, `"enter"`, `"escape"`, `"space"`, `"page up"`, ...) and the typed character otherwise. On a terminal the first key read turns on key-at-a-time mode (no echo, no Enter) until the program ends, `ask`s or runs another program; the terminal is put back at exit, after runtime errors and on Ctrl+C (and SIGTERM, SIGHUP, SIGQUIT). Keys can come from a file or a pipe, which is how they're tested. macOS and Linux; Windows waits for the Windows port.
- **Example:** `examples/programs/snake/`: Snake in the terminal (`play.es`), with its rules in `board.es` tested by the scripted `main.es`; the test suite only compiles `play.es` (`examples/snake-play-compiles`).
- **Tests:** `tests/run/system_*` and `keys_scripted`, 12 `err_sys_*`/`err_keys_*` runtime errors, `errors/system_errors` and `system_own_program_result`, `ast/system` and `err_system`; a run test's `NAME.args` holds the words given to it; CLI checks for words, exit codes and `complain`; `keys/terminal-restored` (`tests/tools/keys_terminal.py`) runs programs on a pseudo-terminal and checks that keys arrive one at a time and the terminal is put back after a normal end, an error and Ctrl+C.
- Inside the compiler: `src/front/parse_system.c`, `check_system.c`, `src/back/codegen_system.c`, and two runtime parts, `runtime/es_keys.h` (15) and `es_system.h` (16); `Records.program_result`, `STMT_STOP`'s `stop_code`.

- **Dates in lists, maps and records:** `a list of dates`, `a map from text to dates`, fields `which is a date`, and dates in variants (`at with moment (a date)`). They're compared (`is`, `contains`, `the position of`, list and record equality), sorted earliest first (`sort`, `sorted`, `sorted by`, `the biggest of` / `the smallest of`), copied, joined with `joined with`, and printed in parentheses inside lists, maps and records: `(Sunday, October 11, 2026) and (Monday, October 12, 2026)`. The error "Dates can't be kept in a list yet." and its design exception are gone. A date can't be a map's key, and "a date or nothing" is a new error ("A date can't be "or nothing" yet."). In the runtime, lists and maps of dates are `ES_ITEMS_DATE` (kept like numbers) and date fields are `'d'`; they hold nothing counted, so freeing and the circle check skip them. Example `examples/programs/orders.es` (totals per month, this month's orders); tests `run/library_dates_kept`, `memory/dates_loop` and `memory/err_date_record_cycle`.
- **Calling a function value that gives back a date** (`call f`, `f applied to every ...`) no longer says the function "has to give back a value".

- **The standard library** (Phase 2, step 21), with no `use` needed. See [stdlib.md](docs/stdlib.md) and the new [chapter](docs/language-guide.md#18-the-standard-library).
  - **Math:** `the square root of X`, `X to the power of Y`, `the absolute value of X`, `X rounded` (half away from zero), `rounded up`, `rounded down`, `rounded to N decimal places`, `the bigger of X and Y`, `the smaller of X and Y`, `pi` (also in constants), with errors for square roots of negatives and impossible powers.
  - **Random numbers:** `a random number from A to B`, `a random decimal between A and B`, `pick one of L` (or `a random item of L`), `L shuffled` and `shuffle L`, `roll a die`, and `use the same random numbers every time.`; xoshiro256** seeded from the clock.
  - **Dates:** a new kind, `a date`: `the time now`, `today`, `tomorrow`, `yesterday`, `N days ago`, `N days from now`, `the year of D` / `D's year` (and month, day, weekday, hour, minute), `D is before E`, `D is after E`, `D is in this month` (last/next month, this/last/next year), `the number of days between D and E`, printed like "Sunday, October 11, 2026". `wait N seconds` (milliseconds, minutes).
  - **Files and folders:** `file F exists` (planned since Phase 1) and `folder F exists`, `put X in file F`, `write X in file F`, `add X to the end of file F`, `the lines of file F`, `make a new file called F`, `make a folder called F`, `rename`/`copy`/`move the file F ...`, `delete the file F`, `delete the folder D [and everything in it]`, `the files in D`, `the folders in D`, `the size of file F`, `the last change of file F`. Nothing is replaced, a folder with things in it needs `and everything in it`, and paths like `"/"` are refused.
  - **CSV:** `read csv file F and call it rows` (a list of maps from column name to text, with full quoting, `\r\n` and blank lines) and `save L as csv file F`, with errors that name the file's line.
  - **The screen:** `clear the screen.` (only when the output is a terminal).
  - **Tests and benchmarks:** under `ES_HOOKS`, `ES_NOW` fixes the clock (and `wait` moves it on instead of sleeping) and `ES_RANDOM_SEED` fixes the random numbers; `tests/run.sh` sets both. A `sales` benchmark reads a csv file of 100,000 rows (0.04 s against Python's `csv` module at 0.09 s, in more memory: every row is kept); `benchmarks/run.sh` runs a benchmark's `setup.sh` first, in `build/bench`.
  - Examples `dice.es`, `countdown.es`, `tidy_folder.es` and `sales_report.es`; `multi_file/` uses `rounded to 1 decimal place` and `pi`.
  - Inside the compiler: `EXPR_LIBRARY` (`LibraryOp`) and `STMT_LIBRARY` (`LibraryAction`), `TYPE_DATE`; new files `src/front/parse_library.c`, `parse_library_stmt.c`, `check_library.c`, `src/back/codegen_library.c`, `src/common/library.c`, and four runtime parts, `runtime/es_math.h`, `es_time.h`, `es_files.h` and `es_csv.h`.
- **Programs in several files** (Phase 2, step 21). See the new [chapter](docs/language-guide.md#17-programs-in-several-files).
  - `use helpers.` (or `use the helpers file.`) reads `helpers.es` from the same folder; a used file can only define things (functions, methods, constants, kinds, interfaces) and can use other files. Each file is read and checked once; files using each other in a circle is an error that shows the circle.
  - A file sees its own definitions first, then those of the files it uses. A name two used files define is an error only where it's used, naming both files and suggesting `helpers's double`, which works anytime. `privately to ...` and `privately keep ...` keep a definition to its file.
  - Errors name the file and its line (`Line 4 of helpers.es: ...`), at compile time and in runtime errors and call chains.
  - Tests in `tests/modules/NAME/` folders (`main.es` plus the files it uses, with `main.out`/`main.err`); examples can be folders too: `examples/programs/multi_file/`.
  - Inside the compiler: `src/modules.c` finds and joins the files before lexing (`lex_files` restarts line numbers per file; `Token.file`, `SourcePos.file`, `diag_set_files`); `STMT_USE`, `Expr.module`, `private` on functions and constants; `src/front/parse_module.c` and `src/front/check_module.c`; the checker renames other files' functions and constants so the C names stay apart. The runtime's sites carry a file (`EsSite.file`, `es_source_files`).

- **Interfaces** (Phase 2, step 20). See the new [chapter](docs/language-guide.md#16-interfaces).
  - **Defining one:** `something that can speak is anything with:` (or `a speaker is anything with:`) with ways indented below: `a way to speak.`, `a way to greet with name (text), giving back text.`
  - **Fitting:** any kind of record or kind with variants with a method of the same name, inputs and result for every way fits, without saying so; a way giving back an interface accepts a method giving back any kind that fits it. Giving one that doesn't is an error naming the missing way ("A fish can't be used as something that can speak: it has no way to speak.").
  - **Using one:** as an input's kind, `an empty list of things that can speak`, in maps and fields, `or nothing`; `speak animal` picks the method by the value's kind while the program runs (a C function per way that's called, testing the value's type). Values of an interface have no fields; a list written out with two kinds suggests the interface they share.
  - Example `zoo.es` (dogs, cats and birds in one list).
  - Inside the compiler: `TYPE_IFACE + n` (`Records.interfaces`), `STMT_INTERFACE` with `Way`s, `EXPR_CALL.dispatch`; record kinds now use 16 to 1023, kinds with variants start at 2048; new files `src/front/parse_iface.c`, `src/front/check_iface.c`, `src/back/codegen_iface.c`.
- **Functions as values** (Phase 2, step 20). See the new [chapter](docs/language-guide.md#15-functions-as-values).
  - **Making them:** `the function double` (a `to` function, not called); `a function taking x that gives back x times 2` on one line, or `a function taking word (text):` with its sentences indented below (the colon ends the sentence); `a function that gives back 42` and `a function:` with no inputs. Inputs and results are inferred like a `to` function's, or written down (`a function taking x (a number), giving back text:`).
  - **Calling them:** `call f with 3` (a sentence or a value; `of` and `using` too; any single value can be called: `call item 1 of steps with 3`, `call the value for "click" in handlers with "OK"`), `f with 3` in a value, and `call next` for one with no inputs.
  - **Kinds:** `a function taking a number and giving back text`, `a function taking text`, `a function giving back a number`, `a function`; `functions taking text` in list and map kinds. They work as inputs, results, list items, map values, record fields and variant fields.
  - **Closures:** a function made inside a value uses (and changes) the names made before it, shared rather than copied, so `to make counter` can give back a function that keeps counting. Each `let` (and each round's loop number or item) is a new name, so functions made in a loop keep their own round's.
  - **Errors:** calling something that isn't a function, the wrong number or kinds of inputs (with the function's kind), using a function that gives back nothing as a value, a check undone by a function that can set the name ("... can set "maybe" whenever it's called, so the check doesn't last."), `it` in a function made in a loop, functions made inside `every`/`only`, inputs nothing decides. A `to` inside a function suggests `let NAME be a function taking ...:`. A broken sentence ending in a colon now skips its block, so its lines don't cause more errors.
  - **Memory:** a closure holds the cells of the names it shares; a function kept in a name it uses keeps itself alive, and the memory check explains that ("... because a function keeps itself alive through a name it uses").
  - **Built-ins that take a function,** each a value and (where it changes a list) a sentence: `double applied to every number in scores` / `apply double to every number in scores.`; `the numbers in scores where it is above 50` and `the numbers in scores that pass even` / `keep only the numbers in scores where ...` (or `that pass F`); `scores combined using sum, starting from 0` (or from the first item; an empty list is then a runtime error); `names sorted by length [biggest first]` / `sort names by length, biggest first.` (stable, by numbers or text). A function's name given to one means the function itself. Each use becomes a C function that calls a `to` function directly. A new `pipeline` benchmark (with `pipeline_loop`, the same as loops) puts them at 1.4 times a hand-written loop and 9 times faster than Python ([performance](docs/performance.md)).
  - Examples: `counter.es`, `event_handlers.es` and `pipeline.es` (a sales report).
  - Inside the compiler (built-ins): `EXPR_APPLY` (`ApplyKind`), `STMT_LIST` with `LIST_APPLY`/`LIST_KEEP`/`LIST_SORT_BY` whose new items are worked out from the list through a hidden name and then replace its items (`es_list_replace`), `es_sort_by_keys`; new files `src/front/parse_apply.c`, `src/front/check_apply.c`, `src/back/codegen_apply.c`, and `src/back/codegen_names.c` (C names, split from `codegen_expr.c`).
  - Inside the compiler: `Type` gained kinds of function (`TYPE_FUNC + n`, each kept once in `Records.funcs`) and the inference engine function variables; new files `src/front/parse_func_value.c`, `src/front/check_func_value.c`, `src/back/codegen_closure.c` and the runtime's tenth part, `runtime/es_closure.h` (closures and cells, both records). A function made inside a value is a `STMT_FUNCTION` with `lambda` set; the checker records its captures and the names that need cells.
- **`or nothing` for numbers, text and yes/no:** `a number or nothing`, `text or nothing`, `yes or no or nothing`, for fields, inputs and results, like `a dog or nothing`. Such a value has to be checked before it's used as its kind (the same checks as for records, and `check which kind X is:` with `when it's nothing:` and `otherwise:`); kind mistakes with one say so ("It could be nothing. Check it first, like "if at is not nothing:"."). A number, text or yes/no goes wherever one that can be nothing is kept; printing shows the value or `nothing`. Lists and maps can't be nothing ("A list can't be "or nothing"."): an empty one says there's nothing. While the program runs, such a value is kept in a small box (like a record; nothing is no box at all).
- **Kinds with variants** (Phase 2, step 19). See the new [chapter](docs/language-guide.md#14-kinds-with-variants).
  - **Defining one:** `a shape is one of:` with its variants indented below, each with its fields and their kinds in parentheses: `a circle with a radius (a number).`, `a rectangle with a width (a number) and a height (a number).`, or none: `a point.` Kinds can be recursive (`a sum with a left (an expression) and a right (an expression).`); one that could never be made is an error.
  - **Making values:** `a new circle with radius 3`, like records. A value never changes after it's made, and two are equal (`is`, `contains`, map values) when they're the same variant with equal fields.
  - **Telling them apart:** `check which kind s is:` with `when it's a circle:` (or `when it is ...`, several variants joined with `or`, and `when it's nothing:`) and an optional `otherwise:`. A variant without a `when` and no `otherwise` is a compile error naming it; an `otherwise` that can never run is a **warning** (the first one: warnings are printed before the program runs, marked "(a warning)", and the shell shows each once). Inside a `when`, `it` is the value and `its radius` its field.
  - **Checked fields:** `s's radius` needs s checked to be a circle: by `if s is a circle:` (also `is not`, `isn't`, with `and`, `otherwise`, `while`, and an early `give back`), a `when`, or `let s be a new circle ...`. Otherwise it's a compile error that suggests both checks. While the program runs it's checked again, for a value reached through a record's field.
  - **Everywhere values go:** lists, maps, record fields, inputs and results (`(a shape)`, `giving back an expression`, inferred like other kinds), `a shape or nothing`, methods (`to describe, for a shape:`, sharing names with record methods), `a copy of` (its own copies of the lists and maps in it), and printing (`a circle with radius 3`).
  - **Memory:** values are reference counted like records; one can only hold itself through a list or map, and the memory check reports that circle.
  - Examples: `shapes.es` (an area calculator), `calculator.es` (reads "2 plus 3 times 4" into a recursive kind with the right precedence and works it out) and `lambda.es` (a tiny lambda calculus interpreter with Church numerals). Matching costs about the same as reading a record's field (see [performance](docs/performance.md)), so there's no new benchmark.
  - Inside the compiler: `Type` gained `TYPE_SUM + n` (kinds in `Records.sums`, variants in `Records.variants`), `type_is_object` for everything kept as an `EsRecord *`, `Diag` gained warnings (`diag_warning`); new files `src/common/type.{h,c}` (split from `ast`), `src/front/parse_kind.c` (kinds of values, moved from `parse_list.c`, and kind definitions), `src/front/parse_match.c`, `src/front/check_names.c` (split from `check.c`), `src/front/check_narrow.c` (narrowing, split from `check_record.c`), `src/front/check_kind.c`, `src/back/codegen_kind.c`, and the runtime's ninth part, `runtime/es_kind.h`. New test folder `tests/warnings/`.
- **`L without item N`** and **`T without character N`**, the value forms of `remove item N from L`, and **`M without K`**, a new map without one key (the key has to be there: "I can't leave out "Bob": the map has no value for it.").
- **`ask ... and call it NAME`** and **`read file ... and call the answer NAME`**: both sentences take both phrasings.
- **Methods with the same name for different kinds of record:** `to speak, for a dog:` and `to speak, for a cat:` can both exist; `speak rex` and `speak tom` each call the one for their record's kind, worked out like any other kind (so a function that calls `speak pet` works once it's given a dog). Two methods with the same name for one kind, or a plain function sharing a method's name, are errors; calling it on another kind lists the kinds that have it; and when nothing decides the kind, the error asks for it (`(a dog)`).
- **More ways to introduce a function's inputs:** `for`, `from` and `in` work like `to`: `to pause for seconds:` is called `pause for 3`, `to busiest artist from names:` is called `busiest artist from names`, and `to count vowels in word:` is called `count vowels in "banana"` (or with `with`, `of` or `using`, as always). Methods take them after the record too (`fetch rex from "the park"`), and so does `to` (`rename rex to "Max"`, which didn't work before). Calling a function with a word it doesn't take is an error that names the ones it does.
- **`L sorted`:** a sorted copy of a list, leaving it as it was, the value form of `sort L`: `scores sorted`, `scores sorted biggest first`, `names sorted by their length`, `names sorted by their length longest first`.
- **Design principles** ([docs/design-principles.md](docs/design-principles.md)): the rules every phrasing follows (statements and values, how inputs are introduced, `of` and `in`, optional and "not found" results, errors, equality, copying), the known exceptions and why each exists, and open questions. Future ideas, starting with a deep copy, are in [docs/ideas.md](docs/ideas.md).
- **Maps** (Phase 2, step 18). See the new [Maps chapter](docs/language-guide.md#13-maps).
  - **Making them:** `a map of "coffee" to 3, "tea" to 2` and `an empty map from text to numbers`. Keys are text or numbers; values any kind (lists, records and other maps too), one kind each per map. A key written out twice is a compile error.
  - **Looking up and setting:** `the value for "Paris" in ages`; a missing key stops the program with an error naming it ("There's no value for "Bob" in the map."). `the value for "Bob" in ages, or 0` gives another value instead, worked out only when it's needed (arithmetic straight after it is an ambiguity error). `set the value for "Paris" in ages to 19` adds or changes a key.
  - **Asking:** `ages has "Paris"` (and `does not have` / `doesn't have`), `the number of entries in ages`, `ages is empty`, `the keys of ages`, `the values of ages`; two maps are equal (`is`) with the same keys and equal values, in any order.
  - **Changing:** `remove "Paris" from ages`, `add 1 to the value for "Paris" in ages` and the other change sentences, and `add X to the end of the value for K in M` for lists in a map. Changing or removing a missing key is a runtime error.
  - **Going through:** `for each name and age in ages:`, always in the order keys were first added, over the entries the map had when the loop started; `it` is the key.
  - **Fixed maps:** `keep prices as a map of "coffee" to 3, "tea" to 2.` Changing one is a compile error that suggests `let`; `the number of entries in` one is worked out by the compiler. Fields can start at a fixed or empty map.
  - **Kinds:** `(a map from text to numbers)`, `giving back a map from numbers to lists of text`, and `maps from text to numbers` inside list and map kinds, or worked out from use.
  - **Sharing:** maps are shared by reference; `a copy of M` copies the lists and maps inside it too (records and text are shared).
  - **Printing:** `say ages` shows `"Paris" is 19 and "Junie" is 24` (text in quotes; lists, maps and records in parentheses).
  - **The hash table** (`runtime/es_map.h`, the runtime's seventh part): entries in the order they were added plus an index of 32-bit places, a power of two kept at most two thirds full (linear probing); maps of up to 8 entries look through their entries instead. Number keys are the same key when they print the same (`0.1 plus 0.2` finds `0.3`). Keys and values are reference counted like list items, and the memory check follows maps to find records in a circle.
  - Examples: `word_counter.es` now counts with a map, and the new `graph.es` finds shortest paths with breadth-first search. A new `words` benchmark counts two million words against Python dicts and Go maps. Tests for every form, every compile and runtime error, ordering, number keys, fixed maps, maps of lists and records, 20,000 entries, and maps in a 200,000-round loop under the memory limit.
  - Inside the compiler: `Type` gained kinds of map (`TYPE_MAP + n`, each kind of map kept once in `Records.maps`), the type inference engine got map variables (a key and a value), `const_eval` takes the program's kinds; new files `src/front/parse_map.c`, `src/front/check_map.c` and `src/back/codegen_map.c`.

- **Records** (Phase 2, step 17). See the new [Records chapter](docs/language-guide.md#12-records).
  - **Kinds of record:** `a dog is a kind of thing with:` and indented fields, `a name, which is text.`, of any kind (lists and other records too), with names of several words (`a start time, which is a number.`) and optional starting values (`an age, which is a number, starting at 0.`; fixed values, or an empty list, copied for each record).
  - **Making them:** `a new dog with name "Rex" and age 3`. A missing field is a compile error unless it has a starting value or can be nothing.
  - **Fields:** `rex's name`, chains (`the order's customer's name`), `it's age`, `(item 1 of dogs)'s name`; `set rex's age to 4`, `add 1 to rex's age` and the other change sentences. The lexer now makes a word ending in `'s` a name and a possessive (`POSSESSIVE` token), so a name can no longer end in `'s`.
  - **Fields that can be nothing:** `a next, which is a node or nothing.` Using one as a record before checking it (`if the node's next is not nothing:`, `while`, `until`, `and`/`or`, or after an `if ... is nothing` that leaves) is a compile error; inside the check it's the record. Setting it ends the check. `nothing` can be stored, given and given back where a record or nothing goes. The program checks again while it runs, so a record set to nothing through another name is a runtime error, not a crash.
  - **Sharing:** records are shared by reference like lists; `a copy of rex` is a shallow copy; `is` means the same record.
  - **Methods:** `to bark, for a dog:` (the record is `the dog` inside), `to have a birthday for a dog:`, `to rename, for a dog, with new name:`; called as `bark rex.`, `have a birthday for rex.`, `rename rex with "Max".` or in values. A function's name ends before `for` only when a kind of record follows.
  - **Printing:** `say rex` shows `a dog with name "Rex" and age 3` (records and lists inside in parentheses, three levels deep); stack traces show record inputs the same way, cut short.
  - **Memory:** records are reference counted with their fields. Freeing works through a list instead of recursion, so a million-node chain frees without running out of C stack. With `ES_DEBUG_MEMORY=1`, records left alive in a circle at the end are found and explained (exit 70); weak references come later ([memory.md](docs/memory.md#cycles)).
  - Records in lists (`a list of dogs`; `contains` and `the position of` find the same record) and lists in records.
  - Examples: `linked_list.es` and `binary_tree.es`. Tests for every form, every error, sharing and copying, and records in a 200,000-round loop under the memory limit.
  - Inside the compiler: `Type` gained record kinds (`TYPE_RECORD + n`) and "or nothing" (`TYPE_MAYBE`), and `TYPE_LIST` is now 32768; `parse_program` gives back a `Program` (statements and record kinds); new files `src/front/parse_record.c`, `src/front/check_record.c`, `src/back/codegen_record.c` and `runtime/es_record.h` (the runtime's sixth part).

### Changed

- **Docs: plans for what comes next.** Rules for new phrases in the [design principles](docs/design-principles.md#rules-for-new-phrases) (one meaning per word, everyday English, beginner messages without programmer words, exact meanings in core terms, high-level operations kept as themselves, distinct trigger words, and the success test), the [English pass](docs/english-pass.md) plan, and a [roadmap](docs/roadmap.md) with steps 21 and 22, the English pass and v0.2.0, a browser playground, going public, and the later phases' planned items.
- **The build works on Linux again:** `src/modules.c` used `SIZE_MAX` without `<stdint.h>`, which macOS's headers include indirectly but glibc's don't. A new audit (`audit/includes`, `tests/tools/includes.py`) checks every C file for standard names used without their header, and `make test-gcc` builds the compiler with GCC and `-Werror` (Homebrew's `gcc-N` on macOS) and runs the whole suite against it, with the strict-C check also using GCC. A string split across two lines in `parse_library_stmt.c` no longer looks like a missing comma to Clang (`-Wstring-concatenation`).
- **No more anonymous struct in a parameter list:** `check_func_value.c` took a `Vec(const char *) *`, which GCC warns about ("anonymous struct declared inside parameter list"); it now takes a `NameList` (a new typedef in `ast.h`, also used for the names kept in cells). No change in behavior.
- **Whole numbers print exactly up to 2^53** (9007199254740992), without an exponent: `1000000 times 1000000 times 1000` prints `1000000000000000` (it printed `1e+15`). Map keys keep whole numbers up to 2^53 exactly too.
- **Generated temporaries are always initialized,** so GCC 13 at `-O2` no longer warns "may be used uninitialized" for map lookups' temporaries (the Linux CI failure), and `consteval.c` initializes its comparison result.
- **Numbers print with at most 10 significant digits** (it was 15), so rounding errors never show: `1 divided by 3` prints `0.3333333333`, and a number that rounds to a whole one prints as one (`0.99999999999` prints `1`). Whole numbers still print without decimals. Only printing rounds; numbers keep their full precision. **Map number keys** are now the same key when they're the same to 12 significant digits (it was 15), closer to `is`, which treats numbers within a relative 1e-12 as equal: `1` and `1.0000000000001` are one key.
- **Programs made with `easyscript build` no longer read the runtime's test variables** (`ES_SKIP_OUTPUT`, `ES_ANSWERS`, `ES_DEBUG_MEMORY`, `ES_MEMORY_LIMIT`). They're compiled in only for `easyscript run` with `ES_HOOKS=1`, which the test suite and the interactive shell set. New [SECURITY.md](SECURITY.md): how to report a problem and what the compiler guarantees (no shell, escaped output, private temporary directories).
- **Error messages never suggest underscore names.** A word that can't be a name suggests a name that says what it holds (`"total"`), a word inside a name suggests the other words (`"price tax"`, `"pay rate"` for `rate of pay`), a function named like a sentence word suggests adding a word (`"say something"`), and a taken item name suggests `"current score"`. A new audit in `make test` checks that suggestions only show underscores the program itself wrote.
- **`for` always ends a function's name** in its definition, because it now introduces inputs: `to print receipt for customer:` is `print receipt` with the input `customer` (it was a function called `print receipt for customer` with no inputs).
- **`L reversed` copies the lists inside it,** like slices and `a copy of`, so changing the reversed list never changes the original (they were shared).
- `a copy of` works on records and maps too, so its message is now "I can only copy a list, a map or a record, but this is text."; `is empty` works on maps, so its message is now "I can only check whether text, a list or a map is empty, but this is a number."
- `remove 95 from scores` is now read as removing a key from a map (a compile error for a list, which suggests `remove item 2 from scores`) instead of the parse error "I expected "item" here".
- The notes for unknown kinds mention maps ("... lists like "a list of numbers", and maps like "a map from text to numbers"."; "Lists and maps hold ...").
- The memory check's advice for records in a circle now also says "or take a record out of the list or map that holds it".
- A fixed list can't hold maps either: "A fixed list can't hold lists or maps, because those could change."

- **Text tools** (Phase 2, step 16). See the new [Text chapter](docs/language-guide.md#11-text).
  - **A character is a Unicode character** (a code point, stored as UTF-8), so `"café"` has 4 characters. Text keeps its character count once it's been worked out (literals carry it from the compiler); text whose count equals its length is plain ASCII and is worked on byte by byte, and a single ASCII character is a static text, so taking ASCII text apart allocates nothing but the list.
  - **Characters and pieces** with exactly the list forms and rules: `character 3 of word`, `the first character of word`, `the last character in word`, `characters 2 to 5 of word`, `characters 3 to the end of word`, `the first 3 characters of word`, `the last 2 characters of word`, `word without the first character` / `without the last character`, `every other character of word`, `every second character of word` (any ordinal), `the characters of word` (a list of text), and `the number of characters in word`. Positions that aren't in the text stop the program like list positions do ("There's no character 5 in the text: it has 4 characters."). Using a list form on text (or the other way round) is a compile error that shows the right form.
  - **Searching:** `word contains "ab"` (`contains` now works on text as well as lists, decided from the kinds like `and`), `starts with`, `ends with` (and `does not` / `doesn't` forms), and `the position of "x" in word`, which is **0 when it isn't there** (positions start at 1); it works for list items too (`the position of 95 in scores`).
  - **Splitting and joining:** `the words of text`, `the lines of text`, `text split by ","`, `names joined with ", "` (lists of text, numbers or yes/no values).
  - **Changing (into new text):** `word in uppercase`, `word in lowercase` (English, accented Latin, Greek and Cyrillic letters), `text without extra spaces` (at the start and end), `text with "a" replaced by "b"`.
  - **Characters:** `C is a letter` / `is a digit` / `is a space` (and `is not`, `isn't`), yes only for exactly one such character; they need the `a`, so `letter is letter` still compares with a variable called `letter`. `the code of C` and `the character with code 65`.
  - **Text built piece by piece grows in place.** `set T to T followed by X` (or `and X`) adds to T where it is when nothing else holds it, doubling its room when full, instead of copying it every time: a million pieces went from 129.7 s and 147 MB to about 5 ms and 6 MB. Every other holder of the old text still sees the old text. A new `build` benchmark measures it against C, Go and Python.
  - **`as a number`** on text that isn't a number now adds a hint ("A number is digits, with a "-" before them if it's negative and a "." for decimals, like 42, -7 or 3.5."), says "empty text" for empty text, and cuts long text at its 60th character (not byte) with "...". Constants give the same message.
  - Runtime errors for characters, ranges and counts, splitting by or replacing empty text, the code of anything but one character, and character codes that aren't one; compile errors for every kind mistake, and for text tools in constants ("A constant can't take text apart or look inside it.").
  - `examples/programs/word_counter.es` (reads a file, counts its words, and reports the most common ones), moved off the planned list in the roadmap. Tests for every form, Unicode text, empty text, every error, in-place building and its sharing rules, and text tools in a 100,000-round loop under the memory limit.
  - Inside the compiler: `src/front/parse_text.c`, `src/front/check_text.c`, `src/back/codegen_text.c` and `runtime/es_text.h` (the runtime's fifth part). `EsText` has two new fields, `count` and `cap`.

- **Joining lists:** `followed by` between two lists of the same kind makes a new list (`the morning slots followed by the evening slots`), of exactly the right size, reusing the left list when nothing else holds it, with lists inside copied so it shares none. Lists of different kinds are a compile error; anything other than two lists still joins as text. The kind is worked out like `and`'s (a deferred check), so it also works for function inputs whose kinds come from calls.

- **Fixed lists:** `keep` works for lists written out of fixed numbers, text or yes/no values (`keep weekdays as a list of "Mon", "Tue" and "Wed".`). Adding, removing, setting, inserting or sorting on one is a compile error that suggests `let`. A fixed list is a static list in the program, never counted or freed; reading it uses it directly, and storing it in a variable or giving it to a function makes a copy that can change. `length of` a fixed list is fixed too. A fixed list can't hold lists.

- **English stack traces in runtime errors.** A runtime error now shows the line with carets under the part that failed, like a compile error, and inside functions the chain of calls that led there: `while working out "average" (count was 0)`, `called from "report" on line 5`, `called from line 8: say report`. Inputs are shown for numbers, yes/no and text (cut short after 30 characters); a function calling itself from the same place is collapsed (`...it called itself 4,998 more times...`). The runtime's call-depth counter became a call stack of frames (the function, the call's site, the values it was given); a call costs three more stores, about 3% on the recursive `fib` benchmark and nothing measurable on the others. Generated programs carry their source lines (`es_source`) and a table of sites (`es_sites`) for this.
- **`every other item of scores`:** items 1, 3, 5, ... (`every second item` is 2, 4, 6, ...).

- **Names of several words:** `let late fee be 25.`, `keep hourly rate as 150.`, `to add tax to amount:` (the function "add tax" with the input "amount"), `count from 1 to 3 as session number:`, `for each guest name in guests:`, `ask ... and call the answer visitor name`. A pre-scan finds every name the program makes with `let`, `keep`, `to`, `as`, `call`, `for each` and `every`, and values then match the longest known name. Names can't contain words that already mean something in a sentence (`plus`, `is`, `and`, `of`, `with`, `to`, ...), with an error suggesting another name. Functions defined with `to` before their inputs are called with `to` (`add tax to 100`), and a sentence starting with a function's name calls it even if the name starts with a sentence word. Messages show names as first written (`clientName`, `HOURLY_RATE`); generated C writes the spaces as `_`. The examples now use natural names (`tax rate`, `notes file`, `most tries`, `top names`, `add tax to amount`).

- **List slices:** `items 2 to 4 of scores`, `items 3 to the end of scores`, `the first 3 items in scores`, `the last 2 items in scores`, `scores without the first item` / `without the last item`, and `every second item of scores` (items 2, 4, 6, ...; any ordinal: `third` ... `tenth`, or `3rd`, `12th`, `21st`). Ranges are 1-based and include both ends; the first and last N give back the whole list when it's shorter; positions that aren't in the list are runtime errors naming the position and the length. Slices are new lists, with the lists inside them copied. Ordinals (`2nd`, `3rd`, `21st`) are a new token; a wrong ending (`2th`) is an error that suggests the right one. `grade_statistics.es` uses slices.

- **Lists** (Phase 2, step 15). A list holds values of one kind, in order. See the new [Lists chapter](docs/language-guide.md#10-lists).
  - Making them: `let scores be a list of 72, 95 and 88.`, `let names be an empty list of text.`, lists of lists (`a list of (a list of 1 and 2) and (a list of 3)`), `a copy of scores` (a separate list, copying the lists inside it too).
  - Items, from position 1: `item 2 of scores`, `the first item in scores`, `the last item of scores`. A position that isn't there stops the program with the position and the length: "There's no item 4 in the list: it has 3 items."
  - Changing them: `add 5 to the end of scores`, `insert 7 at position 1 of scores`, `remove item 2 from scores` (or the first or last item), `set item 2 of scores to 100`, and `sort scores` (`, biggest first`; `by their length, longest first`), a stable merge sort with the comparison inlined. `add X to L` adds to the end when L is a list that doesn't hold numbers; with a list of numbers it could also mean adding to each number, so it's an error that shows both ways to say it.
  - Asking: `the length of scores` / `the number of items in scores`, `scores contains 95` (`does not contain`), `scores is empty` (also for text), and `is` between whole lists.
  - Going through them: `for each score in scores:` (with `it` as the item; the loop goes through the items the list had when it started), `every score in scores plus 5`, `only the scores that are at least 70` (with every comparison, `are` for `is`), `scores added together`, `the biggest of scores`, `the smallest of scores`, `scores reversed`.
  - Lists are shared: every name for a list sees its changes, and functions can change the lists they're given. They're reference counted with everything in them, and a list can never hold itself, so nothing leaks. `say scores` prints "72, 95 and 88".
  - Kinds: a list's kind is inferred like any other ("\"scores\" is a list of numbers, so it can't hold text."), and definitions can say `(a list of numbers)`, `(a list of lists of text)`, `giving back a list of text`.
  - `examples/programs/grade_statistics.es` and `top_scores.es`; a `sort` benchmark against C's `qsort`, Go and Python (EasyScript 0.06 s, C 0.08 s, Go 0.08 s, Python 0.31 s); tests for every list form and error, sharing and copying, nesting, and lists in million-round loops under the memory limit.

- **Static types with inference** (Phase 2, step 14). The compiler works out whether every value is a number, text, or yes/no, so most programs need no annotations. See the new [Types chapter](docs/language-guide.md#9-types).
  - Variables take their kind from their `let` and keep it ("\"total\" is a number (made on line 1), but this is text."). `ask` and reading a file give text; `length of` gives a number; constants take the kind of their value.
  - Function inputs and results are inferred from how they're used and called; calling a function with different kinds, giving back two kinds, or using a function that gives back nothing as a value are friendly compile errors. When nothing decides a kind, the error suggests writing it.
  - Optional kinds in definitions: `to area with width (a number) and height (a number), giving back a number:` (`(text)`, `(yes or no)`, `giving back nothing`). The wording and the alternatives considered are in the guide.
  - Kind mistakes that used to stop the program (text in arithmetic, a condition that isn't yes or no, `and` between yes/no and text, comparing a number with text, ...) are now compile errors with the same wording plus the source line and carets. `and` is resolved before the program runs: logical for yes/no, joining for text.
  - A function that gives back a value but reaches its end without `give back` stops with "The function \"sign\" ended without giving anything back."
  - `examples/programs/kinds.es`; tests for inference, every new error, annotations, and conflicting calls; the constant/runtime cross-check test grew from 11 to 23 pairs.

- **Memory management with reference counting** (Phase 2, step 13). Text made while a program runs (joins, numbers turned into text, `ask` answers, file contents) is freed as soon as nothing uses it, so loops run in flat memory: the text benchmark went from 280 MB to 1.8 MB, and a million-round loop that makes text stays under 2 MB. Programs don't change; there's nothing to write. See [docs/memory.md](docs/memory.md).
  - Every heap object has a common header (kind and reference count), ready for lists and records. Text literals and constants are static objects marked immortal.
  - One ownership convention throughout: expressions produce owned values, operations and functions consume their arguments, reading a variable retains, storing releases the old value. Blocks release their names when they end, including through `stop the loop`, `skip this one`, `give back`, and `stop the program`.
  - `examples/programs/million_lines.es`.
- **Memory checks:** `ES_DEBUG_MEMORY=1` makes a program fail (exit 70) if anything is still alive at its end, and every test program now runs this way. `ES_MEMORY_LIMIT=N` fails a program whose live text goes over N bytes; `tests/memory/` runs million-round loops under a limit and checks their peak memory. A control test makes sure the check catches a missing release.
- **Continuous integration:** a GitHub Actions workflow (`.github/workflows/ci.yml`) runs `make`, `make test` and `make test-debug` on every push, on Ubuntu with GCC and on macOS with Clang.
- **Strict C check:** `make test` compiles the C generated for every test program, example and benchmark with `-Wall -Wextra -Wpedantic -Werror -O2`, using the system compiler.
- **Sanitized programs in `make test-debug`:** `ES_SANITIZE=1` makes the compiler build programs with AddressSanitizer and UBSan, so a use after free or double free is caught. On macOS the leak sweep now also covers the memory tests.

### Changed

- **Leaner lists.** Numbers are sorted in place with pdqsort instead of a merge sort with a second buffer (text and sorting by length keep the stable merge sort), lists made with a known size get exactly that size, `only`'s results are shrunk to fit, and slices and `a copy of` reuse a list nobody else holds instead of copying it. The sort benchmark went from 16.5 MB (17.3 bytes per item) to 9.0 MB (9.4; C's `qsort` program is 9.3), at the same speed. Growing lists by 1.5x instead of doubling was tried and dropped: on macOS it made realloc move the last big block with a deferred copy that the sort then paid for (14.3 MB). "not a number" now sorts last. `make bench` prints bytes per item for the sort benchmark; the sorting code is in the new `runtime/es_sort.h`.
- **A function's name is every word before `with`, `of`, `using` or `to`.** So `to greet someone:` now defines a function called `greet someone` with no inputs; write `to greet with someone:` for an input. Using `someone` in its body, or calling `greet "Paris"`, gives an error that explains this and shows the fix. The tests and docs use `with`.

- The macOS leak sweep leaves out programs that stop with a runtime error (they exit without clean-up by design, so `leaks` could report their variables depending on register allocation). Numbers shown in runtime error messages are kept reachable (`es_shown`).
- `it` is also the item of a `for each` loop, and `length of` also counts a list's items, so their error notes mention lists. The memory check's messages say "values" and "text and lists" instead of "text". `and` between text and a list joins them, like `followed by`.

- **Generated code uses plain C types:** numbers are `double`, yes/no is `bool`, text is `EsText *`, and a function that gives back nothing is `void`. The tagged value is gone from generated code entirely; only text is reference counted, so no retain or release is generated for numbers or yes/no. Nested loops went from 0.85 s to 0.10 s and counting from 0.07 s to 0.05 s; `mod` uses the integer remainder for whole numbers, and counting loops work out their last round once.
- `nothing` can no longer be used as a value (it's only what a function that gives back no value gives back), and comparing values of different kinds is an error instead of always "no". `give back nothing` is the same as `return`.
- The runtime's first file is now `runtime/es_memory.h` (it was `es_value.h`). Compile errors are listed in source order.

- **Faster function calls:** a value is now 16 bytes instead of 48, so it's passed in registers. Recursive fib(38) went from 0.52 s to 0.24 s.
- The runtime is split into `runtime/es_value.h` (values and memory) and `runtime/es_runtime.h` (operations); `tools/embed.c` joins them. The variables of the main program are now C locals of `main()`.

### Fixed

- A list form whose list was written out (`item 1 of a list of "a" and "b"`) generated the list's C code twice, leaving unused static text that `-Wunused-variable` rejects in strict builds; it's generated once now.
- A function with no inputs couldn't say what it gives back with a comma (`to roll, giving back a number:`), although the vocabulary showed it.
- The error audit (`tests/tools/error_coverage.py`) could take minutes on messages with several `%s`; each now only matches within a line.

- **Linux:** the compiler didn't link, because `consteval.c` needs the math library; the Makefile now links `-lm` (generated programs already did).
- **GCC:** every generated program printed a `-Wfree-nonheap-object` warning, because GCC couldn't tell that releasing immortal text never reaches `free()`. Freeing now happens in a separate function that is never inlined or specialized (`noipa` on GCC), so the free path is visibly separate from immortal objects. GCC's `-Winfinite-recursion` is turned off in generated programs: a function that only calls itself is reported by EasyScript while the program runs.
- `ES_MEMORY_LIMIT` now stops the program before allocating the text that would go over the limit, not after, so LeakSanitizer doesn't report that text as leaked.
- Text made while a program ran was only freed when it ended, so long loops that built text kept growing (a known issue in 0.1.0).

## [0.1.0] - 2026-10-10

**Phase 1 (core language) is complete.** EasyScript programs written in plain English sentences compile to C and run: variables and constants, numbers, text and yes/no values, arithmetic and comparisons in words or symbols, `if`/`otherwise`, loops, functions with recursion, output, input, and files, with friendly compile-time and runtime errors. Every error message is tested, every accepted phrase is documented, `make test-debug` finds no sanitizer errors or leaks, and `make bench` compares EasyScript with C, Go, and Python. Four planned items moved to Phase 2: reclaiming memory while programs run, type checking, runtime errors that show the source line, and `file ... exists`.

### Added

- **Leak checking everywhere:** `make test-debug` now also runs `tests/leaks.sh` on macOS (every test input, generated program, and shell session under the system `leaks` tool), where AddressSanitizer can't find leaks; Linux uses LeakSanitizer.

- **Larger examples** that use everything together: `fizzbuzz.es`, `guessing_game.es`, `notes.es`, `hanoi.es`, and `receipt.es` (all run by `make test`). A word counter is planned for Phase 2, once there are text tools to split text.
- **A better interactive shell:** earlier output isn't printed again, earlier `ask` questions aren't asked again (their answers are replayed), a line ending in `:` starts a block (if, loop, function) that ends at a blank line, and entries with errors are shown and dropped. `exit`, `quit`, or `stop the program` leaves. Tested by `tests/shell/`.
- **Benchmarks:** `benchmarks/` (recursive Fibonacci, a counting loop to 100 million, nested loops, building a large text) in EasyScript, C, Go, and Python, and `make bench`, which prints time and peak memory and checks that every language gives the same result. Results and history are in `docs/performance.md`; the README has a summary.
- **Constants:** `keep NAME as X` (or `at X`), top level only. The compiler works the value out before the program runs (numbers, text, yes/no, arithmetic, joining, comparisons, logic, conversions, `length of`, and earlier constants) and generates a C static initializer, so constants cost nothing at run time. Constants are visible everywhere, including inside functions.
  - Compile-time errors: values only known while the program runs (variables, function calls, `it`, file contents), later constants or itself, changing a constant (with a suggestion to use `let`), name clashes with variables, inputs, loop numbers, or functions, duplicates, constants inside blocks, and mistakes in the value (worded like the runtime's errors).
  - `examples/programs/tax.es`: a tax rate used by a function.
- **Functions** (top level only): `to greet someone:` / `to area with width and height:` (also `of`/`using`), `give back E` / `return E` / `return`, calls as sentences (`greet "Paris".`, `greet with "Paris".`, `call greet with "Paris".`) and as values (`area of 3 and 4`, `area with 3 and 4`; a bare name calls a function with no inputs).
  - Functions can be used before they're defined, and can call themselves. Each sees only its inputs and the names it makes; its locals are hoisted to the top of the generated C function.
  - Compile-time errors with suggestions: unknown functions ("Did you mean "area"? It's defined on line 1."), the wrong number of inputs (pointing to the definition), `give back` outside a function, functions defined inside blocks, duplicate functions or inputs, and variables named like functions.
  - Endless recursion stops with a friendly runtime error instead of crashing.
  - `and` straight after a call's input now starts another input (`greet using name and age` gives two inputs); a parenthesized last input ends the call, so `fib of (n minus 1) plus 1` adds.
  - Examples: `factorial.es`, `fibonacci.es`; the README's bigger example now runs and is tested.
- **Loops:** `count from A to B` (counts down by itself when A is bigger; `by S` / `in steps of S`; `and call each number N` / `as N`, otherwise the number is called `number`), `count down from`, `go from A to B in steps of S`, `for each N from A to B`, `do this N times` / `repeat N times`, `while` / `as long as` / `repeat while`, `keep doing this until` / `repeat until`, and `forever`.
  - `stop the loop` (`stop`, `break`) and `skip this one` (`skip`, `continue`, `move on`); both are errors outside a loop.
  - `it` is the innermost counting loop's number (or the round of a `times` loop); `it` outside one is an error.
  - A loop's number and names made in its body exist only inside the loop; a loop's number can't reuse an existing name.
  - Friendly runtime errors for counts that aren't numbers, steps that aren't more than zero, bad `times` counts, and loop conditions that aren't yes or no.
  - `examples/programs/loops.es`, and `logan.es`: the 2024 prototype's loop example ported to the new syntax.
- **Decisions:** `if CONDITION:` with an indented block, any number of `otherwise if CONDITION:`, and a final `otherwise:` (`else` works too). One-line forms: `if x is 5, say "hi".` and `if x is 5 then say "hi".`
  - An `otherwise` must line up with its `if`; a misplaced one, one with no `if` above it, one after a one-line `if`, or a second plain `otherwise` gets a friendly error saying what's wrong.
  - Conditions must be yes or no; anything else is a runtime error: "Line 3: An "if" needs yes or no to decide, but this is a number."
  - Names made inside a block exist only inside it ("You made "secret" inside the "if" on line 2, so it only exists inside that block.").
- An unexpected indented block is now skipped as a whole after its error, so it can't end an enclosing block early.

### Changed

- **Number equality has a tolerance:** numbers within a relative 1e-12 of each other count as equal (so `1000000000 is 1000000000.5` is still `no`), so `0.1 plus 0.2 is 0.3` is `yes`. `is at most`/`is at least` and the other ordering comparisons agree with it.
- **`followed by` binds more loosely than `plus` and `minus`,** so the maths happens before joining: `"Sum: " followed by 2 plus 3` gives `Sum: 5`. It still binds more tightly than comparisons.
- `main.c` moved to `src/main.c`, and `.vscode/` is no longer tracked.

### Added

- **Programs run.** `run`, `build`, and `emit` now use the new pipeline: lexer → parser → checker → C code generator → `cc -O2`.
  - **C backend** (`src/back/codegen_c.c`): top-level variables become C globals; evaluation is strictly left to right; `and`/`or` short-circuit.
  - **Runtime** (`runtime/es_runtime.h`), embedded in the compiler by `tools/embed.c`, so generated programs are self-contained. Values are nothing, numbers, text, or yes/no. Whole numbers print without decimals. `and` joins text or means logical and, decided while the program runs.
  - **Friendly runtime errors** with line numbers, e.g. "Line 3: I can't subtract text from a number." and "Line 8: You divided by zero."
  - **Checker** (`src/front/check.c`) for names: "I don't know anything called "totl". Did you mean "total"? You made it on line 1.", names used before they're made, names made twice, and functions and `it` (not available yet).
  - `write`/`append` end what they write with a new line, and reading a file leaves off its final new line.
- The interactive shell runs the new syntax and only keeps lines that work.
- `make test` checks that the README's programs match their example files. Run tests can provide input (`NAME.in`) and expect runtime errors (`NAME.err`).
- **Roadmap:** the planned EasyScript Notebook (a chat-style notebook, Jupyter kernel first; see `docs/roadmap.md`).
- **Statement parser** (`src/front/parse_stmt.c`). A program is now a list of statements, each ended by a period or the end of its line, and several can share a line:
  - `let X be E` / `let X equal E`, `set X to E` / `change X to E`
  - `add E to X`, `subtract E from X`, `increase X by E`, `decrease X by E`, `multiply X by E`, `divide X by E`
  - `say E`, with synonyms `print`, `show`, `display`, and plain `write`
  - `ask E and call the answer X`
  - `write E to file F`, `append E to file F`, `read file F and call it X`
  - `stop the program`, and a leading `please` is skipped
  - Errors: unknown first words suggest the closest statement word; `let a be 5` explains that `a`/`an`/`the` are ignored filler; `let yes be 1` explains reserved words; missing or wrong connecting words show a correct example; words after a finished statement are reported.
- The lexer records where it dropped a filler word (`Token.filler`); `easyscript tokens` shows it as `[after "the"]`.
- Suggestions now treat two swapped letters as one typo (`sya` suggests `say`).
- **Expression parser** (`src/front/parse_expr.c`) and **syntax tree** (`src/common/ast.h`). `easyscript ast FILE` prints the tree. For now a file is one expression per line (or several, separated by periods).
  - Precedence, loosest first: `or`, `and` (logical or text joining, decided later by the checker; stops before `and call`), `not`, comparisons, `followed by` (since moved below arithmetic), `plus`/`minus`, `times`/`multiplied by`/`divided by`/`mod`, unary minus, `as a number`/`as text`, then values.
  - Comparisons in words and symbols: `is`, `is not`, `isn't`, `equals`, `is equal to`, `is greater/more/bigger than`, `is above/over`, `is less/smaller than`, `is below/under`, `is at least`, `is at most`, the "or equal to" forms, `reaches`, `= == != < > <= >=`. Comparisons don't chain.
  - Values: numbers, text, `yes`/`no`/`true`/`false`, `nothing`, `it`, names, parentheses, `NAME using X, Y` calls, `length of X`, and `contents of file X`.
  - Errors with suggestions: phrases missing their last word ("Did you mean "is greater than"?"), `is not greater than`, chained comparisons, ambiguous call arguments, misspelled operators, unmatched parentheses, `.5`. The parser recovers at the next sentence.
- `==` is now a token.
- `examples/parser/` and `tests/ast/`, checked by `make test`. They're written as statements now; expression-only files are gone.
- **Documentation:** a new README, `docs/` (language guide, vocabulary, errors, architecture, roadmap), `CONTRIBUTING.md`, and this changelog.
- **Examples:** `examples/programs/` (runnable programs), `examples/lexer/`, and `examples/parser/`. Every example is checked by `make test`.
- **New lexer** (`src/front/lexer.c`) for the sentence syntax:
  - sentences ending in a period or a new line; blocks from a colon and indentation (`INDENT`/`DEDENT`, with a tab counting as 4 spaces)
  - lowercased words, with filler words (`the`, `a`, `an`) dropped and apostrophes allowed inside words
  - whole numbers and decimals (`5.` is 5 and then a period); text with `\n \t \" \\` escapes
  - the symbols `. , : ( ) + - * / % = < > <= >= !=`, and comments with `#` and `note:`
  - friendly errors for bad indentation, unterminated text, unknown escapes, letters stuck to numbers, curly quotes, and stray characters, all reported in one run
- **`easyscript tokens FILE`** to print the lexer's tokens.
- **Diagnostics** (`src/common/diag.c`): errors show `Line N:`, the source line, carets under the problem, and suggestions. Every error from a run is collected and shown together.
- **Foundation** (`src/common/`): arena allocator, growable string builder, dynamic arrays, and edit distance.
- **CLI commands** `run`, `build -o`, and `emit`. Generated C and binaries go in a temporary directory instead of the current folder.
- **Tests:** `make test` runs unit, lexer, program, compile-error, example, and CLI tests with a pass/fail summary. `make debug` and `make test-debug` build and test with AddressSanitizer and UndefinedBehaviorSanitizer.

### Changed

- The build uses `cc`, puts objects in `build/`, and tracks header dependencies automatically. It builds with no warnings.
- The interactive shell keeps its scratch file in a temporary directory and no longer has a line-length limit.
- The old prototype examples moved to `old/` (historical; they no longer compile).

### Fixed

- `FILE` followed by an unknown action no longer uses an uninitialized pointer. It now reports an error.
- Compiler warnings: sign comparisons in the lexer and code generator. The lexer also no longer reads past the end of an empty file.

### Removed

- Committed build outputs (`*.o`, binaries, generated C) and empty scratch files. A `.gitignore` now covers them.

### Removed

- The 2024 prototype pipeline (uppercase `PRINT #` / `MAKE A VARIABLE` syntax): its lexer, parser, and code generator, and the tests and examples written in it. That also removes its known bugs (a buffer overflow on words over 255 characters, the extra blank line after `FILE READ`, and redeclared variables reading the first declaration).

### Changed (build)

- The build uses `-std=c11`, and `make test-debug` no longer turns leak checking off (the compiler has no leaks).

### Known issues

- Text made while a program runs is freed only when it exits, so a long loop that builds text grows (about 20 bytes per round in a simple case).
- `stop` on its own now means "stop the loop"; to end the program, write `stop the program`.
- Functions can't see the program's other variables yet (pass values in as inputs).
- In the shell, error line numbers count from the start of the session, and every entry reruns the whole session (so slow sessions get slower, and files are written again).

## 2024-07-06: First prototype

- The original EasyScript prototype: an uppercase command syntax (`MAKE A VARIABLE x ASSIGN 10`, `PRINT # x`, `FILE OPEN/WRITE/READ/CLOSE`) compiled to C, plus an interactive shell.
