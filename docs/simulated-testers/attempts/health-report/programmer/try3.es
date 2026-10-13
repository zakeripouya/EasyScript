let rows be read "appointments.csv"

let clinics be unique(rows, "clinic")
for each c in clinics:
    let clinic_rows be rows where clinic == c
    let missed be number of clinic_rows where status == "missed"
    let total be number of clinic_rows
    let pct be missed / total * 100
    say c + ": " + missed + " missed, " + round(pct) + "%"

let days be "Monday", "Tuesday", "Wednesday", "Thursday", "Friday"
let best_day be ""
let best_n be 0
for each d in days:
    let n be number of rows where weekday == d
    if n > best_n:
        set best_n to n
        set best_day to d
say "Busiest day: " + best_day + " with " + best_n + " appointments"
