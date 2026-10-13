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
