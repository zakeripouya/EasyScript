read the table in "guests.csv" and call it guests

let yes_count be 0
let headcount be 0
let fish be 0
let veggie be 0
let chicken be 0

for each g in guests:
    let rsvp be g's rsvp
    let meal be g's meal
    let extra be g's plus ones
    if rsvp is "yes":
        let people be 1 plus extra
        add 1 to yes_count
        add people to headcount
        if meal is "fish":
            add people to fish
        if meal is "veggie":
            add people to veggie
        if meal is "chicken":
            add people to chicken

say "Yes RSVPs: " + yes_count
say "Total headcount: " + headcount
say "fish: " + fish
say "veggie: " + veggie
say "chicken: " + chicken

read the table in "guests.csv" and call it meals
make a pie chart of meals called "meals.svg"
