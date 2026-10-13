read the table in "appointments.csv" and call it appointments.
let all per clinic be the number of appointments per clinic.
let missed per clinic be the number of appointments whose status is "missed" per clinic.
for each clinic and its count in all per clinic:
    let missed be the value for clinic in missed per clinic, or 0.
    let percent be missed times 100 divided by count, rounded.
    say clinic, "missed", missed, "of", count, "([percent]%)".
say "Busiest day:", the weekday with the highest number of appointments.
