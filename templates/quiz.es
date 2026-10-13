note: A quiz. Change the questions and answers in the map below.
let questions be a map of "What is 2 plus 2?" to "4", "What color is the sky on a clear day?" to "blue" and "How many legs does a spider have?" to "8".
let score be 0.
for each question and its answer in questions:
    ask "[question] " and call it reply.
    if reply in lowercase is answer:
        say "Right!"
        add 1 to score.
    otherwise:
        say "Not quite: it's [answer]."
say "You got [score] out of [the number of entries in questions]."
