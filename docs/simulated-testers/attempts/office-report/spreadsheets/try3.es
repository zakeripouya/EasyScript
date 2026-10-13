read file "timesheet.csv" and call it timesheet
let totals be the total hours of timesheet per employee
say totals
for each row in totals:
    if hours > 40, say employee and hours - 40
let grand_total be SUM of hours of timesheet
say grand_total
