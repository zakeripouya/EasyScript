# Ideas

Possible features that aren't planned yet. Nothing here works today, and the wording is only a sketch. When an idea is picked up it moves to the [roadmap](roadmap.md), and it must follow the [design principles](design-principles.md).

## A deep copy

`a copy of X` copies the lists and maps inside X but shares the records inside it (see [design principles](design-principles.md#8-sharing-and-copying)). A second phrase could copy everything, records included:

```
let backup be a full copy of orders
```

Open questions: the exact words (`a full copy of`, `a deep copy of`), and what to do with records that point to each other in a circle (copy the circle once, keeping its shape).

## Reading a csv file one row at a time

`read csv file F and call it rows` reads the whole file into a list of maps, so a big file takes memory for every row at once (the `sales` benchmark: 58 MB for 100,000 rows, against 12 MB for Python's `csv` module, which reads a row at a time). A loop could read one row, run its block, and free the row before reading the next:

```
for each row in csv file "sales.csv":
    add (the value for "amount" in row) as a number to total
```

Each row would be a map from text to text, like an item of `read csv file`. Memory would stay flat however big the file is. Open questions: whether `stop the loop` should close the file at once (it should), what the error says when a row has more fields than the header, and whether the same form should exist for plain lines (`for each line in file F:`).
