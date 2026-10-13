read the table in "timesheet.csv" and call it timesheet.
let hours be the total hours of timesheet per employee.
for each employee and its total in hours:
    say employee, total, "hours".
    if total is over 40, say "  overtime:", total minus 40, "hours".
say "Whole team:", the total hours of timesheet, "hours".
