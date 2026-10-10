# Text passed into a function and given back, a million times.
to label with n:
    let prefix be "#"
    give back prefix followed by n as text
let last be ""
repeat 1000000 times:
    set last to label with it
say last
