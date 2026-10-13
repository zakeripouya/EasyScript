read file "guests.csv" and call it guests
let yes be how many guests have rsvp yes
say yes
let total be 0
for each g in guests:
    if g rsvp is yes:
        add 1 plus g plus ones to total
say total
total the people per meal
make a pie chart of meals called "meals.svg"
