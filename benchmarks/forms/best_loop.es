note: the entry with the highest value, by hand.
an entry has a group (text) and a value (a number).
let groups be a list of "north", "south", "east", "west" and "middle".
let entries be an empty list of entries.
let x be 42.
repeat 200000 times:
    set x to x times 16807 mod 2147483647.
    add (a new entry with group item (x mod 5 plus 1) of groups and value x mod 1000) to the end of entries.
let result be 0.
repeat 300 times:
    let best be item 1 of entries.
    for each e in entries:
        if e's value is above best's value, set best to e.
    add best's value to result.
say result.
