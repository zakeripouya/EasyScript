let countries be a list of "France", "Japan", "Kenya", "Canada" and "Brazil"
let capitals be a list of "Paris", "Tokyo", "Nairobi", "Ottawa" and "Brasilia"

let score be 0
for each i from 1 to 5:
    let country be item i of countries
    let capital be item i of capitals
    ask "What is the capital of " + country + "? " and call the answer answer
    if lowercase of answer is lowercase of capital:
        say "Right!"
        set score to score + 1
    else:
        say "Wrong! The answer is " + capital + "."

say "Your score: " + score + " out of 5"
if score >= 4:
    say "Well done!"
