Load appointments.csv
For each clinic:
  Missed = COUNTIF(status, "missed")
  Total = COUNT(rows)
  Missed % = Missed / Total * 100
Show clinic, Missed, Missed %
Pivot by weekday, count of appointments
Show the weekday with MAX count
