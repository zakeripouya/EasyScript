# 0014: the file decides a column's kind

**Status:** Built
**Kind:** Change (to `read the table in F`): a column the program leaves as "a number unless something says otherwise" takes its kind from the file. Programs that worked before keep their meaning: the only programs that change are ones that stopped on a cell like `$1200`, or a blank cell, while the program ran.

## The phrase

No new words. `read the table in F [and call it NAME]`, as before:

```
read the table in "bank.csv" and call it purchases.
say the total amount of purchases.
```

A table's columns are fields whose kinds come from how the program uses them. Some uses only say "a number or an amount" (`the total amount of`, `the average`, arithmetic, a chart); until now, such a column became a number when nothing else decided, and reading `$1200` stopped the program. Now the file decides it:

- **Before the program runs** (the compiler), when the file's name is written out in quotes: it looks at the file in the folder the program runs in, or, if there's no such file yet, at the text the program itself writes into a file of that name (`put the text below in file "bank.csv":`, the first one). For each such column, ignoring blank cells:
  - every value is an amount in one currency (`$1200`, `-$5`, `€4.50`, `1,200 dollars`), or a plain number (read as that currency, as the program would read it): the column is amounts in that currency;
  - every value is a number (`96.40`, `-3`, `1e5`): it's a number;
  - amounts in two currencies: an error showing a line of each;
  - anything else (text): it's left alone, so it's a number as before, and the next check reports it.
- **Use and file disagree:** when the program uses a column as a number or an amount and the file holds something else (amounts where it uses a number, text where it uses a number, euros where it uses dollars), it's an error before the program runs, naming the column, the line of the file and both kinds. Plain numbers in a column of amounts are fine (`1200` is `$1,200.00`).
- **Two reads of the same file** (the same name in quotes) use each column the same way: their kinds are one kind, so the second read in a chain is money when the first one's use or the file says so.
- **When the file is read** (while the program runs), the kinds are already fixed. Each cell is read as its column's kind; a blank cell in a column of numbers or amounts was 0 (`$0.00`; now nothing, [proposal 0024](0024-blank-cells.md)); a cell that doesn't fit stops the program with a message naming both kinds (`In bank.csv, line 2, "amount" is "$1200", an amount in dollars, but the program uses it as a number.`). That happens when the compiler couldn't look (the name isn't written out, the file wasn't there and the program doesn't write it, or the file changed after `easyscript build`).

What it means in core-language terms: nothing new; the column's kind is written down as if the program had said `(dollars)` for it.

## Examples

```
before: read the table in "bank.csv" and call it purchases.   (in the shell)
        say the total amount of purchases.
        Line 1: In bank.csv, line 2, "amount" is "$1200", which isn't a number.
after:  $1,858.15
```

```
read the table in "bank.csv" and call it purchases.
say the total amount of purchases.
read the table in "bank.csv", then total the amount per month,
then make a bar chart of it called "spending.svg".
```

The second read used to fail on `$1200`; now both reads' `amount` are the file's dollars.

## Checked against the design rules

- One meaning per word: no new words.
- Common words only, readable aloud: no new words.
- Distinct trigger words: no new pattern.
- Statements vs values, `of`/`in`, optional and "not found" results: unchanged. Blank cells in a column of numbers or amounts are 0, the way a spreadsheet adds them up (an "or nothing" amount is still an open question).
- Exact meaning in the core: the column gets the kind a written-down `(dollars)` would give it.
- Deterministic: the rule is fixed and looks at every non-blank cell; it never picks between two readings (two currencies are an error).
- Errors: "In trip.csv, "cost" has amounts in two currencies: line 2 is "$300" and line 4 is "€45"." with "Write every amount in it in one currency."; "In bank.csv, line 2, "amount" is "$1200", an amount in dollars, but the program uses it as a number." Both grade 8 or below, no programmer words.

## Cognitive Dimensions notes

- Closeness of mapping: a person reading a bank export sees dollars; now so does the program.
- Hidden dependencies: the kind depends on a file the program doesn't show. The error names the file, the line and both kinds, and the shell shows it right away.
- Error-proneness: a file that changes after `easyscript build` is caught when it's read.
- Consistency: the same rule as kinds from use, with the file as one more place a kind can come from.

## Open questions

- A column the program only prints, compares or sorts stays text, as before: judging it from the file would change what existing programs print (`"96.40"` would print `96.4`) and how they sort. Suggested: judge those too, at the next point where changing meanings is allowed (before the freeze), since sorting `1200, 249, 96.40` as text is rarely what anyone wants.
- Blank cells as nothing: decided and built in the v0.2 prep ([proposal 0024](0024-blank-cells.md)). Writing a column's kind down: [proposal 0023](0023-column-kinds.md).
