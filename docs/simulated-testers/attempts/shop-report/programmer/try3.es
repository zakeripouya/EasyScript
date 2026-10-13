read the table in "sales.csv" and call it sales

let total be 0

for each row in sales:
    let price be (price of row) as a number
    let revenue be (quantity of row) * price
    set revenue of row to revenue
    set total to total + revenue

say "Total: $" + total

total the quantity per product and call it units
sort units by quantity from highest to lowest
say "Best seller: " + first item of units

total the revenue per month and call it monthly
say monthly

make a bar chart of monthly called "months.svg"
