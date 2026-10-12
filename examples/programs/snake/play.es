note: Snake, in the terminal. Steer with the arrow keys (or W, A, S and D),
note: eat the food (*) to grow, and don't run into the walls or yourself.
note: Q or Escape stops. Run it with:
note:     easyscript run examples/programs/snake/play.es
note: It's written with events: the game moves on every 150 milliseconds, keys
note: steer it as they come, and a rule ends it when it's over.

use board.

let g be start game with 24 and 12.

whenever a key is pressed:
    if it is "q" or it is "escape", stop the program.
    turn with g and it.

every 150 milliseconds:
    step with g.
    clear the screen.
    say picture of g.
    say "Score:", g's score, "    Arrows or WASD to steer, Q to stop.".

whenever g's over:
    say "Game over! Your score was", g's score followed by ".".
    stop the program.
