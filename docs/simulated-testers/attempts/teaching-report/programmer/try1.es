data = read_csv("grades.csv")

averages = {}
for student in unique(data.student):
    rows = data.filter(row => row.student == student)
    averages[student] = mean(rows.score)
    print(student + ": " + averages[student])

best = max(averages, key=averages.get)
print("Best average: " + best)

below_50 = count(data.filter(row => row.score < 50))
print("Results below 50: " + below_50)

chart = bar_chart(averages, title="Average score per student")
chart.save("averages.svg")
