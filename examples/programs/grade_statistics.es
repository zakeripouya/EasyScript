note: Statistics for one class's test scores, using lists.

let names be a list of "Ana", "Ben", "Cleo", "Dev", "Eli", "Fay", "Gus" and "Hal"
let scores be a list of 72, 95, 64, 88, 79, 91, 55 and 83

to average of marks:
    give back marks added together divided by the length of marks

to letter of mark:
    if mark is at least 90, give back "A"
    if mark is at least 80, give back "B"
    if mark is at least 70, give back "C"
    if mark is at least 60, give back "D"
    give back "F"

say "Scores: " followed by scores
say "Average: " followed by average of scores
say "Highest: " followed by the biggest of scores
say "Lowest: " followed by the smallest of scores

note: the median is the middle of the sorted scores (a copy, so the
note: original order stays the same as the names)
let ordered be a copy of scores
sort ordered
let middle be the length of ordered divided by 2
say "Median: " followed by (item middle of ordered plus item (middle plus 1) of ordered) divided by 2
say "Top three: " followed by (the last 3 items in ordered) reversed
say "Middle half: " followed by items 3 to 6 of ordered

let passing be only the scores that are at least 60
say "Passed: " followed by the length of passing followed by " of " followed by the length of scores
say "Curved by 5: " followed by (every score in scores plus 5)

let letters be an empty list of text
for each score in scores:
    add letter of score to letters
say ""
count from 1 to the length of names as n:
    say item n of names followed by ": " followed by item n of scores followed by " (" followed by item n of letters followed by ")"

let honours be an empty list of text
count from 1 to the length of names as n:
    if item n of letters is "A", add item n of names to honours
say ""
say "Honours: " followed by honours

note: a fixed list ("keep"): the grades, which never change
keep all grades as a list of "A", "B", "C", "D" and "F"
say ""
for each grade in all grades:
    say grade followed by ": " followed by length of (only the letters that are grade)
