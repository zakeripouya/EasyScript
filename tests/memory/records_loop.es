# 200,000 rounds making records that hold text, lists and other records,
# changing, copying and dropping them, with live memory kept small.
a node is a kind of thing with:
    a label, which is text.
    a next, which is a node or nothing.
    the marks, which is a list of numbers, starting at an empty list of numbers.

a holder is a kind of thing with:
    a first, which is a node or nothing.
    a size, which is a number, starting at 0.

let keep be a new holder.
let total be 0.
count from 1 to 200000:
    let item be a new node with label "item " followed by it and next keep's first.
    add it to the end of item's marks.
    set keep's first to item.
    add 1 to keep's size.
    if keep's size is 20:
        set keep's first to nothing.
        set keep's size to 0.
    let twin be a copy of item.
    set twin's label to twin's label followed by "!".
    add length of twin's label to total.
    let group be list of item, twin.
    remove item 1 from group.
say total.
say keep's size.
