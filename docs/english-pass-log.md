# English pass: work log

A running record of the unattended English-pass run (parts 2 to 5, then charts), on the `english-pass` branch. Part 1 was finished before this run (see the update log in [english-pass.md](english-pass.md)). The plan and decisions live in [english-pass.md](english-pass.md); this file records what was done, when, and what's left for review.

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
