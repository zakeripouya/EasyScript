note: A guessing game. The computer picks a number; you guess it.
let secret be a random number from 1 to 20.
let tries be 0.
let guess be 0.
keep doing this until guess is secret:
    ask "Guess a number from 1 to 20: " and call it guess.
    add 1 to tries.
    if guess is above secret, say "Lower!"
    if guess is below secret, say "Higher!"
say "You got it in [tries] tries!"
