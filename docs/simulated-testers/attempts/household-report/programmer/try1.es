budget = {"groceries": 400, "eating out": 150, "transport": 120, "fun": 100}

rows = read_csv("spending.csv")

totals = {}
for row in rows:
    month = row.date[0:7]
    amount = number(row.amount.replace("$", ""))
    if month not in totals:
        totals[month] = {}
    totals[month][row.category] = totals[month].get(row.category, 0) + amount

month_totals = {}
for month, cats in totals:
    print("Month: " + month)
    month_totals[month] = 0
    for category, spent in cats:
        print("  " + category + ": $" + spent)
        month_totals[month] += spent
        if spent > budget[category]:
            print("  OVER BUDGET: " + category + " by $" + (spent - budget[category]))

chart = bar_chart(month_totals, title="Total spent per month")
chart.save("months.svg")
