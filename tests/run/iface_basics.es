note: interfaces: any kind with the ways fits, without saying so

a shape is anything with:
    a way to area, giving back a number.
    a way to scale with factor (a number), giving back a shape.
something that can be named is anything with:
    a way to label, giving back text.

a square is a kind of thing with:
    side, which is a number.
a figure is one of:
    a circle with radius (a number).
    a dot.

to area, for a square:
    give back the square's side times the square's side.
to scale, for a square, with factor:
    give back a new square with side (the square's side times factor).
to label, for a square:
    give back "a square of side " followed by the square's side.
to area, for a figure, giving back a number:
    check which kind the figure is:
        when it's a circle:
            give back 3 times its radius times its radius.
        otherwise:
            give back 0.
to scale, for a figure, with factor (a number), giving back a shape:
    check which kind the figure is:
        when it's a circle:
            give back a new circle with radius (its radius times factor).
        otherwise:
            give back a new dot.

to total area of shapes (a list of shapes), giving back a number:
    let total be 0.
    for each s in shapes:
        add area s to total.
    give back total.

let shapes be an empty list of shapes.
add (a new square with side 2) to the end of shapes.
add (a new circle with radius 1) to the end of shapes.
add (a new dot) to the end of shapes.
say total area of shapes.
let bigger be (a function taking s that gives back scale s with 10) applied to every s in shapes.
say total area of bigger.
say bigger.

note: in a record, a map, and "or nothing"
a canvas is a kind of thing with:
    main, which is a shape.
    spare, which is a shape or nothing.
let canvas be a new canvas with main (a new square with side 3).
say area canvas's main.
if canvas's spare is nothing, say "no spare".
set canvas's spare to a new dot.
if canvas's spare is not nothing:
    say area canvas's spare.
let named be an empty map from text to things that can be named.
set the value for "s" in named to a new square with side 5.
say label (the value for "s" in named).

note: the same record is the same, a copy of a list of them shares them
let first be item 1 of shapes.
say first is item 1 of shapes.
say first is item 2 of shapes.
say shapes contains first.
