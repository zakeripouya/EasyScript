# A chain of a million records is freed when nothing holds its first one.
# Freeing works through a list of what's left to free, not by calling
# itself, so a long chain can't run out of C stack.
a node is a kind of thing with:
    a value, which is a number.
    a next, which is a node or nothing.
a chain is a kind of thing with:
    a head, which is a node or nothing.

let row be a new chain.
count from 1 to 1000000:
    set row's head to a new node with value it and next row's head.
let at be row's head.
let total be 0.
while at is not nothing:
    add at's value to total.
    set at to at's next.
say total.
set row's head to nothing.
say "freed".
