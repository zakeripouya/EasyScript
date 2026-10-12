note: The sales report from docs/english-pass.md, written with questions about lists
note: (part 2 of the English pass). The first run writes an example orders.csv to read.
if not (file "orders.csv" exists):
    put "customer,region,amount,cost" in file "orders.csv"
    add "Ann,East,1200,700" to the end of file "orders.csv"
    add "Bo,West,300,250" to the end of file "orders.csv"
    add "Ann,East,800,500" to the end of file "orders.csv"
    add "Cy,North,2500,1900" to the end of file "orders.csv"
    add "Bo,West,150,90" to the end of file "orders.csv"

an order has a customer (text), a region (text), an amount (a number) and a profit (a number).

read csv file "orders.csv" and call it rows.
let orders be an empty list of orders.
for each row in rows:
    let amount be (the value for "amount" in row) as a number.
    let profit be amount minus (the value for "cost" in row) as a number.
    add (a new order with customer (the value for "customer" in row), region (the value for "region" in row), amount amount and profit profit) to the end of orders.

say "Big orders:", how many orders whose amount is over 1000.

let profits be the total profit of orders per customer, sorted biggest first.
let best be the first item of the keys of profits.
say "Best customer:", best, "(" followed by the value for best in profits, "profit)".

for each region and total in the total amount of orders per region:
    say region followed by ":", total.

if every order's profit is above 0, say "Every order made money.".
