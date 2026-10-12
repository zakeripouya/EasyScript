# English pass: work log

A running record of the unattended English-pass run (parts 2 to 5, then charts), on the `english-pass` branch. Part 1 was finished before this run (see the update log in [english-pass.md](english-pass.md)). The plan and decisions live in [english-pass.md](english-pass.md); this file records what was done, when, and what's left for review.

## Summary (for review)

**Finished:** parts 2, 3, 4 and 5 of the English pass, then the charts module. Part 1 was done before this run. After review, the branch was merged into master (see [After review](#after-review-argument-order-cleanup-examples): argument-order fixes for GCC on Linux, the "Start here" examples rewritten, [Still not English](#still-not-english), and [proposals/](proposals/README.md)).

**Tags:** `english-pass-part-2` (e3c55e2), `english-pass-part-3` (ffbaafb), `english-pass-part-4` (8d896aa), `english-pass-part-5` (9359cbc). `english-pass-charts` (c10d371), tagged after review. There's no `english-pass-part-1` tag: part 1 was finished before the branch.

**Tests at the end:** `make test` 712 passed, `make test-gcc` 712 passed, `make test-debug` 711 passed (it skips the strict-C check by design) with 0 leaking processes. Up from 629 on master. `make bench` passes, including the 20% rule for every high-level form.

**Skipped, and why** (each also in [english-pass.md's Later list](english-pass.md#later) and under Open questions):

- **Exact money amounts and units of measure** (part 4, items 5 and 8): each needs a new kind of value through the checker, code generation and runtime: a step of its own. `as money`, `as a percent` and `with commas` were built.
- **Named kinds of text with rules** (part 4, item 11, second half): the plan doesn't say how a value becomes one (`"a@b" as an email`?). Guards in `check which kind` were built.
- **`try to` beyond files and folders**: built as a check before the sentence (no safe way to undo a half-done sentence in the generated C), so it covers file and folder sentences only.
- **Actions with inputs in the middle** (`give every loyal customer a 10 percent discount`) and **the best group** (`the store with the highest total amount`): no settled wording.
- **`they ordered more than 5 times`** (part 3) and **`each drink in drinks's name`**: logged as rough spots.

**Open questions for you** (details under [Open questions](english-pass.md#open-questions)):

1. Actions with inputs in the middle: how should a definition name them?
2. Should `try to` cover other failures, and how?
3. Exact money (`$19.99`) and units: as kinds of value, how should they mix with plain numbers? Other currencies in `as money`?
4. Named kinds of text: how does a value become one, and where can it be used as text?
5. The best group: should `the X with the highest total Y` group by X when X is a field?
6. Who are the first non-coder testers?

**Breaking changes** (all marked in [CHANGELOG.md](../CHANGELOG.md)): `total`, `average`, `median` and the other question words start phrases when no definition has the name (`total of 3` with a variable `total`); `F of X` where F is both a definition and a field is an error; `every`, `whenever` and `when` start event sentences at the top level; a word a kind defines with `is ... if` is that word everywhere; sentences starting `NAME must`, `NAME depends on`, `KIND's FIELD is` and `the NAME is the one ...` now define things; `me` and `undo` are reserved, `tell` and `list` start sentences; in a program that reads a table, a misspelled field is reported by the checker; a table's rows are a kind named after it (`purchases`: `a purchase`), so they print as `a purchase with ...`. Changed expected files (each read before it was rewritten): `tests/ast/err_library.err`, `err_missing_values.err` (and `.out`), `err_missing_words.err` (and `.out`), `err_names.err`, `err_stmt_unknown.err`, `tests/errors/call_variable.err`, `iface_errors.err`, `library_errors.err`, `tests/tokens/err_unknown_escape.err`, `tests/run/table_files.out`. `make bless` was never run; new expected files were written from runs whose output was checked.

**Benchmarks** (Apple M4, best of 3; [performance.md](performance.md)): fib 0.25 s, count 0.05 s, nested 0.11 s, sort 0.06 s and 9.0 MB, words 0.10 s, pipeline 0.13 s, sales 0.04 s: unchanged from step 21. Every list question is at most as slow as its hand-written loop (ratios 0.03 to 1.05; `sorted by value` went from 2.1 to 1.00 in part 5). Found and fixed on the way: `benchmarks/run.sh` never actually applied its 20% check, and two benchmarks didn't build since part 1.

**Worth a look first:** the "Your first programs" chapter in the [language guide](language-guide.md#your-first-programs); `examples/programs/budget.es`, `groceries.es`, `best_months.es` (with its chart) and `tip_calculator.es`; `lib/charts.es` (a module written in EasyScript).

**Checks before every commit:** `make test`, `make test-gcc` and `make test-debug` (with the leak sweep), all passing. Baseline on master before the run: 629 / 629 / 628 tests, 0 leaks (test-debug skips one strict-C check by design).

## Part 2: questions and totals

### Done

- **Questions about lists** as one AST node (`EXPR_AGGREGATE`), each compiled to one C function that goes through the list once:
  `how many L are|is TEST`, `how many L there are`, `the number of L`, `any|every|no ITEM TEST` (list by plural), `any|no ITEM in L TEST`, `all/any of/none of L TEST`, `the total|average|median [FIELD] of L`, `the top|lowest N L [by FIELD]`, `the N most common L`, `the ITEM [in L] with the highest|lowest|most|fewest... FIELD`, and `... per FIELD` / `per month|year|day|weekday [of FIELD]`.
- `any`/`every`/`no` stop at the first item that decides the answer; the top N use a heap of N (never sorting the list); a filter written in the list (`the orders whose amount is over 50`) is tested in the same pass unless its test calls a definition.
- Maps sorted by their values (`M sorted biggest first`, `sort M`).
- Math words: `half of`, `twice`, `N percent of`, `the remainder of X divided by Y`, `X is infinite`, and `X divided by Y, rounded down` (a comma before `rounded`/`sorted`/`as` applies to the whole value).
- Plural verbs after `that` and in `how many` (`start with`, `contain`, `don't contain`, ...).
- `easyscript plan FILE` / `easyscript build --plan FILE`, produced by the code generator (`cg_plan`), golden-tested in `tests/plan/`.
- Examples: `examples/programs/gradebook.es`, `examples/programs/order_report.es` (the doc's sales report with part 2 forms).

### Decisions made (the doc didn't settle these; each picks the option that changes least)

- **Group keys for `per month`:** text like "October 2026" (days "Sunday, October 11, 2026", weekdays "Sunday"; years are numbers), so dates didn't need to become map keys.
- **`per month` with no field named:** uses the item's one date field; none or several is an error asking `per month of which date` (like `for each score:`'s fixed plural rule).
- **Built-in phrases win over fields** (as part 1 already decided for `the length of`): `the total of rex` with a field `total` is an error with a note suggesting `rex's total`.
- **A definition named like a question word keeps its meaning** (`total of marks` still calls `to total of marks:`); with `the` it's an error asking which. 19 existing tests and examples define `total`, `average`, `half` or `twice`, and all keep working.
- **`total of 3` where `total` is a name holding a number** now reads as the phrase (breaking, in the changelog); `tests/errors/call_variable.es` uses `price` instead so it still tests "can't be called".
- **Ties:** the item that came first wins (top N, the one with the highest, most common: the first to come up).

### Skipped or moved (with reasons)

- **`the one with the most Y`** needs a list to refer to; it fits part 3's definitions (`the best customers are the ones with ...`), so it moves there.
- **Action forms** (`total the amount per month`, `count the big orders`) only make sense with somewhere to put the result: they move to part 4's `, then` chains, where `it` is the previous step's result. `sort it biggest first` already exists as `sort L biggest first` (now for maps too).

### Benchmarks (the speed rule)

`benchmarks/forms/` has a pair per form (`NAME.es`, `NAME_loop.es`); `make bench` fails if a form prints a different result or takes more than 20% longer than its loop. Every form came out faster (ratios 0.03 to 0.74; the table is in [performance.md](performance.md#questions-about-lists-against-hand-written-loops)).

### Commits

- `5bc8505` Add questions and totals about lists, math words and easyscript plan (test 647, gcc 647, debug 646 + 0 leaks). Blessed nothing; new golden files written by hand-checked runs: `tests/run/questions*`, `math_words`, `err_average_empty`, `err_median_empty`, `err_best_empty`, `err_top_count`, `tests/errors/question_errors`, `tests/ast/questions`, `err_questions`, `tests/plan/*`, `tests/memory/questions_loop`, the two examples. Changed: `tests/errors/call_variable.es/.err` (uses `price`, see decisions).

## Part 3: choices without if

### Done

- **Defaults:** `X, or D [if there isn't one]` for anything that can be nothing (not just map lookups), and `add 1 to the value for word in counts, starting at 0` (every change sentence, and lists: `starting at an empty list of text`). The start is only worked out when the key is missing.
- **Describing words:** `a customer is loyal if ...` and `a big order is one whose ...`, used as `if rex is loyal`, `the loyal customers`, `how many orders are big`, `every loyal customer's ...`. They compile to methods named "is loyal" (a name a program can't write), so several kinds can share a word.
- **Values and named values:** `an order's profit is its amount minus its cost.` (used like a field everywhere, including `the total profit of orders`), `the best customer is the one with the highest profit.`, `the big orders are the ones whose ...`.
- **Tables:** `the shipping cost depends on the total:` with `VALUE when it's ...` / `VALUE when it starts with ...` lines and a required `VALUE otherwise.`; `the price of a drink depends on its size:` for a kind's value. A missing `otherwise` names the missing range when the lines are all `over`/`at least` (or `under`/`at most`).
- **Rules:** `an age must be between 0 and 150.`, `a customer's email must contain "@".`, `a team's players must not be empty.`, checked in `new`, `set` and the change sentences; the runtime error quotes the rule.
- **Groups:** `remove the tasks that are done.`, `remove the finished tasks.`, `set every monster's color to "blue".`, `add 10 to every player's score.`
- Example: `examples/programs/shipping.es`.

### Decisions made

- **Describing vs naming:** `a big order is one whose ...` defines a word; `the best customer is the one with ...` names a value. The dropped `the` before `one` decides, as in English (recorded as a known exception).
- **A defined word wins in `X is WORD`**, even over a name `WORD` (recorded as a known exception).
- **Named values are read where they're used**, so they see the names there and are always current.
- **A table that makes a name works it out where it's written** (like `let`): the doc didn't say when it's worked out; this changes least, and "always up to date" values are part 4.
- **`the one whose ...` and `the ones with the highest ...` are errors** (one might match several; the other doesn't say how many).
- **Rules about a field of any kind** (`an age must ...`) cover every kind with that field, which must all be the same kind.

### Skipped or moved

- **`give every loyal customer a 10 percent discount`**: a method whose inputs sit in the middle of its words ("give ... a ... discount") doesn't exist yet; `set every loyal customer's discount to 10` works. Open question added.
- **`a customer is loyal if they ordered more than 5 times`** (a verb about the record): "they" + a verb isn't parsed; `if the number of items in their orders is above 5` works.

### Commits
- `8866ac3` Add choices without if: defaults, describing words, values, tables, rules, groups (test 661, gcc 661, debug 660 + 0 leaks).
- (next) Events and schedules, Snake with events, sentence definitions written in place, examples and benchmarks; tagged `english-pass-part-3`.

### Events (part 3, item 7)

- `every N seconds|minutes|hours|milliseconds:`, `every day at 9pm:`, `every Monday at 9:30am:`, `every Friday at 17:00:`, `whenever C:` / `whenever C, S`, `when C, S`, `whenever a key is pressed:` (`it` is the key).
- **Decided:** events run in one loop after the program's other sentences; `whenever` fires when its condition becomes yes (checked after the other sentences and after each event), `when` fires once; keys from a file (tests, or piped input) come one before each round of the schedules, so runs repeat exactly; the program ends with `stop the program`, or when there are no schedules and (if it reads keys) the keys run out.
- Snake (`examples/programs/snake/play.es`) is rewritten with `whenever a key is pressed:`, `every 150 milliseconds:` and `whenever g's over:`.
- `tests/leaks.sh` now builds programs with the test hooks and the fixed clock, like `tests/run.sh`, so programs with events or `wait` don't wait for real.
- **Speed rule:** a describing word used in a question (`how many entries are big`) was 70% slower than the hand-written loop as a call; sentence definitions are now written in place at their calls (1.00 of the loop). Tables are 0.69.
- Examples: `coffee_shop.es` (the plan's coffee shop without its web parts) and `shipping.es`.

### Rough spots found

- `each drink in drinks's name` can't be written: the `'s` attaches to the list. Left for part 5's rough spots.

## Part 4: talking to it

### Done (first commit)

- Questions ending in `?`, requests (`show me`, `give me`, `tell me`, `list`), polite words (`I want to`, `I'd like to`, `let's`, `can you`, `could you`, `would you`, `please`).
- Steps in a row (`, then`, `and then`, `and`, `after that`, `next`, `first`, `finally`) with the `it` rule, and actions (`total the amount per month`, `average`, `median`, `count`, `... and call it`, `called`).
- Tables: `read the table in "sales.csv"` learns its columns from the program, checks them when it reads the file ("sales.csv has no column called "amout". Did you mean "amount"?"), keeps every other column for printing and saving; `save X as F` for lists of maps, lists of records and maps.
- The plan's "best months" request compiles and runs (`examples/programs/best_months.es`), without its chart line (charts come after part 5).

### Decisions made

- **`?`** is a sentence end like `.`; a question uses the same words as a program (`what is total?` is `say total`).
- **`it` in steps**: the step before's result is kept in a hidden name only when the next step says `it`; where a loop's or filter's item could also be meant, it's an error asking for a name.
- **Tables:** the plan's `read the table in "sales.csv", then total the amount per month` needs to know the columns before the file exists, so a table's kind of row learns its columns from the program's use (like `has` fields with kinds from use) and the file is checked when read; `per month` on rows uses the column called `date`; columns nothing decides are text. Saved maps get `key` and `value` columns.
- **File sentences keep `file`:** `write "milk" in file it` (not `in it`), as decided in part 1.
- **`me` and `undo` are reserved.**

### Done (second commit)

- `ask for a number` / `ask yes or no`, asking again until the answer fits (item 4).
- `as money`, `as a percent`, `with commas` (item 5); `the NAME is always VALUE.` and `the sum of` (item 6).
- `try to S. if that fails, ...` with `the reason` (item 7), for sentences that use a file or a folder.
- `the text below:` text blocks, `\r` and `\u{e9}` escapes (item 10); `when it's a circle whose radius is over 10:` guards (item 11, first half).
- Example `tip_calculator.es`. Text lexing moved into `src/front/lexer_text.c` (lexer.c was over 600 lines).

### Done (third commit)

- Examples as tests (item 9): `for example, X is Y.` checked where it's written (a warning with the real value when it's wrong, and the program goes on); `easyscript test FILE` says how many were right and fails if any weren't.
- The closest sentences (item 12): an unknown first word with no close statement word gets the 2 or 3 example sentences sharing the most words with it.
- Examples (item 13): `groceries.es` (the plan's multi-step request, word for word but with quotes and `file`) and `budget.es` (the budget session's saved program).
- For those: `, one per line`, `what's in file F` / `what's in it`, `as a table`, `sorted by FIELD`, and table rows named after the table (`purchases` → `a purchase`).

### Decisions made (third commit)

- **Examples warn, they don't stop the program**, as the plan says ("warns when it stops being true"). `easyscript test` is the strict form. A wrong example shows the left side's value only for `X is Y`/`X is not Y`.
- **Closest sentences** are ranked by how many words they share with the sentence (ties: a short list of common sentences first, then the statement table's order), so it's deterministic. They're shown only where the generic "Sentences start with ..." note was; a misspelled statement word keeps its single "Did you mean".
- **`the store with the highest total amount`** (the best group, from the budget session) isn't built: it would need a group's totals inside `the X with the highest Y`. The example writes `the first item of the keys of totals per store` instead. Left for part 5's rough spots.
- **`sorted by amount`** reads a field only when `amount` isn't a definition, a name or a constant and some kind has that field (or the program reads a table); otherwise it's as before.
- **A table's row kind takes the table's name in the singular** (`purchases` → `purchase`: `-ies` → `-y`, `-xes/-ches/-shes/-sses/-zzes` drop `es`, otherwise drop `s`, and the plural rule must give the name back). Breaking: rows print as `a purchase with ...`.
- **`what's in`** only reads files: `what's in file F`, or `what's in it` right after a step that made or wrote to a file. For a list, `show me scores` already works.
- **Named kinds of text with rules** (item 11, second half: `an email is text that must contain "@".`) **skipped**: the plan doesn't say how a value becomes an email (`"a@b" as an email`? any text put where an email is kept?) or what an email can be used as. Logged as an open question.

### Decisions made (second commit)

- **`try to` checks first instead of catching the failure.** Catching it would need `setjmp`/`longjmp` around generated C, which leaves the sentence's half-made text and lists unreleased (the memory check would fail) and isn't safe for C locals. So `try to` covers the sentences whose failures can be checked before they start: file and folder sentences. The check uses the same words as the error. A file that changes between the check and the sentence still stops the program (very unlikely in practice). Other failures (dividing by zero, a missing map key) aren't covered: they have their own forms (`if ... is 0`, `, or D`). Logged as an open question.
- **The file's name in `try to` must be text or a name**, since it's looked at twice.
- **A name a tried sentence makes exists after it either way** (empty if it failed), like a name made in a loop that ran zero times.
- **`as money` is dollars only** for now; other currencies are an open question. It binds like `as text`: `(price times 3) as money`.
- **Exact money amounts ($19.99 kept exactly) and units of measure: skipped** for this run. Both need a new kind of value through the checker, code generation and runtime (exact decimals, or a unit carried by every number's kind), which is a step on its own. Logged under Open questions.
- **Guards don't cover their variant:** `when it's a circle whose radius is over 10:` still needs a plain `when it's a circle:` or `otherwise`, so "every variant covered" stays a check the compiler can do.

### Commits (part 4)

- 61e67be questions, requests, steps in a row, tables read from csv files; 5b04399 asking for numbers, money and percents, values kept up to date, `try`, text blocks and guards; 8d896aa examples as tests, the closest sentences, tables shown in columns, the groceries and budget examples. Tag `english-pass-part-4` (8d896aa). Each commit: make test, make test-gcc and make test-debug passed (702/702/701, 0 leaks for the last).

## Part 5: rough spots, audit, beginner docs, benchmarks

### Done (first commit)

- `a new ...` values in a list or a map without parentheses: the next `new` (or the map's next key) ends the one before.
- In a call sentence, each input is a whole value up to `and` (`report with "bad" and "I expected " followed by word.`): the result isn't used, so arithmetic and `followed by` can only belong to the input. A call inside a value still asks for parentheses.
- `profit by customer` as a name now suggests "customer profits" (it suggested "profit customer").
- A list written out with items of different kinds that fit exactly one interface is a list of that interface; fitting several is an error asking for `an empty list of ...`. (The old note "Both can be used as ..." is gone: it can't happen any more.)
- Natural joiners: a definition can put `in`, `to`, `from`, `by` or `at` between two inputs (`to column of cell in grid:`, `to range from low to high:`), and calls use the same word or `and`.

### Decisions made

- **Joiners are declared by the definition,** never guessed at a call; `and` keeps working. Only words that already end a name (`in`, `to`, `from`, `by`, `at`) can be joiners, so an input's name can't swallow them.
- **`apply`/`applied` reviewed, no change:** `F applied to every X in L` (a value) and `apply F to every X in L` (a sentence) are the same pair as part 4's `the total amount` / `total the amount`; `each X in L ...` is the beginner form for transforming, and the guide leads with it.
- **Mixed lists pick the one interface they all fit,** like a field picks the only kind that has it. If a later interface also fits, the list becomes an error asking which: the program's meaning never changes silently.

### Done (second commit)

- **The phrase audit (item 2):** new Known exceptions in design-principles.md for the words parts 4 and 5 reuse (`when`, `as`, `with commas`, `for example`, `sorted by NAME`, table row names, `what's in`, `try to`, mixed lists), and `and`/`is` marked reviewed. Every example in vocabulary.md's tables (433) was parsed on its own: the 117 that didn't were all fragments needing names, definitions or fields from around them (no drift). The stale "Modules (Phase 2)" line under Later phases was fixed.
- **Examples (item 3):** examples/README.md starts with "Start here" (the plain-English programs: hello, first program, ask name, tip calculator, groceries, budget, gradebook, best months, shipping, coffee shop, order report, taste), then "Every feature"; four examples that weren't listed are; `text.es`'s description no longer mentions joining with `and`; `loops.es`, `tax.es` and `receipt.es` use pieces and brackets instead of `followed by` chains (same output).
- **"Your first programs" (item 4):** a chapter before chapter 1 (unnumbered, so links to the others stay the same): saying, names, asking, choices, repeating, lists, things with parts, new words with `for example`, steps in a row, errors. Grade 2.6 by the audit's measure, no banned words (checked), every program run by `make test`.
- **Benchmarks (item 5):** `make bench` passes; history row "0.1.0 + system tools and the English pass"; README and Latest results updated. Every form is within 20% of its hand-written loop: `sorted by value` was 2.1 times slower (a heap of every item) and is now 1.00 (the items kept and their keys gathered in the pass, then the stable key sort; a first version sorted every item and lost the filter in `the big purchases whose ..., sorted by amount`, which the budget example caught). `benchmarks/run.sh`'s ratio was never worked out (`>` inside awk's `printf` is a redirection), so the 20% check couldn't fail before: fixed. The `sort` and `words` benchmarks still joined text with `and` (removed in part 1) and didn't build: fixed.
- **english-pass.md (item 6):** the pass marked done in "Where it fits", a [Later](english-pass.md#later) list of everything left out, and update-log rows.

## Charts

### Done

- `save a bar chart of X as F`, `save a line chart of X as F`, `save a pie chart of X as F`, `make a bar chart of X called F`, `..., titled T`; X a list of numbers or a map from text or numbers to numbers.
- The charts are drawn by `lib/charts.es`, written in EasyScript (bars, a line with points, a pie with a key; labels turned when they're too wide, the picture taller for them; widths that grow with the number of bars; value labels with commas; text escaped for SVG; the Okabe-Ito palette; sine and cosine worked out by their series, so the language needed no new math words).
- The compiler embeds the module and joins it, as a hidden file, to programs with a chart sentence; messages in a one-file program read exactly as before (no "of main.es").
- `best_months.es` ends with `and then make a bar chart of it called "best months.svg"`, as the plan wrote it.
- Tests: `tests/run/charts` compares the SVG text of each kind (a title with `&` and `<`, negative values, number keys, a single slice, an empty list, a chart made in steps with `what's in it`); `errors/chart_errors`; `ast/charts` and `ast/err_charts`.

### Decisions made

- **Two wordings:** the user's request says `save a bar chart of X as "file.svg"`; the plan's example says `make a bar chart of it called "best months.svg"`. Both are accepted (same meaning), like `make a new file called`.
- **The module joins itself only when needed,** found by its words (`bar chart of`, `line chart of`, `pie chart of`) before parsing, so other programs don't pay for it and their messages don't change.
- **The module calls its own definitions with `with`:** in a program that reads a table, `place of x` would also read as a column called `place` (found by the first example that used both).
- **A title is optional** (`, titled T`); labels come from a map's keys in their order, or 1, 2, 3 for a list.
- **SVG only**, as asked; PNG waits for Phase 4.


## After review: argument order, cleanup, examples

### Done

- **The order arguments are worked out in.** C doesn't fix it, and GCC on Linux x86-64 goes right to left, so `plan/questions` and `cli/build-plan` failed there: in `if any score in scores is 45 and not every score is 45`, the plan note for `every` was written before the one for `any`. `emit_binary` (`and`/`or`) and `cg_emit_example` now work each operand out into a local first, left then right. A search of `src/back/` and `src/front/` for calls whose arguments generate code, add sites or static text, write plan notes or add kinds found the same thing in `es_write_file`, `es_times`, `es_read_file` (sentence and value), `es_text_number`, `es_event_interval`, `es_vfield_*`, a `when` guard, `es_try_two`, fixed maps' keys and values, `es_program_word`, `es_set_status`, and the checker's `filled_in` (map kinds added to `Records` in an order the compiler chose). All fixed the same way. The rest only format messages from values already worked out. Rule added to CLAUDE.md.
- **Cleanup:** the stray `names.txt` at the top of the repo (committed by mistake in part 1) is gone; the roadmap's browser playground is now "After Phase 3 (playground lite comes before v0.2.0 for testing)".
- **"Start here" examples in the most natural form** (same output for every one):
  - `order_report.es` reads its csv file with `read the table in "orders.csv" and call it orders` and works out profit with `an order's profit is its amount minus its cost.`, instead of turning csv rows into records by hand (19 lines down to 14).
  - `unless file "X" exists:` instead of `if not (file "X" exists):` in `budget.es`, `best_months.es` and `order_report.es`. **`if file X doesn't exist` isn't in the grammar**, and the request said not to change the grammar, so `unless` (which is) is used; `doesn't exist` is under "Still not English" below.
  - No parentheses around `new` values in lists (`gradebook.es`, `shipping.es`, `coffee_shop.es`), around `the extra of it`/`the care of it`/`the shot of it` (now `its extra plus its care`, `its shot`), around `(the total cost of its drinks)`, or in `(parcel's country)`. `(bill plus tip) as money` became `bill plus tip, as money` (a comma before `as` applies it to the whole value). The ones left are needed: `(100 minus its discount)`, and the inner lists in `drinks (a list of latte and tea)`.
  - Brackets and commas in text instead of `followed by`: `"Nice to meet you, [name]."`, `"Last time you tipped [last tip]."`, `"([the biggest order's total] dollars)"`, `"  [size]:"`, `"Evening report at [the time now]: ..."`, `"[region]:"`. A bracket can't hold quoted text, so shipping's `(parcel's note, or "no note")` became `let remark be parcel's note, or "no note".` and then `"([remark])"`.
  - Field kinds left out where they're worked out from use: `a student has a name, a class and a score.`, `a drink has a name, a size and an extra shot.`, `a customer has a name and an email.`. shipping keeps its kinds: `a note (text or nothing)` can't be worked out from `note nothing`.
  - Small wording: `if last tip isn't empty` (was `is not ""`), `let tip be bill times tip rate, rounded to 2 decimal places.` (the comma rounds the whole value; was parentheses), `show what's in file "best months.csv".` (was `say the contents of file`), `say the number of parcels per country.` (was an empty map and a loop adding 1 with `starting at 0`), `the number of sold drinks per size` (was `the number of sold per size`). The notes at the top of `budget.es` and `order_report.es` were brought up to date.
  - Not changed: `hello.es`, `first_program.es` and `taste.es` (the README's copies must match them), `groceries.es`.
- **Checked:** `make test` 712, `make test-gcc` 712, `make test-debug` 711 with 0 of 802 processes leaking (macOS); and `make CC=gcc test` on Linux x86-64 with GCC 16.2 in Docker: 712 passed. There, the commit before the fix fails `plan/questions`, and the fix passes it.

## Still not English

Every line of the "Start here" examples, read aloud as someone who has never written a program would hear it. Each line below is one they'd stumble on. Nothing here is built; each needs a proposal ([proposals/](proposals/README.md)).

| Example | Line | Why it trips | Suggested wording |
|---|---|---|---|
| groceries | `write "milk" in file it` | "file it" isn't English; "it" already is the file | `write "milk" in it` |
| groceries | `add "eggs" and "bread" to the end of file it, one per line` | Same "file it"; "the end of" is how the computer does it, not what the person wants | `add "eggs" and "bread" to it, one per line` |
| budget | `the first item of the keys of totals per store` | Three layers of "of"; "keys" is a programmer word; the person wants the top store | `the store with the highest total amount` (open question 5) |
| budget, best_months, order_report | `unless file "spending.csv" exists:` | "unless" reads well, but people say "if it doesn't exist"; the request asked for exactly that | `if file "spending.csv" doesn't exist:` |
| budget, best_months, order_report | `put "date,store,category,amount" in file ...` then one `add "2026-09-01,Landlord,rent,1200" to the end of file ...` per row | Comma-separated rows typed as text, one sentence per row | `write the text below in file "spending.csv":` with the rows indented under it (text blocks exist, but only as values after `the text below:`, not after `write ... in file`) |
| shipping | `(the extra of it) plus (the care of it)` (now `its base price plus its extra plus its care`) | The old form needed parentheses and "of it"; the new one reads well, but "extra" and "care" are made-up names for "the charge for going abroad" | `a parcel's price is its base price, plus 12 if it's abroad, plus 4 if it's fragile.` |
| shipping | `a fragile (yes or no)` and `depends on its fragile: 4 when it's yes.` | "a fragile" and "its fragile" aren't English; "when it's yes" is how a computer says it | `a parcel can be fragile.` and `4 if it's fragile.` |
| shipping | `"([remark])"` | Brackets inside parentheses inside quotes; and a quote can't go inside the brackets, so a name had to be made up | `say ..., "(" then parcel's note or "no note", ")"`-style pieces, or quotes allowed inside brackets |
| gradebook, shipping, coffee_shop | `let students be a list of a new student with name "Ana", class "Math", score 72, a new student ...` | "a list of a new student" isn't English; one comma both separates fields and separates students | `the students are:` with one `"Ana" in "Math" scored 72`-style line each, or a table in the text below |
| gradebook | `the student with the highest score's name` | The `'s` belongs to the student, but it reads as "the highest score's name"; `the name of the student with the highest score` is an error today | `the name of the student with the highest score` |
| gradebook | `how many students whose score is at least 60` | "how many students whose" is missing a verb | `how many students have a score of at least 60` |
| gradebook | `say "  ", student's name, student's score.` | The `"  "` is there only to indent the line | `say student's name, student's score, indented.` or a list shown as a table |
| gradebook, coffee_shop, order_report | `for each class and average in ...` / `for each size and count in ...` / `for each region and total in ...` | Naming two things at once is a programmer habit | `for each class, with its average, in ...` or `show ... as a table` |
| coffee_shop | `the total total of orders` | The field is called "total", so the question doubles it | Rename the field (`an order's bill`), or accept `the sum of the totals` |
| coffee_shop | `the total cost of its drinks times (100 minus its discount) divided by 100` | Percent off written as arithmetic | `the total cost of its drinks, minus its discount percent` (`N percent of X` exists; `less N percent` doesn't) |
| coffee_shop | `the 1 most common names` | "the 1 most common names" isn't English; there's no singular | `the most common name` |
| coffee_shop | `say "A large drink costs", the price of mocha.` | Fine aloud, but `mocha` is a name made with `let`, so "the price of mocha" quietly means that one drink | (no change needed; noted) |
| tip_calculator | `say "Total:", bill plus tip, as money.` (was `(bill plus tip) as money`) | Reads well aloud, but whether `as money` covers the whole sum depends on one comma; without it, it means `tip as money` and is an error | Keep, and teach the comma in the guide |
| tip_calculator | `if good service, set tip rate to 0.2.` | "if good service" is missing its verb | `if the service was good, ...` (the answer named by the question) |
| tip_calculator | `ask for a number "How much was the bill? " and call it bill.` | The space before the closing quote matters, and only a programmer would know | Add the space after a question mark automatically |
| ask_name | `ask "What's your name? " and call the answer name` | Same trailing space | Same |
| order_report | `let best be the first item of the keys of profits.` | Same as budget's top store | `the customer with the highest total profit` |
| order_report | `"([the value for best in profits] profit)"` | "the value for best in profits" is how a map is read, not how a person asks | `best's total profit` or `the total profit of best's orders` |
| first_program | `let name be "Ada".` | "let ... be" is how maths talks | `the name is "Ada".` (it clashes with comparisons; needs a proposal) |
| taste | `to greet using name:` | "greet using name" isn't English | `to greet someone called name:` or `to greet a person:` |
| taste | `count down from 3 to 1: say it.` | "it" for the number counted only works once you know it | `say the number.` |

## Reads like text

On the `reads-like-text` branch (from master after the English pass merge), 2026-10-12. The rule and every decision are in [english-pass.md](english-pass.md#decided-2026-10-12-reads-like-text); each new phrase has a file in [proposals/](proposals/README.md). Each feature is one commit, after `make test`, `make test-gcc`, `make test-debug` (with the leak sweep) and `make CC=gcc test` on Linux x86-64 (GCC 16 in Docker) all passed.

| # | Feature | Result | Tests (test / gcc / debug / Linux gcc) |
|---|---|---|---|
| 1 | Inputs in the middle of a definition's first line (`to give a customer a discount of some percent:`, `give every loyal customer a discount of 10 percent.`, `introduce ann to bob.`), `increase/reduce/decrease X by N percent`, `reduce X by N` | Built. New: `run/pattern_actions`, `errors/pattern_errors`, `ast/patterns`, `ast/err_patterns`, `examples/programs/discounts`. Breaking: `reduce` starts sentences; `to adopt a dog:` now has an input | 717 / 717 / 716, 0 leaks / 717 |
| 2 | Yes/no fields as describing words (`..., and can be fragile.`, `a parcel can be urgent.`, `a new fragile parcel`, `mark box as [not] fragile`, `box is fragile`, `the fragile parcels`), `it's abroad` in definitions, one-line choices (`4 if it's fragile, otherwise 0`, `, plus 12 if it's abroad`) | Built. New: `run/describing_fields`, `errors/describing_errors`, `ast/describing`, `ast/err_describing`; `examples/programs/shipping.es` uses them (same output). Breaking: `mark` starts sentences; a yes/no field's name after `is` reads the field. Only fields written as yes or no (or `can be`) are describing words: the parser can't know a kind worked out from use | 721 / 721 / 720, 0 leaks / 721 |
