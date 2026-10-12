note: the top 10, by hand: sorted, then the first 10.
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let result be 0.
repeat 30 times:
    add (the first 10 items of (numbers sorted biggest first)) added together to result.
say result.
