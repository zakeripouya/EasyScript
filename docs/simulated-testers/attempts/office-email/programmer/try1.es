function send_bookings():
    bookings = read csv "bookings.csv"
    body = ""
    for b in bookings:
        body = body + b.room + " - " + b.day + " " + b.time + " (" + b["booked by"] + ")\n"
    send_email(to="team@ourcompany.com", subject="Room bookings this week", body=body)

schedule(every="monday", at="09:00", run=send_bookings)
