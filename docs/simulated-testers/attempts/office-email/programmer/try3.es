read file "bookings.csv" and call it bookings

let body be ""
for each b in bookings:
    set body to body + (room of b) + " - " + (day of b) + " " + (time of b) + " (" + (booked by of b) + ")\n"

every Monday at 9am:
    email body to "team@ourcompany.com" with subject "Room bookings this week"
