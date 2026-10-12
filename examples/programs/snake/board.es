note: The rules of Snake, without the screen or the keyboard: a board, a
note: snake that moves one cell at a time, food that makes it longer, and
note: the game ending when the snake runs into a wall or into itself.
note:
note: A cell is one number, from 0: its row times the width, plus its column.
note: (A cell with two fields would be a record, and two records are never
note: equal, so "contains" couldn't find a cell in the snake.)

a game is a kind of thing with:
    width, which is a number.
    height, which is a number.
    snake, which is a list of numbers.
    heading, which is text, starting at "right".
    food, which is a number, starting at -1.
    score, which is a number, starting at 0.
    over, which is yes or no, starting at no.

to column of cell and g, giving back a number:
    give back cell mod g's width.

to row of cell and g, giving back a number:
    give back (cell minus (column of cell and g)) divided by g's width.

note: A new game: a snake of three cells in the middle row, heading right,
note: and food somewhere else. The board has to be at least 3 cells wide.
to start game with width and height, giving back a game:
    let middle be ((height divided by 2) rounded down) times width plus the bigger of 2 and ((width divided by 2) rounded down).
    let g be a new game with width width, height height and snake (a list of middle, middle minus 1 and middle minus 2).
    place food in g.
    give back g.

to place food in g:
    let free be an empty list of numbers.
    count from 0 to g's width times g's height minus 1 as cell:
        if g's snake doesn't contain cell, add cell to the end of free.
    if free is empty:
        set g's over to yes.
    otherwise:
        set g's food to pick one of free.

note: Turning: "up", "down", "left" or "right" (W, A, S and D work too). A
note: snake can't turn straight back into itself, so that's ignored.
to turn with g and key:
    let directions be a map of "w" to "up", "a" to "left", "s" to "down" and "d" to "right".
    let heading be key in lowercase.
    if directions has heading, set heading to the value for heading in directions.
    let opposites be a map of "up" to "down", "down" to "up", "left" to "right" and "right" to "left".
    if opposites doesn't have heading, give back.
    if the value for heading in opposites is not g's heading:
        set g's heading to heading.

note: One step: the head moves on; eating food makes the snake one longer.
to step with g:
    if g's over, give back.
    let head be first item of g's snake.
    let x be column of head and g.
    let y be row of head and g.
    if g's heading is "up", subtract 1 from y.
    if g's heading is "down", add 1 to y.
    if g's heading is "left", subtract 1 from x.
    if g's heading is "right", add 1 to x.
    if x is below 0 or y is below 0 or x is at least g's width or y is at least g's height:
        set g's over to yes.
        give back.
    let next be y times g's width plus x.
    let eating be next is g's food.
    let body be g's snake.
    if not eating, set body to g's snake without the last item.
    if body contains next:
        set g's over to yes.
        give back.
    if not eating, remove the last item from g's snake.
    insert next at position 1 of g's snake.
    if eating:
        add 1 to g's score.
        place food in g.

note: The board as text: a wall of "#", the head "@", the body "o" and the food "*".
to picture of g, giving back text:
    let wall be "#".
    repeat g's width plus 1 times:
        set wall to wall followed by "#".
    let lines be a list of wall.
    count from 0 to g's height minus 1 as y:
        let line be "#".
        count from 0 to g's width minus 1 as x:
            let cell be y times g's width plus x.
            if cell is first item of g's snake:
                set line to line followed by "@".
            otherwise if g's snake contains cell:
                set line to line followed by "o".
            otherwise if cell is g's food:
                set line to line followed by "*".
            otherwise:
                set line to line followed by " ".
        add line followed by "#" to the end of lines.
    add wall to the end of lines.
    give back lines joined with "\n".
