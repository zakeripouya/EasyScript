read the table in "bank.csv" and call it rows
let rows be rows where amount is not nothing

say "Spending by category:"
total the amount per category

say "Spending by month:"
total the amount per month

let biggest be the largest amount in rows
say "Biggest expense: " followed by biggest

make a pie chart of rows called "categories.svg"
