note: 200,000 rounds of events: text made in every tick, keys never pressed,
note: and rules checked each round. (The clock is fixed, so waiting only moves it.)
let ticks be 0.
let log be "".
every 1 millisecond:
    add 1 to ticks.
    set log to "tick " followed by ticks.
    if ticks is 200000:
        say log.
        stop the program.
whenever ticks mod 1000 is 0:
    let note be "round " followed by ticks.
    set log to note.
