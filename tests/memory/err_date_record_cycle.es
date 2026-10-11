# Records with dates in them that point to each other in a circle: the
# memory check finds the circle and prints the dates.
a visit is a kind of thing with:
    day, which is a date.
    next, which is a visit or nothing.

let first visit be a new visit with day yesterday.
let second visit be a new visit with day today and next first visit.
set first visit's next to second visit.
say "made a circle of two".
