note: any, by hand.
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let result be 0.
repeat 300 times:
    let found be no.
    for each n in numbers:
        if n is above 5000:
            set found to yes.
            stop the loop.
    if found, add 1 to result.
say result.
