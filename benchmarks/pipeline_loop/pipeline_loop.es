note: The pipeline benchmark written as loops: the same million numbers, ten
note: times over: doubled, the ones divisible by three added up, then each
note: number made one bigger.
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let total be 0.
repeat 10 times:
    for each n in numbers:
        let doubled be n times 2.
        if doubled mod 3 is 0, add doubled to total.
    count from 1 to the number of items in numbers as i:
        set item i of numbers to item i of numbers plus 1.
say total.
