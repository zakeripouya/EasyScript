read csv "bookings.csv" and call it bookings

let body be ""
for each b in bookings:
    set body to body + b.room + " - " + b.day + " " + b.time + " (" + b.booked_by + ")\n"

every monday at "09:00":
    send email to "team@ourcompany.com" with subject "Room bookings this week" and body body
