note: A gradebook: questions about a class, with no loops that count by hand.

a student has a name, a class and a score.

let students be a list of a new student with name "Ana", class "Math", score 72,
    a new student with name "Ben", class "Math", score 95,
    a new student with name "Cleo", class "Art", score 64,
    a new student with name "Dev", class "Art", score 88,
    a new student with name "Eli", class "Math", score 51 and
    a new student with name "Fay", class "Art", score 91.

say "Students:", how many students there are.
say "Passed:", how many students have a score of at least 60.
say "Average:", the average score of students rounded to 1 decimal place.
say "Middle score:", the median score of students.
say "Best:", the name of the student with the highest score.

if any student's score is above 90, say "Someone got an A+.".
if every student's score is at least 50, say "Nobody is below 50.".
if no student's score is above 100, say "All scores look right.".

say "Top three:".
for each student in the top 3 students by score:
    say "  ", student's name, student's score.

say "Average per class:".
for each class and average in the average score of students per class, sorted biggest first:
    say "  ", class, average rounded to 1 decimal place.
