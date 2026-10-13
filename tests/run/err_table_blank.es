# Using a blank cell directly in arithmetic stops the program with how to
# give it a value.
put the text below in file "tips.csv":
    day,tip
    Monday,$5
    Tuesday,

read the table in "tips.csv" and call it tips.
for each row in tips:
    say row's day, row's tip plus $1.
