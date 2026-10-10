note: Sorting: a million pseudo-random numbers (the same ones in every
note: language: x times 16807 mod 2147483647), sorted smallest first.
let numbers be an empty list of numbers
let x be 42
repeat 1000000 times:
    set x to x times 16807 mod 2147483647
    add x to the end of numbers
sort numbers
say item 1 of numbers and " " and item 500000 of numbers and " " and item 1000000 of numbers
