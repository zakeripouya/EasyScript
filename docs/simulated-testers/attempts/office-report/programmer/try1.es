data = read csv "timesheet.csv"

totals = {}
for row in data:
    totals[row.employee] = totals[row.employee] + row.hours

for name, hours in totals:
    print(name + ": " + hours)

print("Overtime:")
for name, hours in totals:
    if hours > 40:
        print(name + ": " + (hours - 40))

team_total = sum(data.hours)
print("Team total: " + team_total)
