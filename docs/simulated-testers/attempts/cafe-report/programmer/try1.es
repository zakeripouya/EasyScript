orders = read_csv("orders.csv")

totals = {}
for order in orders:
    totals[order.day] = totals.get(order.day, 0) + order.price

for day, total in totals:
    print(day + ": $" + format(total, 2))

item_counts = count(orders, by="item")
print("Most popular item: " + max(item_counts).key)

avg = sum(orders.price) / len(orders)
print("Average order price: $" + format(avg, 2))

day_counts = count(orders, by="day")
print("Busiest day: " + max(day_counts).key)
