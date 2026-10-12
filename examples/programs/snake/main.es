note: A scripted game of Snake, testing the rules in board.es (play.es is the
note: game itself, played with the keyboard). The food is put where the test
note: wants it, so nothing depends on random numbers.

use board.
use the same random numbers every time.

to describe with g and what:
    say what followed by " (score " followed by g's score followed by ", " followed by the number of items in g's snake followed by " long):".
    say picture of g.

let g be start game with 8 and 5.
set g's food to 21.
describe with g and "A new game, with the food just ahead".
step with g.
describe with g and "One step right: the snake eats and grows".
turn with g and "left".
step with g.
describe with g and "Turning straight back is ignored".
turn with g and "s".
step with g.
step with g.
describe with g and "Down twice (S is down)".
step with g.
say "Over after hitting the bottom wall: " followed by g's over.
step with g.
say "Steps after the end change nothing: " followed by the number of items in g's snake.

note: Running into itself: a long snake turning in a tight square. Moving
note: into the cell the tail is leaving is fine.
let h be start game with 8 and 5.
set h's snake to a list of 20, 19, 18, 17, 16.
set h's food to 0.
turn with h and "down".
step with h.
turn with h and "left".
step with h.
turn with h and "up".
step with h.
describe with h and "A long snake turns into itself".
say "Over: " followed by h's over.

let square be start game with 8 and 5.
set square's snake to a list of 8, 9, 1 and 0.
turn with square and "up".
step with square.
say "Following its own tail around a square is fine: over is " followed by square's over followed by ", the snake is at " followed by square's snake followed by ".".

note: Food never lands on the snake.
let small be start game with 4 and 1.
say "On a board of 4, the food is at " followed by small's food followed by ", the snake at " followed by small's snake followed by ".".
step with small.
say "Eating the last free cell fills the board, and the game is over: " followed by small's over.
