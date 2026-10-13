read file "timesheet.csv" and call it data

let totals be a list
for each row in data:
    add row.hours to totals of row.employee

for each name in totals:
    say name + ": " + totals of name

say "Overtime:"
for each name in totals:
    if totals of name > 40:
        say name + ": " + (totals of name - 40)

let team_total be sum of hours in data
say "Team total: " + team_total
