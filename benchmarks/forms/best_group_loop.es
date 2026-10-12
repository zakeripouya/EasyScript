note: the group with the highest total value, by hand.
an entry has a group (text) and a value (a number).
let groups be a list of "north", "south", "east", "west" and "middle".
let entries be an empty list of entries.
let x be 42.
repeat 200000 times:
    set x to x times 16807 mod 2147483647.
    add (a new entry with group item (x mod 5 plus 1) of groups and value x mod 1000) to the end of entries.
let result be "".
repeat 300 times:
    let totals be an empty map from text to numbers.
    for each e in entries:
        add e's value to the value for e's group in totals, starting at 0.
    let best total be -1.
    for each g and total in totals:
        if total is above best total:
            set result to g.
            set best total to total.
say result.
