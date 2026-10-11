note: An area calculator: a kind with variants, "check which kind", and fields
note: that can only be used once the variant is known.

keep pi as 3.14159.

a shape is one of:
    a circle with a radius (a number).
    a rectangle with a width (a number) and a height (a number).
    a triangle with a base (a number) and a height (a number).
    a point.

to area of s:
    check which kind s is:
        when it's a circle:
            give back pi times its radius times its radius.
        when it's a rectangle:
            give back its width times its height.
        when it's a triangle:
            give back its base times its height divided by 2.
        when it's a point:
            give back 0.

to describe, for a shape:
    check which kind shape is:
        when it's a circle:
            give back "a circle of radius " followed by its radius.
        when it's a rectangle or a triangle:
            give back "a shape with corners".
        when it's a point:
            give back "a point".

let shapes be a list of (a new circle with radius 1), (a new rectangle with width 3 and height 4).
add (a new triangle with base 6 and height 2) to the end of shapes.
add (a new point) to the end of shapes.

let total be 0.
for each s in shapes:
    say (describe s) followed by ": " followed by area of s.
    add area of s to total.
say "Total area: " followed by total.

note: "if s is a circle" lets the block use s's radius.
let biggest round be 0.
for each s in shapes:
    if s is a circle and s's radius is more than biggest round:
        set biggest round to s's radius.
say "Biggest radius: " followed by biggest round.

note: Values are equal when they're the same variant with equal fields.
say shapes contains (a new point).
say (a new circle with radius 1) is item 1 of shapes.
