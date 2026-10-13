let countries be "France", "Japan", "Kenya", "Canada", "Brazil"
let capitals be "Paris", "Tokyo", "Nairobi", "Ottawa", "Brasilia"

let score be 0
for each i from 1 to 5:
    let country be item i of countries
    let capital be item i of capitals
    ask "What is the capital of " + country + "? " and call the answer answer
    if lowercase(answer) is lowercase(capital):
        say "Right!"
        set score to score + 1
    else:
        say "Wrong! The answer is " + capital + "."

say "Your score: " + score + " out of 5"
if score >= 4:
    say "Well done!"
