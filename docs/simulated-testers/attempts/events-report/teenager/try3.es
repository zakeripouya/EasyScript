read file "guests.csv" and call it guests
let total be 0
let going be 0
for each g in guests:
    let r be g's rsvp
    if r is "yes":
        add 1 to going
        add 1 to total
        add g's plus ones to total
say going
say total
total the plus ones per meal
make a pie chart of guests called "meals.svg"
