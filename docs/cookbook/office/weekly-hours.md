# Add up the hours on a timesheet

Show each employee's hours for the week, the overtime of anyone over 40 hours, and the hours of the whole team.

## The program

`weekly-hours.es`:

```
read the table in "timesheet.csv" and call it timesheet.
let hours be the total hours of timesheet per employee.
for each employee and its total in hours:
    say employee, total, "hours".
    if total is over 40, say "  overtime:", total minus 40, "hours".
say "Whole team:", the total hours of timesheet, "hours".
```

## The data

`timesheet.csv`, in the same folder:

```
employee,day,hours
Rosa,Mon,9
Rosa,Tue,8.5
Rosa,Wed,9
Rosa,Thu,9
Rosa,Fri,7
Tom,Mon,8
Tom,Tue,8
Tom,Wed,8
Tom,Thu,6
Tom,Fri,8
Lena,Mon,10
Lena,Tue,10
Lena,Wed,9.5
Lena,Thu,8
Lena,Fri,6
```

## Running it

`easyscript run weekly-hours.es` shows:

```
Rosa 42.5 hours
  overtime: 2.5 hours
Tom 38 hours
Lena 43.5 hours
  overtime: 3.5 hours
Whole team: 124 hours
```
