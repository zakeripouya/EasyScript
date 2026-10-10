note: A number guessing game. (make test types the guesses from guessing_game.in.)
keep secret as 42
keep most_tries as 10
let tries be 0
let found be no
say "I'm thinking of a number between 1 and 100."
repeat until found or tries reaches most_tries:
    ask "Your guess? " and call the answer reply
    let guess be reply as a number
    add 1 to tries
    if guess is secret:
        set found to yes
    otherwise if guess is less than secret:
        say "Higher!"
    otherwise:
        say "Lower!"
if found:
    say "You got it in " followed by tries followed by " tries."
otherwise:
    say "Out of tries! It was " followed by secret followed by "."
