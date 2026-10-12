note: the top 10, 30 times (a heap of 10).
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let result be 0.
repeat 30 times:
    add the total of (the top 10 numbers) to result.
say result.
