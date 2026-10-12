note: Snake, in the terminal. Steer with the arrow keys (or W, A, S and D),
note: eat the food (*) to grow, and don't run into the walls or yourself.
note: Q or Escape stops. Run it with:
note:     easyscript run examples/programs/snake/play.es

use board.

let g be start game with 24 and 12.
let quitting be no.
while not g's over and not quitting:
    let key be the key being pressed.
    while key is not nothing:
        if key is "q" or key is "escape":
            set quitting to yes.
        otherwise:
            turn with g and key.
        set key to the key being pressed.
    clear the screen.
    say picture of g.
    say "Score: " followed by g's score followed by "    Arrows or WASD to steer, Q to stop.".
    step with g.
    wait 150 milliseconds.
clear the screen.
say picture of g.
say "Game over! Your score was " followed by g's score followed by ".".
