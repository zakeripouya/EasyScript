# "stop the program" from inside loops and an if, with text still held by
# names of the program and of the blocks being left.
let kept be "kept" followed by " text"
repeat 10 times:
    let piece be "piece " followed by it as text
    count from 1 to 3 as n:
        let inner be piece followed by "." followed by n as text
        if piece is "piece 2" and n is 2:
            say inner
            stop the program
say "never printed"
