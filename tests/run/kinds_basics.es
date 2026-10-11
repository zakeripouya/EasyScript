note: Kinds with variants: making values, matching, narrowing, fields,
note: equality, copies, printing, methods, and kinds in lists, maps and records.

a shape is one of:
    a circle with a radius (a number).
    a rectangle with a width (a number) and a height (a number).
    a labelled point with a label (text) and tags (a list of text).
    a point.

to area of s:
    check which kind s is:
        when it's a circle:
            give back 3 times its radius times its radius.
        when it's a rectangle:
            give back its width times its height.
        otherwise:
            give back 0.

to describe, for a shape:
    check which kind shape is:
        when it's a circle or a rectangle:
            say "round or square: " followed by area of shape.
        when it's a labelled point:
            say "labelled " followed by its label.
        when it's a point:
            say "just a point".

let shapes be a list of (a new circle with radius 2), (a new rectangle with width 3 and height 4) and (a new point).
add (a new labelled point with label "home" and tags (a list of "a", "b")) to the end of shapes.
for each s in shapes:
    say s followed by ": " followed by area of s.
    describe s.

note: Narrowing: "if", "and", "otherwise", early exits, and a value just made.
let c be a new circle with radius 3.
say c's radius.
let first be item 1 of shapes.
if first is a circle and first's radius is 2:
    say "a circle of radius 2".
if first is not a circle:
    say "not a circle".
otherwise:
    say "radius " followed by first's radius.
to width of s:
    if s isn't a rectangle, give back 0.
    give back s's width.
say width of (item 2 of shapes).
say width of c.

note: "it" is the value checked; a loop inside a "when" has its own "it".
check which kind item 2 of shapes is:
    when it's a rectangle:
        count from 1 to 2:
            say "round " followed by it.
        say its height.
    otherwise:
        say "no".

note: "stop the loop" inside a "when" leaves the loop around it.
for each s in shapes:
    check which kind s is:
        when it's a rectangle:
            say "found the rectangle".
            stop the loop.
        otherwise:
            skip this one.

note: Equality is by value; a copy has its own lists.
say c is a new circle with radius 3.
say c is a new circle with radius 4.
say c is a new point.
say shapes contains (a new point).
say the position of (a new point) in shapes.
let home be item 4 of shapes.
let other be a copy of home.
if other is a labelled point:
    add "c" to the end of other's tags.
    say other's tags.
if home is a labelled point:
    say home's tags.
say home is other.

note: Kinds in maps and in records, and "or nothing".
let named be a map of "big" to (a new circle with radius 10), "small" to (a new point).
for each name and s in named:
    if s is a circle, say name followed by " is round".
a drawing is a kind of thing with:
    a title, which is text.
    a main shape, which is a shape or nothing.
let d be a new drawing with title "sun".
check which kind d's main shape is:
    when it's nothing:
        say "empty".
    otherwise:
        say "something".
set d's main shape to a new circle with radius 5.
if d's main shape is a circle:
    say "radius " followed by d's main shape's radius.
say d.

note: A recursive kind.
an expression is one of:
    a number value with an amount (a number).
    a sum with a left (an expression) and a right (an expression).
to evaluate with e:
    check which kind e is:
        when it's a number value:
            give back its amount.
        when it's a sum:
            give back (evaluate with its left) plus (evaluate with its right).
say evaluate with (a new sum with left (a new number value with amount 2) and right (a new number value with amount 5)).
