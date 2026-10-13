# A quiz on capital cities

Ask five questions one at a time, say whether each answer is right, and give the score at the end. Capital letters in the answers don't matter.

## The program

`capitals-quiz.es`:

```
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
```

## Running it

When it's run with `easyscript run capitals-quiz.es` and these answers are typed (`paris`, `Tokyo`, `Nairobi`, `toronto`, `Brasilia`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
What is the capital of France? Right!
What is the capital of Japan? Right!
What is the capital of Kenya? Right!
What is the capital of Canada? Not quite. It's Ottawa.
What is the capital of Brazil? Right!
You got 4 out of 5.
Well done!
```
