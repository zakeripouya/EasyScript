let k be 0
forever:
    add 1 to k
    if k is 2, skip this one
    if k is 5, stop the loop
    say "forever " followed by k
count from 1 to 10:
    if number mod 2 is 0, continue
    if number is greater than 5, break
    say "odd " followed by number
count from 1 to 3 as outer:
    count from 1 to 3 as inner:
        if inner is 2, stop
        say outer followed by "," followed by inner
repeat 3 times:
    if it is 2, move on
    say "moved " followed by it
repeat 5 times:
    if it is 2, skip
    if it is 4:
        stop the loop
    say "skip/stop " followed by it
forever:
    say "ending the program"
    stop the program
