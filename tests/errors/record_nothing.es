a node is a kind of thing with:
    a value, which is a number.
    a next, which is a node or nothing.

to find with wanted and start (a node), giving back a node or nothing:
    if start's value is wanted, give back start.
    give back nothing

to value of start (a node), giving back a number:
    give back start's value

let first be a new node with value 1.
say first's next's value.
say (find with 1 and first)'s value.
say value of first's next.
if first is nothing, say "never".
if 5 is not nothing, say "never".
if nothing is nothing, say "always".
let later be first's next.
set first to later.
set first's value to nothing.

to twice with x:
    if x is not nothing, give back item 1 of x.
    give back 0

note: setting what was checked undoes the check, and so does a loop that sets it
if later is not nothing:
    say later's value.
    set later to nothing.
    say later's value.
let walker be first's next.
if walker is not nothing:
    repeat 2 times:
        say walker's value.
        set walker to nothing.
