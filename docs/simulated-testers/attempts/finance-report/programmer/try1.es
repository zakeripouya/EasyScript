rows = read_csv("bank.csv")
rows = rows.filter(row => row.amount != "")

for row in rows:
    row.amount = number(row.amount.replace("$", ""))
    row.month = row.date[0:7]

by_category = {}
by_month = {}
for row in rows:
    by_category[row.category] = by_category.get(row.category, 0) + row.amount
    by_month[row.month] = by_month.get(row.month, 0) + row.amount

print("Spending by category:")
for category, total in by_category:
    print(category + ": " + format(total, 2))

print("Spending by month:")
for month, total in by_month:
    print(month + ": " + format(total, 2))

biggest = max(rows, key=row => row.amount)
print("Biggest expense: " + biggest.description + " on " + biggest.date + " ($" + format(biggest.amount, 2) + ")")

chart = pie_chart(labels=keys(by_category), values=values(by_category), title="Spending per category")
chart.save("categories.svg")
