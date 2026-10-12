note: Keys and schedules: a key from the input before each round of the schedules,
note: "every day at", "every Monday at".
let moves be 0.
whenever a key is pressed:
    add 1 to moves.
    say "key", it, "(move", moves followed by ")".
    if it is "q", stop the program.
every day at 9pm:
    say "Good evening:", the time now.
every Monday at 9:30am:
    say "Monday meeting:", the time now.
every Tuesday at 0:15:
    say "Tuesday:", the time now.
