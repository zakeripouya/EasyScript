note: Counting, repeating, and leaving loops early.
count from 1 to 3:
    say number
count from 10 to 0 by 5 as n:
    say "down " followed by n
repeat 2 times:
    say "round " followed by it
let total be 1
while total is less than 100:
    multiply total by 2
say total
count from 1 to 10:
    if it mod 2 is 0, skip this one
    if it is greater than 5, stop the loop
    say "odd " followed by it
