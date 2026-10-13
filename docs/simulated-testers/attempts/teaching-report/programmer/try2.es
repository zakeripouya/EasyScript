let data be read_csv("grades.csv")

let students be unique(data, "student")
let averages be list()
let best be ""
let best_avg be 0

for each student in students:
    let rows be filter(data, "student", student)
    let avg be mean(column(rows, "score"))
    append(averages, avg)
    say student + ": " + avg
    if avg > best_avg:
        set best_avg to avg
        set best to student

say "Best average: " + best

let below_50 be 0
for each score in column(data, "score"):
    if score < 50:
        set below_50 to below_50 + 1
say "Results below 50: " + below_50

let chart be bar_chart(students, averages)
save chart as svg file "averages.svg"
