note: 200,000 rounds of values of a kind holding text, lists, records and other values:
note: made, matched, compared, copied, kept in lists and maps, and dropped.
a shape is one of:
    a circle with a radius (a number).
    a tagged with a label (text), tags (a list of text) and an inner (a shape).
    a point.
a holder is a kind of thing with:
    a shape, which is a shape or nothing.

to label of s:
    check which kind s is:
        when it's a tagged:
            give back its label followed by "!".
        otherwise:
            give back "none".

let total be 0.
let kept be an empty list of shapes.
let named be an empty map from text to shapes.
let h be a new holder.
repeat 200000 times:
    let s be a new tagged with label ("round " followed by it), tags (a list of "a", "b") and inner (a new circle with radius it).
    let copy be a copy of s.
    if copy is not s:
        say "same".
    set h's shape to s.
    if h's shape is a tagged:
        check which kind h's shape's inner is:
            when it's a circle:
                add its radius to total.
            otherwise:
                skip this one.
    add s to the end of kept.
    if the number of items in kept is more than 3, remove the first item from kept.
    set the value for (label with s) in named to copy.
    if the number of entries in named is more than 5, set named to an empty map from text to shapes.
    check which kind s is:
        when it's a point:
            stop the loop.
        otherwise:
            set total to total plus 0.
say total.
