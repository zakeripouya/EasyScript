# A million new texts, each replacing the last: only one is alive at a time.
let last be ""
repeat 1000000 times:
    let line be "item " followed by it as text followed by "!"
    set last to line
say last
