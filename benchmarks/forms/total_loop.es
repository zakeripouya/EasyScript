note: the total, by hand.
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let result be 0.
repeat 300 times:
    for each n in numbers:
        add n to result.
say result.
