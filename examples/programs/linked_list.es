note: A linked list. Each node holds a number and the node after it, or
note: nothing at the end. A chain keeps the first node and how many there are.
a node has a value (a number) and a next (a node or nothing).
a chain has a head (a node or nothing) and a size (a number, starting at 0).

a chain can push with value:
    set the head of the chain to a new node with value value and next the chain's head.
    add 1 to the size of the chain.

a chain can pop, giving back a number:
    let top be the head of the chain.
    give back 0 if top is nothing.
    set the head of the chain to top's next.
    subtract 1 from the size of the chain.
    give back top's value.

a chain can describe:
    let line be "".
    let at be the head of the chain.
    while at is not nothing:
        set line to line followed by at's value.
        if at's next is not nothing, set line to line followed by " -> ".
        set at to at's next.
    say "[[[line]]] ([the size of the chain] nodes)".

note: Recursion follows the links: nothing at the end adds nothing.
to sum of start:
    give back 0 if start is nothing.
    give back start's value plus (sum of start's next).

a chain can reverse:
    let done be a new chain.
    let at be the head of the chain.
    while at is not nothing:
        set the head of done to a new node with value at's value and next done's head.
        set at to at's next.
    set the head of the chain to done's head.

let row be a new chain.
count from 1 to 5:
    push row with (it times 10).
describe row.
say "Sum:", sum of row's head.
reverse row.
describe row.
say "Popped", pop row.
say "Popped", pop row.
describe row.
