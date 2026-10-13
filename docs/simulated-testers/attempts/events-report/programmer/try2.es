read the table in "guests.csv" and call it guests

let yes_count be 0
let headcount be 0
let fish be 0
let veggie be 0
let chicken be 0

for each g in guests:
    if g.rsvp is "yes":
        let people be 1 plus g.plus_ones
        add 1 to yes_count
        add people to headcount
        if g.meal is "fish":
            add people to fish
        if g.meal is "veggie":
            add people to veggie
        if g.meal is "chicken":
            add people to chicken

say "Yes RSVPs: " + yes_count
say "Total headcount: " + headcount
say "fish: " + fish
say "veggie: " + veggie
say "chicken: " + chicken

make a pie chart of fish, veggie, chicken called "meals.svg"
