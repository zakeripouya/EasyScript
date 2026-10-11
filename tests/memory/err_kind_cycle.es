note: A value of a kind can't change, but a list in it can: a tree whose
note: list of children holds the tree itself is a circle, reported at the end.
a tree is one of:
    a leaf.
    a node with children (a list of trees).
let kids be a list of (a new leaf).
let t be a new node with children kids.
add t to the end of kids.
say the number of items in kids.
