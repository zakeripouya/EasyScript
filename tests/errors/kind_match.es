note: Every mistake in "check which kind".
a shape is one of:
    a circle with a radius (a number).
    a rectangle with a width (a number) and a height (a number).
    a point.
an animal is one of:
    a cat.
    a fish.

let s be a new point.
check which kind s is:
    when it's a circle:
        say its radius.
check which kind s is:
    when it's a circle:
        say 1.
check which kind s is:
    when it's a circle or a cat:
        say 1.
    when it's nothing:
        say 2.
    when it's a circle:
        say 3.
    otherwise:
        say 4.
let n be 5.
check which kind n is:
    when it's nothing:
        say 1.
for each x in a list of 1:
    check which kind s is:
        when it's a point:
            stop the loop.
        otherwise:
            say "something else".
check which kind s is:
    when it's a point:
        let inside be 1.
    otherwise:
        say 2.
say inside.
check which kind s is:
    when it's a point:
        stop the loop.
    otherwise:
        say 2.
