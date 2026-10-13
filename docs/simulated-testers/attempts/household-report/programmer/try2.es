let rows be read_csv("spending.csv")

for each row in rows:
    set row.amount to number(replace(row.amount, "$", ""))
    set row.month to slice(row.date, 0, 7)

let budget_groceries be 400
let budget_eating_out be 150
let budget_transport be 120
let budget_fun be 100

total the amount per month

for each month in months(rows):
    print("Month: " + month)
    for each category in categories(rows):
        let spent be sum_where(rows, month, category)
        print("  " + category + ": $" + spent)
        let limit be 0
        if category == "groceries":
            set limit to budget_groceries
        if category == "eating out":
            set limit to budget_eating_out
        if category == "transport":
            set limit to budget_transport
        if category == "fun":
            set limit to budget_fun
        if spent > limit:
            print("  OVER BUDGET: " + category + " by $" + (spent - limit))

make a bar chart of rows called "months.svg"
