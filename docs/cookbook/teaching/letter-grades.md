# Give every test result a letter grade

Turn each score into a letter (A for 90 and up, B for 80 and up, and so on) and count the A grades.

## The program

`letter-grades.es`:

```
read the table in "grades.csv" and call it grades.
the letter of a grade depends on its score:
    "A" when it's at least 90.
    "B" when it's at least 80.
    "C" when it's at least 70.
    "D" when it's at least 60.
    "F" otherwise.
for each grade in grades:
    say grade's student, grade's subject, grade's score, grade's letter.
say "Number of A grades:", how many grades have a letter of "A".
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

`easyscript run letter-grades.es` shows:

```
Amira Math 82 B
Amira Science 91 A
Amira English 77 C
Ben Math 45 F
Ben Science 58 F
Ben English 62 D
Chloe Math 95 A
Chloe Science 88 B
Chloe English 93 A
Dev Math 38 F
Dev Science 49 F
Dev English 71 C
Number of A grades: 3
```

## Under the hood

`the letter of a grade depends on its score:` makes a value every grade has, like a field. It's worked out each time it's used, by checking the lines from the top: the first that fits gives the letter.
