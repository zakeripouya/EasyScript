read file "appointments.csv" and call it appointments
For each clinic:
  let Missed be COUNTIF status = "missed"
  let Total be COUNT rows
  let MissedPct be Missed / Total * 100
  Show clinic, Missed, MissedPct
For each weekday:
  let Appointments be COUNT rows
Show the weekday with the highest Appointments
