note: Functions given to built-ins: a million pseudo-random numbers (x times
note: 16807 mod 2147483647, each kept mod 1000), ten times over: each doubled
note: (plus the round), the ones divisible by three kept, and those added up,
note: with "applied to every", "that pass" and "combined using".
note: pipeline_loop does the same with a loop.
to double with x (a number):
    give back x times 2.
to wanted with x (a number):
    give back x mod 3 is 0.
to sum of so far (a number) and x (a number):
    give back so far plus x.
let numbers be an empty list of numbers.
let x be 42.
repeat 1000000 times:
    set x to x times 16807 mod 2147483647.
    add x mod 1000 to the end of numbers.
let total be 0.
repeat 10 times:
    let doubled be double applied to every number in numbers.
    let kept be the numbers in doubled that pass wanted.
    add (kept combined using sum, starting from 0) to total.
    apply (a function taking n that gives back n plus 1) to every number in numbers.
say total.
