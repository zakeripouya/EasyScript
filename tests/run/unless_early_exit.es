note: "unless" runs its sentences when the condition is no; "give back X unless C" and "stop here if C" leave a function early.
a node is a kind of thing with:
    a value, which is a number.
    a next, which is a node or nothing.

to second value of start (a node), giving back a number:
    give back 0 unless start's next is not nothing.
    give back start's next's value.

to warn with amount:
    stop here if amount is at most 100.
    say "That's a lot: " followed by amount.

to sign of x:
    give back -1 if x is below 0.
    give back 1 unless x is 0.
    give back 0.

let one be a new node with value 1 and next nothing.
let two be a new node with value 2 and next one.
say second value of two.
say second value of one.
warn with 50.
warn with 500.
say (sign of -3) followed by (sign of 0) followed by sign of 9.
let ready be no.
unless ready:
    say "not ready".
unless ready, say "still not ready".
unless ready then say "and again".
to first word of line (text), giving back text:
    give back "" if line is empty.
    give back item 1 of the words of line.
say "[" followed by (first word of "") followed by "]".
say first word of "hello there".
to count up with limit:
    count from 1 to 10:
        stop here if it is above limit.
        say it.
count up with 3.
