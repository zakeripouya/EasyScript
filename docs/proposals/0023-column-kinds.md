# 0023: a table's column kinds, written down

**Status:** Built
**Kind:** Addition. `read the table in F, with ...` was an error before.

## The phrase

```
read the table in "bank.csv", with amount in dollars.
read the table in "bank.csv", with amount in dollars and quantity as a number, and call it purchases.
read the table in "bank.csv" and call it purchases, with amount in euros.
read the table in "fares.csv", with fare in pounds, then total the fare per city, then say it.
```

After the file's name (or after `and call it NAME`): `, with COLUMN KIND`, where KIND is `in dollars`, `in euros`, `in pounds` or `as a number`; more columns are joined with `and` (or commas). COLUMN is a column's name as the program uses it (several words are fine: `weight kg`).

What it means in core-language terms: the row kind's field COLUMN has that kind, as if it were written `(dollars)` in a `has` sentence. The column is a field even if nothing else uses it, so reading the file checks that it's there.

## Why

Decided in the shell-and-money review: since [proposal 0014](0014-file-decides-column-kind.md), a column used only as "a number or an amount" takes its kind from the file the compiler sees. A program then depends on what the file held when it was checked. Writing the kind down removes that: **a stated kind wins over what the file looks like**. When the file and the stated kind disagree, it's the existing error naming the column, the line and both kinds ("In bank.csv, line 2, "amount" is "$1200", an amount in dollars, but the program uses it as a number."), before the program runs when the file can be seen, and when it's read otherwise.

Plain numbers in a column stated (or judged) to be amounts are that currency (`1200` among `$` amounts is `$1,200.00`), when the program is checked and when the file is read.

## Checked against the design rules

- One meaning per word: `with` introduces what a table is read with, as `with` introduces inputs and fields; `in dollars` and `as a number` are the existing kind words (`an amount in dollars`, `as a number`).
- Distinct trigger words: only after `read the table in F`.
- Deterministic: the stated kind, or the existing rule.
- Errors: a kind EasyScript doesn't know ("I expected "in dollars", "in euros", "in pounds" or "as a number" here, ..."), a column stated twice with two kinds (""amount" is said to be a number here, but it's used as an amount in dollars."), and the file not fitting (proposal 0014's error).

## Cognitive Dimensions notes

- Hidden dependencies: removes the dependency on the file seen when checking.
- Closeness of mapping: "with amount in dollars" is how a person would describe the column.

## Open questions

- `with date as a date`, `with code as text` (a column of digits kept as text, like zip codes): not asked for; easy to add with the same shape.
