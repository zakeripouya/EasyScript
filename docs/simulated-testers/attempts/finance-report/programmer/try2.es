read the table in "bank.csv" and call it rows
let rows be rows where amount is not nothing

for each row in rows:
    set row.amount to number(replace(row.amount, "$", ""))

say "Spending by category:"
total the amount per category

say "Spending by month:"
total the amount per month

let biggest be max(rows, amount)
say "Biggest expense: " + biggest.description + " on " + biggest.date + " ($" + (biggest.amount rounded to 2 decimal places) + ")"

make a pie chart of rows called "categories.svg"
