note: The sales report from docs/english-pass.md, written with questions about lists
note: (part 2 of the English pass) and a table read from a csv file. The first run
note: writes an example orders.csv to read.
if file "orders.csv" doesn't exist:
    put the text below in file "orders.csv":
        customer,region,amount,cost
        Ann,East,1200,700
        Bo,West,300,250
        Ann,East,800,500
        Cy,North,2500,1900
        Bo,West,150,90

read the table in "orders.csv" and call it orders.
an order's profit is its amount minus its cost.

say "Big orders:", how many orders have an amount of more than 1000.

let best be the customer with the highest total profit.
say "Best customer: [best] ([the total profit of the orders whose customer is best] profit)".

for each region and total in the total amount of orders per region:
    say "[region]:", total.

if every order's profit is above 0, say "Every order made money.".
