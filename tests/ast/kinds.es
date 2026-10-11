note: Every form of kinds with variants.
a shape is one of:
    a circle with a radius (a number).
    a rectangle with a width (a number), a height (a number) and a label (text).
    a point.
an expression is one of:
    a number value with an amount (a number).
    a sum with a left (an expression) and a right (an expression).

let s be a new circle with radius 3.
let e be a new sum with left (a new number value with amount 1) and right (a new number value with amount 2).
let p be a new point.
if s is a circle and s's radius is 3, say s's radius.
if s is not a point, say 1.
if s isn't a rectangle, say 2.
say s is circle.
check which kind s is:
    when it's a circle:
        say its radius.
    when it is a rectangle or a point:
        say it's width.
    otherwise:
        say "other".
check which kind (item 1 of shapes) is:
    when it's a circle or nothing:
        say 1.
to describe, for a shape:
    say shape.
to scale with s (a shape) and factor, giving back a shape or nothing:
    give back s.
let shapes be an empty list of shapes.
let its be 5.
say its.
