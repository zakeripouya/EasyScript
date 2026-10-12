note: The sales report from docs/english-pass.md, written with questions about lists
note: (part 2 of the English pass) and a table read from a csv file. The first run
note: writes an example orders.csv to read.
unless file "orders.csv" exists:
    put "customer,region,amount,cost" in file "orders.csv"
    add "Ann,East,1200,700" to the end of file "orders.csv"
    add "Bo,West,300,250" to the end of file "orders.csv"
    add "Ann,East,800,500" to the end of file "orders.csv"
    add "Cy,North,2500,1900" to the end of file "orders.csv"
    add "Bo,West,150,90" to the end of file "orders.csv"

read the table in "orders.csv" and call it orders.
an order's profit is its amount minus its cost.

say "Big orders:", how many orders whose amount is over 1000.

let profits be the total profit of orders per customer, sorted biggest first.
let best be the first item of the keys of profits.
say "Best customer:", best, "([the value for best in profits] profit)".

for each region and total in the total amount of orders per region:
    say "[region]:", total.

if every order's profit is above 0, say "Every order made money.".
