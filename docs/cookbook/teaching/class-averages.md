# Find each student's average and the top student

Your class's test results are in a csv file. Show each student's average score, who has the best average, how many results were below 50, and save a bar chart of the averages.

## The program

`class-averages.es`:

```
read the table in "grades.csv" and call it grades.
say "Average score per student:".
for each student and its average in the average score of grades per student:
    say "  [student]: [average rounded to 1 decimal]".
say "Best average:", the student with the highest average score.
say "Results below 50:", how many grades have a score of less than 50.
save a bar chart of the average score of grades per student as "averages.svg", titled "Average scores".
```

## The data

`grades.csv`, in the same folder:

```
student,subject,score
Amira,Math,82
Amira,Science,91
Amira,English,77
Ben,Math,45
Ben,Science,58
Ben,English,62
Chloe,Math,95
Chloe,Science,88
Chloe,English,93
Dev,Math,38
Dev,Science,49
Dev,English,71
```

## Running it

`easyscript run class-averages.es` shows:

```
Average score per student:
  Amira: 83.3
  Ben: 55
  Chloe: 92
  Dev: 52.7
Best average: Chloe
Results below 50: 3
```

It also makes `averages.svg`.

## Under the hood

`the average score of grades per student` is one pass over the rows that keeps a running total and count for each student in a map, and `the student with the highest average score` works those averages out the same way, then keeps the best one. `easyscript plan class-averages.es` shows what each of them became.
