let capitals be a map of "France" to "Paris", "Japan" to "Tokyo", "Kenya" to "Nairobi", "Canada" to "Ottawa" and "Brazil" to "Brasilia".
let score be 0.
for each country and its capital in capitals:
    ask "What is the capital of [country]? " and call it answer.
    if answer in lowercase is capital in lowercase:
        say "Right!"
        add 1 to score.
    otherwise:
        say "Not quite. It's [capital]."
say "You got [score] out of 5."
if score is at least 4, say "Well done!"
