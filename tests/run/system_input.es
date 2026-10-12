note: "ask" reads a line, then "the text sent to the program" is everything left
ask "Name? " and call it name
say "Hello, " followed by name
let rest be the text sent to the program
say "The rest has " followed by the number of characters in rest followed by " characters."
say the lines sent to the program
for each line in the lines sent to the program:
    say "[" followed by line followed by "]"
say the number of items in the words of the text sent to the program
ask "Anything else? " and call it more
say "more is empty: " followed by (more is empty)
