note: Roll two dice a thousand times and count each total.
note: "use the same random numbers every time" makes every run the same, so this output can be checked.
use the same random numbers every time.

let counts be an empty map from numbers to numbers
repeat 1000 times:
    let total be roll a die plus roll a die
    set the value for total in counts to (the value for total in counts, or 0) plus 1

count from 2 to 12 as total:
    let rolled be the value for total in counts, or 0
    let bar be ""
    repeat (rolled divided by 10) rounded times:
        set bar to bar followed by "#"
    say total followed by ": " followed by bar followed by " " followed by rolled

let players be a list of "Ann", "Bo", "Cy" and "Di"
shuffle players
say "Playing order: " followed by players
say "First to pick a card: " followed by pick one of players
