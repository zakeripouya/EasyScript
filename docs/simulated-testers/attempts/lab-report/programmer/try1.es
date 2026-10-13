data = read_csv("readings.csv")

groups = data.group_by("group")
for name, rows in groups:
    temps = rows.map(r => r.temperature)
    avg = sum(temps) / len(temps)
    biggest_drop = 0
    for i in range(1, len(temps)):
        drop = temps[i-1] - temps[i]
        if drop > biggest_drop:
            biggest_drop = drop
    print(f"Group {name}: average = {avg}, biggest drop = {biggest_drop}")

for row in data:
    f = row.temperature * 9 / 5 + 32
    print(f"{row.group} minute {row.minute}: {f} F")

group_a = data.filter(r => r.group == "A")
chart = line_chart(x=group_a.minute, y=group_a.temperature)
chart.save("cooling.svg")
