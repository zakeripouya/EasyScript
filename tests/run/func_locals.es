note: each function has its own names; the same name can be used in several places
let total be "top level"
to sum_to n:
    let total be 0
    count from 1 to n:
        add it to total
    give back total
to twice x:
    let total be x times 2
    if total is greater than 10:
        let note be "big"
        say note
    give back total
say sum_to of 4
say twice of 3
say twice of 6
say total
to counter:
    let k be 0
    add 1 to k
    give back k
say counter
say counter
