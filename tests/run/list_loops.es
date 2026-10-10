note: going through lists: for each, every, only
let scores be a list of 72, 95, 64 and 88
for each score in scores:
    say "score: " followed by score
for each name in a list of "Ann" and "Bob":
    say "Hello, " followed by it followed by "!"

note: stop and skip work as in other loops
for each score in scores:
    if score is less than 70, skip this one
    if score is 88, stop the loop
    say score
note: the item's name can be made again by the next loop
for each score in scores:
    let label be score as text followed by "%"
    if score is at least 90, say label

note: a loop goes through the items the list had when it started
let growing be a list of 1, 2 and 3
for each n in growing:
    add n times 10 to the end of growing
say growing
let shrinking be a list of 1, 2, 3 and 4
for each n in shrinking:
    if shrinking is not empty, remove the first item from shrinking
    say n
say "left: " followed by shrinking

note: loops inside loops, and "it" is the innermost item
let rows be a list of (a list of 1 and 2) and (a list of 3, 4 and 5)
for each row in rows:
    let total be 0
    for each cell in row:
        set total to total plus it
    say "row of " followed by length of row followed by ": " followed by total

note: every: the rest of the value, worked out for each item
say every score in scores plus 5
say every score in scores is at least 70
say every s in scores as text followed by "!"
let bonus be 3
say every score in scores plus bonus times 2
say every name in (a list of "ab", "c" and "def") followed by "?"
note: without parentheses, "followed by" would be part of the list's last item
say every name in a list of "ab", "c" and "def" followed by "?"
say every r in rows added together
say every r in rows reversed
say every n in (a list of 1, 2 and 3) times it
say (every n in (a list of 1 and 2) plus 1) added together

note: only: the items for which the comparison is yes
say only the scores that are at least 70
say only the scores that are less than 70
say only the scores that are not 95
say only the scores that aren't equal to 64
say only the scores that are 88
say only the scores that are greater than bonus times 30
let words be a list of "", "fig", "" and "kiwi"
say only the words that aren't empty
say length of (only the words that are empty)
say only the rows that are not empty
say only (every score in scores minus 60) that are above 10
let limit be 80
let high be only the scores that are over limit
add 1 to the end of high
say high
say scores

note: the item's name and the variables it uses
to curve of marks and amount:
    give back every mark in marks plus amount
say curve of scores and 10
to passing with marks (a list of numbers) and pass_mark (a number):
    give back only the marks that are at least pass_mark
say passing with scores and 80
say every row in rows added together plus (every cell in row times bonus) added together
