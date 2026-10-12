note: Statistics for one class's test scores, using lists.

let names be a list of "Ana", "Ben", "Cleo", "Dev", "Eli", "Fay", "Gus" and "Hal".
let scores be a list of 72, 95, 64, 88, 79, 91, 55 and 83.

to average of marks:
    give back marks added together divided by the length of marks.

to letter of mark:
    give back "A" if mark is at least 90.
    give back "B" if mark is at least 80.
    give back "C" if mark is at least 70.
    give back "D" if mark is at least 60.
    give back "F".

say "Scores:", scores.
say "Average:", average of scores.
say "Highest:", the biggest of scores.
say "Lowest:", the smallest of scores.

note: the median is the middle of the sorted scores (a copy, so the
note: original order stays the same as the names)
let ordered be scores sorted.
let middle be the length of ordered divided by 2.
say "Median:", (item middle of ordered plus item (middle plus 1) of ordered) divided by 2.
say "Top three:", (the last 3 items in ordered) reversed.
say "Middle half:", items 3 to 6 of ordered.

let passing be the scores that are at least 60.
say "Passed: [the length of passing] of [the length of scores]".
say "Curved by 5:", (each score in scores plus 5).

let letters be letter applied to every score in scores.
say "".
count from 1 to the length of names as n:
    say "[item n of names]: [item n of scores] ([item n of letters])".

let honours be an empty list of text.
count from 1 to the length of names as n:
    if item n of letters is "A", add item n of names to honours.
say "".
say "Honours:", honours.

note: a fixed list ("keep"): the grades, which never change
keep all grades as a list of "A", "B", "C", "D" and "F".
say "".
for each grade in all grades:
    say "[grade]: [the length of (the letters that are grade)]".
