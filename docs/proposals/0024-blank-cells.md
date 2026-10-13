# 0024: a blank cell is nothing

**Status:** Built
**Kind:** Change (breaking): a blank cell in a table's column of numbers or amounts used to be 0.

## The rule

Decided in the shell-and-money review. In a table read with `read the table in F`, a blank cell (empty, or only spaces) in a column of numbers or amounts is **nothing**:

- **The total, the average, the median, the top N, the highest/lowest, `per` groups and charts skip blank values**, as a spreadsheet's SUM, AVERAGE and MEDIAN do. (A group whose cells are all blank has no entry.) `sorted by` keeps the row and puts it last.
- **`how many` counts rows**, blank or not.
- **Showing a row or a table prints the blank cell as blank** (`amount ""` in a row, an empty cell in `as a table`), and **saving the table writes an empty cell**.
- **Using a blank value directly** (arithmetic, a comparison, a change like `add 1 to its count`) **stops the program**: `"amount" is blank here: its cell in the table is empty.` with `To use $0 when it's blank, write ", or $0" after it, like "its amount, or $0".`
- `its amount, or $0` gives $0 for a blank cell, and `if its amount is nothing` / `is not nothing` test for one.

## How

The checker doesn't know which cells will be blank, and the program shouldn't depend on the file it was checked with (see [0023](0023-column-kinds.md)), so the column keeps its kind (a number, or an amount) and the cell holds a mark for blank: a "not a number" with its own bits for numbers, the smallest 64-bit number of cents for amounts (amounts stay below 2^53, so it can't be a real one). Reading a cell as a value checks for the mark first; the questions about a list read it without the check and skip it; `, or D` and `is nothing` read it and test for it. Nothing the program works out can make the mark.

What it means in core-language terms: for each row, `if its amount is not nothing:` around the step that uses it; the total is the hand-written loop with that check.

## Checked against the design rules

- Deterministic: a fixed rule, the same for every program and file.
- Statements vs values, "not found" results: blank is `nothing`, the one word for "there isn't one"; `, or D` is the existing default.
- Errors: the runtime error names the column and gives the `, or` fix, in the currency of the column.

## Cognitive Dimensions notes

- Closeness of mapping: a spreadsheet user expects an empty cell to be left out of an average, not counted as 0.
- Error-proneness: a blank used in arithmetic is an error the first time it happens, never a silent 0.

## Open questions

- Blank cells in a column of dates or yes/no are still an error when the file is read ("which isn't a date like 2026-10-31").
- Using a blank in a comparison inside a filter (`the purchases whose amount is over 100`) stops the program; a spreadsheet's COUNTIF would skip it. Suggested: treat a blank as failing any comparison in a filter, if user testing shows people hit it.
