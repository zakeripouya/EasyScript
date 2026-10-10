# Names made inside blocks that are left early: skip this one, stop the loop,
# and give back from inside a loop.
to first_long with limit:
    count from 1 to limit as n:
        let word be "w" followed by n as text
        if length of word is 4:
            give back word
    give back "none"
let kept be 0
repeat 1000000 times:
    let piece be "p" followed by it as text
    if it mod 2 is 0:
        skip this one
    increase kept by 1
    if it is 999999:
        let last be piece followed by "!"
        say last
        stop the loop
say kept
let found be ""
repeat 1000 times:
    set found to first_long with 2000
say found
