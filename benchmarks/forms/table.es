note: a table used for every item: the total fee of entries, 100 times.
the fee of an entry depends on its value:
    3 when it's over 900.
    2 when it's over 500.
    1 otherwise.
an entry has a group (text) and a value (a number).
let groups be a list of "north", "south", "east", "west" and "middle".
let entries be an empty list of entries.
let x be 42.
repeat 200000 times:
    set x to x times 16807 mod 2147483647.
    add (a new entry with group item (x mod 5 plus 1) of groups and value x mod 1000) to the end of entries.
let result be 0.
repeat 100 times:
    add the total fee of entries to result.
say result.
