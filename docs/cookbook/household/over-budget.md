# A month's spending against the budget

Compare September's spending in each category with the monthly budget, say what's over and what's left, and save a chart of the spending.

## The program

`over-budget.es`:

```
read the table in "spending.csv", with amount in dollars, and call it spending.
let budget be a map of "groceries" to $400, "eating out" to $150, "transport" to $120 and "fun" to $100.
for each category and its total in the total amount of spending per category:
    let limit be the value for category in budget.
    if total is over limit:
        say category, "is over budget by", total minus limit.
    otherwise:
        say category, "is within budget, with", limit minus total, "left."
save a bar chart of the total amount of spending per category as "september.svg", titled "September".
```

## The data

`spending.csv`, in the same folder:

```
date,category,amount
2026-09-03,groceries,$210.75
2026-09-07,eating out,$32.40
2026-09-10,groceries,$205.10
2026-09-14,transport,$130.00
2026-09-19,eating out,$48.60
2026-09-25,fun,$120.00
2026-09-28,groceries,$61.35
```

## Running it

`easyscript run over-budget.es` shows:

```
groceries is over budget by $77.20
eating out is within budget, with $69.00 left.
transport is over budget by $10.00
fun is over budget by $20.00
```

It also makes `september.svg`.
