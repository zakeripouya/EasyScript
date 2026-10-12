note: A linked list. Each node holds a number and the node after it, or
note: nothing at the end. A chain keeps the first node and how many there are.
a node is a kind of thing with:
    a value, which is a number.
    a next, which is a node or nothing.

a chain is a kind of thing with:
    a head, which is a node or nothing.
    a size, which is a number, starting at 0.

to push, for a chain, with value:
    set the chain's head to a new node with value value and next the chain's head.
    add 1 to the chain's size.

to pop, for a chain, giving back a number:
    let top be the chain's head.
    if top is nothing, give back 0.
    set the chain's head to top's next.
    subtract 1 from the chain's size.
    give back top's value

to describe, for a chain:
    let line be "".
    let at be the chain's head.
    while at is not nothing:
        set line to line followed by at's value.
        if at's next is not nothing, set line to line followed by " -> ".
        set at to at's next.
    say "[[" followed by line followed by "]] (" followed by the chain's size followed by " nodes)".

note: Recursion follows the links: nothing at the end adds nothing.
to sum of start:
    if start is nothing, give back 0.
    give back start's value plus (sum of start's next)

to reverse, for a chain:
    let done be a new chain.
    let at be the chain's head.
    while at is not nothing:
        set done's head to a new node with value at's value and next done's head.
        set at to at's next.
    set the chain's head to done's head.

let row be a new chain.
count from 1 to 5:
    push row with (it times 10).
describe row.
say "Sum: " followed by (sum of row's head).
reverse row.
describe row.
say "Popped " followed by (pop row).
say "Popped " followed by (pop row).
describe row.
