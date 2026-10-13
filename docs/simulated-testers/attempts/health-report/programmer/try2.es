let rows be read csv "appointments.csv"

let clinics be unique clinic of rows
for each c in clinics:
    let clinic_rows be rows where clinic == c
    let missed be number of clinic_rows where status == "missed"
    let total be number of clinic_rows
    let pct be missed / total * 100
    say c + ": " + missed + " missed, " + round pct + "%"

let day_totals be number of rows grouped by weekday
let busiest be max of day_totals
say "Busiest day: " + busiest
