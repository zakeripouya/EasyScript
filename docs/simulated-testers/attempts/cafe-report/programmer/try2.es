let orders be read_csv("orders.csv")

for each day in unique(column(orders, "day")):
    let total be 0
    for each order in orders:
        if get(order, "day") == day:
            set total to total + get(order, "price")
    say day + ": $" + round(total, 2)

let best_item be ""
let best_count be 0
for each item in unique(column(orders, "item")):
    let n be count(column(orders, "item"), item)
    if n > best_count:
        set best_count to n
        set best_item to item
say "Most popular item: " + best_item

let prices be column(orders, "price")
let avg be sum(prices) / len(prices)
say "Average order price: $" + round(avg, 2)

let busiest be ""
let most be 0
for each day in unique(column(orders, "day")):
    let n be count(column(orders, "day"), day)
    if n > most:
        set most to n
        set busiest to day
say "Busiest day: " + busiest
