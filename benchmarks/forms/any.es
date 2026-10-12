note: any: a test no item passes, so every item is looked at, 300 times.
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let result be 0.
repeat 300 times:
    if any number is above 5000, add 1 to result.
say result.
