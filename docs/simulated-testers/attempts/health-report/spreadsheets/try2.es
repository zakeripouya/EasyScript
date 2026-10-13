Read "appointments.csv"
For each clinic:
  let Missed be COUNTIF(status, "missed")
  let Total be COUNT(rows)
  let MissedPct be Missed / Total * 100
  Show clinic, Missed, MissedPct
count the appointments per weekday
Show the weekday with the highest count
