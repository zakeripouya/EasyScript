secret = random number between 1 and 10
guesses = 0
repeat:
  ask "Guess the number" and store in guess
  guesses = guesses + 1
  if guess > secret then show "Too high"
  if guess < secret then show "Too low"
until guess = secret
show "You got it in " & guesses & " guesses!"
