set num to random between 1 and 10
ask "guess?" and call the answer guess
if guess > num, say "Too high"
if guess < num, say "Too low"
if guess is num, say "You got it in N guesses!"
