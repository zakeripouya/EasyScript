note: the entries sorted by their value, biggest first, with a definition giving the value.
an entry has a group (text) and a value (a number).
to value of entry:
    give back entry's value.
let groups be a list of "north", "south", "east", "west" and "middle".
let entries be an empty list of entries.
let x be 42.
repeat 200000 times:
    set x to x times 16807 mod 2147483647.
    add (a new entry with group item (x mod 5 plus 1) of groups and value x mod 1000) to the end of entries.
let result be 0.
repeat 20 times:
    let sorted be entries sorted by value biggest first.
    add (first item of sorted)'s value plus (last item of sorted)'s value to result.
say result.
