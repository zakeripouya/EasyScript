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

### Commits

(filled in below as they land)
