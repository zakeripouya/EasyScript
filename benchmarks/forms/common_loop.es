note: the 10 most common numbers, by hand: counted in a map, then the counts sorted.
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let result be 0.
repeat 30 times:
    let counts be an empty map from numbers to numbers.
    for each n in numbers:
        set the value for n in counts to (the value for n in counts, or 0) plus 1.
    let common be the first 10 items of (the keys of (counts sorted biggest first)).
    add common added together to result.
say result.
