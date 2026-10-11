note: A number that can be nothing, checked, then set to nothing through another name for its record.
a counter is a kind of thing with:
    a count, which is a number or nothing.

to reset with c:
    set c's count to nothing.

let tally be a new counter with count 3.
if tally's count is not nothing:
    reset with tally.
    say tally's count plus 1.
