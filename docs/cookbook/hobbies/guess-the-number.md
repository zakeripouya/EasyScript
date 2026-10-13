# Guess the number

The computer picks a whole number from 1 to 10, then says "Too high" or "Too low" after each guess, and how many guesses it took.

## The program

`guess-the-number.es`:

```
let secret be a random number from 1 to 10.
let guesses be 0.
let guess be 0.
keep doing this until guess is secret:
    ask "Your guess? " and call it guess.
    add 1 to guesses.
    if guess is above secret, say "Too high."
    if guess is below secret, say "Too low."
say "You got it in [guesses] guesses!"
```

## Running it

When it's run with `easyscript run guess-the-number.es` and these answers are typed (`1`, `2`, `3`, `4`, `5`, `6`, `7`, `8`, `9`, `10`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
Your guess? Too low.
Your guess? Too low.
Your guess? Too low.
Your guess? Too low.
Your guess? You got it in 5 guesses!
```

## Under the hood

`a random number from 1 to 10` is different each time the program runs. This cookbook's tests fix the random numbers, so the output stays the same.
