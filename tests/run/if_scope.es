note: names made inside a block exist only inside it, so each branch can make its own
let x be 3
if x is 3:
    let label be "three"
    say label
otherwise:
    let label be "other"
    say label
if x is 3:
    let label be "again"
    say label
