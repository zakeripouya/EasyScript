# The English pass

**Status: Coming soon.** This is the plan for the step after step 22 (see the [roadmap](roadmap.md)). Every phrase here is a **wording proposal**, not a decision: none of it compiles yet, and the words may change. The pass follows the [rules for new phrases](design-principles.md#rules-for-new-phrases) and adds the audits for them.

## Questions and totals

- `how many X are Y` (`how many scores are above 50`)
- `if any X is Y`, `if all X are Y`, `if no X is Y`
- `the average of`, `the total of`, `the median of`
- `the top N` (`the top 3 scores`)
- `the one with the highest ...` (`the player with the highest score`)
- grouping with `per` and `by` (`the total sales per region`)
- `half of`, `twice`, `N percent of`, `rounded to N decimal places`

## Choices without `if`

- defaults: `, or X` and `starting at` wherever a value can be missing
- adjectives: `a customer is loyal if ...`, then `the loyal customers`
- value definitions: `a customer's profit is ...`
- acting on groups: `remove the tasks that are done`
- `depends on` tables, with every case covered (`the shipping depends on the country:`)
- `must` rules (`a price must be at least 0`)
- events: `when ...` / `whenever ...`, and `every N seconds`
- `unless`

## Plain words

- text templates: `"Hello, [name]!"`
- `a dog has a name and an age`, as well as `a dog is a kind of thing with:`
- `ask for a number`, `ask yes or no`
- `as money`, `as a percent`, `with commas`
- `try to ... if that fails ...`
- units of measure (`5 km`, `3 hours`)
- `the name of rex` alongside `rex's name`
- `whose` and `where` (`the dogs whose age is above 3`)
- chains: `take ..., then ...`, with `them` and `the ones`
- examples as tests: `for example, the price of a large coffee is 5.`

## Rough spots to fix

- changing a value in a map without writing the lookup twice
- the parentheses needed before `'s` and around `new` values in lists
- `profit by customer` rejected as a name
- one family of phrases for filtering and mapping (`where`, `that pass`, `only`, `every`, `applied to every`)
- list literals of mixed kinds when an interface they share is declared
- review the naming of `apply` and `applied to every`

Found by writing the JSON parser (`examples/programs/json/`) and Snake (`examples/programs/snake/`) in step 22:

- **No early exit for errors:** every parsing function has to check `if r's problem is not "", give back ...` after each call; there's no way to stop a chain of calls with a problem except `stop the program`.
- **No raw text:** JSON written inside a program needs `\"` for every quote (the tests read theirs from `the text sent to the program` instead), and there's no `\r` escape (`the character with code 13`) or `\u` escape.
- **`as a number` doesn't read exponents** (`1e3`), so the parser works them out with `to the power of`; and `as text` keeps only 10 significant digits, so numbers don't survive writing and reading back exactly.
- **No small values with parts that compare by value:** a position `(x, y)` is a record (never equal to another record, so `contains` can't find it) or a kind with one variant (which still has to be checked before each field is read). Snake keeps cells as single numbers.
- **No way to ask whether a number is infinity** (the parser uses `amount minus amount is not 0`), and no whole-number division (`(n divided by 16) rounded down`).
- **A function's inputs are only joined by `and`,** so `the column of cell in g` has to be written `column of cell and g`.
- **Arguments followed by `followed by` need parentheses:** `report in r and ("I expected " followed by word)`.
- **`new` values inside a map literal need parentheses,** as in lists.
- **A file's helpers are visible to the files that use it** unless each is marked `privately` (18 of them in `json.es`), and a helper named like a variable in the using file is an error there.
- **`count from 1 to 0` counts down,** so loops over "the first N - 1 items" need a `while` when N can be 1.

## Then

1. The first non-coder test: 3 to 5 people who have never written code, against the success test (a working quiz in 30 minutes, with no help).
2. Templates to start from.
3. Freeze the core grammar at **v0.2.0**: after that, only additions.
