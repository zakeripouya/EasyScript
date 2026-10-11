note: A binary search tree: smaller keys go left, bigger ones right, so
note: going left, then the node, then right visits the keys in order.
a tree node is a kind of thing with:
    a key, which is a number.
    a left, which is a tree node or nothing.
    a right, which is a tree node or nothing.

to place with key and tree (a tree node or nothing), giving back a tree node:
    if tree is nothing, give back a new tree node with key key.
    if key is less than tree's key:
        set tree's left to place with key and tree's left
    otherwise:
        set tree's right to place with key and tree's right
    give back tree

to walk with tree:
    if tree is nothing, return.
    walk with tree's left.
    say tree's key.
    walk with tree's right.

to holds with key and tree, giving back yes or no:
    if tree is nothing, give back no.
    if key is tree's key, give back yes.
    if key is less than tree's key, give back holds with key and tree's left.
    give back holds with key and tree's right

to depth of tree:
    if tree is nothing, give back 0.
    let left depth be depth of tree's left.
    let right depth be depth of tree's right.
    if left depth is greater than right depth, give back left depth plus 1.
    give back right depth plus 1

let root be a new tree node with key 50.
for each key in list of 30, 70, 20, 40, 60, 80, 35:
    set root to place with key and root.
walk with root.
say "Depth: " followed by (depth of root).
say "Holds 35: " followed by (holds with 35 and root).
say "Holds 36: " followed by (holds with 36 and root).
say root's left.
