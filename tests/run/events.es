note: Events: "every N seconds", "whenever", "when", run once the rest of the program has.
note: (Tests fix the clock with ES_NOW, so waiting only moves it.)
let ticks be 0.
let score be 0.
every 2 seconds:
    add 1 to ticks.
    add 40 to score.
    say "tick", ticks, "score", score.
    if ticks is 5, stop the program.
every 3 seconds:
    say "  (three seconds)".
whenever score is over 100:
    say "Level up!".
whenever score is at least 120 and score is at most 160:
    say "In the middle.".
when score is over 50, say "Halfway there.".
say "Started at", the time now.
