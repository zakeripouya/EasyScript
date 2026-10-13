read the table in "tasks.csv" and call it tasks.
for each task in tasks sorted by due date:
    let days left be the number of days between today and task's due date.
    say task's task, "is due in", days left, "days."
    if days left is at most 3, say "  That's soon!"
