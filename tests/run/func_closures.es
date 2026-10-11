note: functions made inside values use the names around them, shared, not copied

to make counter, giving back a function giving back a number:
    let count be 0.
    give back a function giving back a number:
        add 1 to count.
        give back count.

let next be call make counter.
let other be call make counter.
say call next.
say call next.
say call other.
say call next.

note: two functions sharing one name
to make account with balance (a number), giving back a list of functions taking a number and giving back a number:
    let deposit be a function taking amount that gives back balance plus amount.
    let change be a function taking amount:
        add amount to balance.
        give back balance.
    give back a list of deposit and change.
let account be call make account with 100.
say call item 2 of account with 50.
say call item 1 of account with 0.
say call item 2 of account with -30.

note: the main program's names, changed from both sides
let total be 0.
let log be "".
let record be a function taking amount:
    add amount to total.
    set log to log followed by amount followed by " ".
call record with 5.
call record with 7.
set total to total plus 100.
call record with 1.
say total.
say log.

note: each round of a loop keeps its own number and its own names
let makers be an empty list of functions giving back text.
count from 1 to 3 as k:
    let label be "round " followed by k.
    add (a function that gives back label followed by " of " followed by k) to the end of makers.
for each maker in makers:
    say call maker.
let items be a list of "a", "b" and "c".
let shown be an empty list of functions giving back text.
for each letter in items:
    add (a function that gives back letter in uppercase) to the end of shown.
for each f in shown:
    say call f.

note: functions inside functions inside functions
to make adder with amount, giving back a function taking a number and giving back a function taking a number and giving back a number:
    give back a function taking x that gives back (a function taking y that gives back x plus y plus amount).
let adder be call make adder with 100.
let add ten be call adder with 10.
say call add ten with 1.
say call add ten with 2.

note: a list a function changes
let seen be an empty list of text.
let remember be a function taking word:
    add word to the end of seen.
call remember with "one".
call remember with "two".
say seen.
