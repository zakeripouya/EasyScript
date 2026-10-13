read file "spending.csv" and call it rows

let budget_groceries be 400
let budget_eating_out be 150
let budget_transport be 120
let budget_fun be 100

total the amount per month in rows

for each row in rows:
    let limit be 0
    if row.category == "groceries":
        set limit to budget_groceries
    if row.category == "eating out":
        set limit to budget_eating_out
    if row.category == "transport":
        set limit to budget_transport
    if row.category == "fun":
        set limit to budget_fun
    print(row.month + " " + row.category + ": $" + row.amount)
    if row.amount > limit:
        print("  OVER BUDGET: " + row.category + " by $" + (row.amount - limit))

make a bar chart of rows called "months.svg"
