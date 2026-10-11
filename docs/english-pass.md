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

## Then

1. The first non-coder test: 3 to 5 people who have never written code, against the success test (a working quiz in 30 minutes, with no help).
2. Templates to start from.
3. Freeze the core grammar at **v0.2.0**: after that, only additions.
