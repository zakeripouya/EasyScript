sales = read csv "sales.csv"

total = 0
units = {}
monthly = {}

for row in sales:
    price = number(row.price.replace("$", ""))
    revenue = row.quantity * price
    total = total + revenue
    units[row.product] = units.get(row.product, 0) + row.quantity
    month = row.date[0:7]
    monthly[month] = monthly.get(month, 0) + revenue

print("Total: $" + total)
best = max(units, key=units.get)
print("Best seller: " + best + " (" + units[best] + " units)")

for month in monthly:
    print(month + ": $" + monthly[month])

chart = bar_chart(monthly, title="Revenue per month")
chart.save("months.svg")
