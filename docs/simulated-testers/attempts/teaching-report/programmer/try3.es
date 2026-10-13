let data be read csv "grades.csv"

let students be unique values of student in data
let averages be empty list
let best be ""
let best_avg be 0

for each name in students:
    let rows be rows of data where student is name
    let avg be average of score in rows
    add avg to averages
    say name + ": " + avg
    if avg > best_avg:
        set best_avg to avg
        set best to name

say "Best average: " + best

let below_50 be 0
for each row in data:
    if score of row < 50:
        set below_50 to below_50 + 1
say "Results below 50: " + below_50

let chart be bar chart of averages
save chart as "averages.svg"
