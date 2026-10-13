let secret be RANDBETWEEN(1, 10)
let guesses be 0
let guess be 0
repeat until guess = secret:
  ask "Guess the number" and call the answer guess
  set guesses to guesses + 1
  if guess > secret, show "Too high"
  if guess < secret, show "Too low"
show "You got it in " plus guesses plus " guesses!"
