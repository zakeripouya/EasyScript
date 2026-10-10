# Text that grows to 1,000 characters and starts again, a million times.
let s be ""
let restarts be 0
repeat 1000000 times:
    set s to s followed by "x"
    if length of s is 1000:
        set s to ""
        increase restarts by 1
say restarts
