note: A walk on a grid. Each spot is a point, and points are equal when
note: their x and y are, so "visited contains" finds a spot seen before.
a point is made of an x (a number) and a y (a number).

to step from spot (a point) and way, giving back a point:
    give back a new point with x (the x of spot plus 1) and y (the y of spot) if way is "right".
    give back a new point with x (the x of spot minus 1) and y (the y of spot) if way is "left".
    give back a new point with x (the x of spot) and y (the y of spot plus 1) if way is "up".
    give back a new point with x (the x of spot) and y (the y of spot minus 1).

let moves be a list of "right", "right", "up", "left", "down", "left" and "up".
let at be a new point with x 0 and y 0.
let visited be a list of at.
let repeats be 0.
for each move:
    set at to step from at and move.
    if visited contains at:
        say "Back at ([at's x], [at's y]) after going", move.
        add 1 to repeats.
    otherwise:
        add at to the end of visited.
say "Visited [the number of items in visited] spots, and came back [repeats] times.".
unless repeats is 0, say "The walk ended at", at.
