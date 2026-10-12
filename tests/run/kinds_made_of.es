note: "a point is made of ...": a kind whose values compare by their fields and never change.
a point is made of an x (a number) and a y (a number).
let start be a new point with x 1 and y 2.
let same be a new point with x 1 and y 2.
say start.
say start is same.
say start's x plus the y of start.
let visited be a list of start.
say visited contains (a new point with x 1 and y 2).
say visited contains (a new point with x 2 and y 1).
say the position of same in visited.
let moved be a new point with x (start's x plus 1) and y start's y.
say moved.
a cell is made of a row (a number), a column (a number) and a label (text).
let cells be a map of "start" to (a new cell with row 1, column 1 and label "S").
say the value for "start" in cells.
check which kind start is:
    when it's a point:
        say "a point at", its x, "and", its y.
to step right from p (a point), giving back a point:
    give back a new point with x (the x of p plus 1) and y (the y of p).
say step right from start.
say start is a point.
