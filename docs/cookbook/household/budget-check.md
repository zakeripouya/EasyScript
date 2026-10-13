# Check the budget at the start of the month

Ask for the income and each regular bill, show what's left, warn when it's under $500, and work out how many months it takes to save $5,000.

## The program

`budget-check.es`:

```
ask "Income this month? " and call it income.
let bills be $0.
for each bill in a list of "rent", "utilities", "insurance" and "phone":
    ask "How much is the [bill]? " and call it cost.
    add cost to bills.
let left be income minus bills.
say "Left after bills:", left.
if left is below $500, say "Careful: less than $500 left."
ask for an amount "How much do you want to save each month? " and call it saving.
say "Months to save $5,000:", $5000 divided by saving, rounded up.
```

## Running it

When it's run with `easyscript run budget-check.es` and these answers are typed (`$4200`, `$1650`, `$180`, `$95`, `$60`, `$400`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
Income this month? How much is the rent? How much is the utilities? How much is the insurance? How much is the phone? Left after bills: $2,215.00
How much do you want to save each month? Months to save $5,000: 13
```

## Under the hood

`, rounded up` after the division rounds the whole value up (12.5 months is 13), because of the comma. Without the comma, EasyScript would ask whether you meant to round only the last number.
