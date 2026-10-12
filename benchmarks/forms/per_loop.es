note: the total value per group, by hand with a map.
an entry has a group (text) and a value (a number).
let groups be a list of "north", "south", "east", "west" and "middle".
let entries be an empty list of entries.
let x be 42.
repeat 200000 times:
    set x to x times 16807 mod 2147483647.
    add (a new entry with group item (x mod 5 plus 1) of groups and value x mod 1000) to the end of entries.
let result be 0.
repeat 100 times:
    let totals be an empty map from text to numbers.
    for each e in entries:
        set the value for (e's group) in totals to (the value for (e's group) in totals, or 0) plus e's value.
    add the value for "north" in totals to result.
say result.
