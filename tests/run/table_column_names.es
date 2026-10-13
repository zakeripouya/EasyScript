# A table's columns are field names the program can use, even with several
# words or an EasyScript word in them; a column called "day" is the field
# in "per day"; "sorted by" takes a column of several words.
put the text below in file "guests.csv":
    name,rsvp,plus ones,booked by,due date,day
    Ana,yes,1,Rosa,2026-10-20,Monday
    Eli,yes,2,Tom,2026-10-14,Tuesday
    Cy,no,0,Rosa,2026-11-02,Monday

read the table in "guests.csv" and call it guests.
say the total plus ones of guests.
say the number of guests per booked by.
say the number of guests per day.
for each guest in guests sorted by plus ones:
    say guest's name, guest's plus ones, guest's booked by, guest's due date.
say the number of days between today and (item 1 of guests)'s due date.
for each guest in guests sorted by due date:
    say guest's name, guest's due date.
