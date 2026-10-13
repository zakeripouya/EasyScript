# Count the days until each deadline

List the tasks by their due dates, with how many days are left, and point out the ones due within three days.

## The program

`deadlines.es`:

```
read the table in "tasks.csv" and call it tasks.
for each task in tasks sorted by due date:
    let days left be the number of days between today and task's due date.
    say task's task, "is due in", days left, "days."
    if days left is at most 3, say "  That's soon!"
```

## The data

`tasks.csv`, in the same folder:

```
task,due date
Quarterly report,2026-10-20
Budget review,2026-10-14
Team survey,2026-11-02
```

## Running it

`easyscript run deadlines.es` shows:

```
Budget review is due in 3 days.
  That's soon!
Quarterly report is due in 9 days.
Team survey is due in 22 days.
```

## Under the hood

`today` is the date the program runs on. In this cookbook's tests it's fixed at October 11, 2026, so the output stays the same.
