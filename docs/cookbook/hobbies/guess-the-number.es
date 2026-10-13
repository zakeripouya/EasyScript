let secret be a random number from 1 to 10.
let guesses be 0.
let guess be 0.
keep doing this until guess is secret:
    ask "Your guess? " and call it guess.
    add 1 to guesses.
    if guess is above secret, say "Too high."
    if guess is below secret, say "Too low."
say "You got it in [guesses] guesses!"
