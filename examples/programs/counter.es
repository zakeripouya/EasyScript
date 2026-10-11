note: Closures that share state. Each counter keeps its own count, and the
note: functions made by one call of "make counter" all share the same one.

a counter is a kind of thing with:
    step up, which is a function giving back a number.
    step down, which is a function giving back a number.
    reset, which is a function.

to make counter from start (a number), giving back a counter:
    let count be start.
    let up be a function giving back a number:
        add 1 to count.
        give back count.
    let down be a function giving back a number:
        subtract 1 from count.
        give back count.
    let start again be a function:
        set count to start.
    give back a new counter with step up up, step down down and reset start again.

let clicks be call make counter from 0.
let lives be call make counter from 3.

say "Clicks: " followed by call clicks's step up.
say "Clicks: " followed by call clicks's step up.
say "Clicks: " followed by call clicks's step up.
say "Lives: " followed by call lives's step down.
call clicks's reset.
say "Clicks after a reset: " followed by call clicks's step up.
say "Lives are still: " followed by call lives's step down.

note: a function made in each round of a loop keeps that round's name
let greetings be an empty list of functions giving back text.
for each name in a list of "Ann", "Bo" and "Cy":
    let greeting be "Hello, " followed by name followed by "!".
    add (a function that gives back greeting) to the end of greetings.
for each greet in greetings:
    say call greet.
