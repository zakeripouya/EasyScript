let orders be read_csv "orders.csv"

for each order in orders:
    say order

let total be 0
let count be 0
for each order in orders:
    set total to total + order price
    set count to count + 1

let avg be total / count
say "Average order price: $" + avg
