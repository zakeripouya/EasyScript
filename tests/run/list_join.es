note: "followed by" joins two lists of the same kind into a new list
let morning slots be a list of 9, 10 and 11
let evening slots be a list of 17 and 18
say the morning slots followed by the evening slots
let all slots be morning slots followed by evening slots
add 19 to the end of all slots
say all slots
say morning slots
say (a list of "a") followed by (an empty list of text) followed by (a list of "b" and "c")
say "Slots: " followed by morning slots
let rows be a list of (a list of 1) and (a list of 2)
let more be rows followed by rows
add 5 to the end of item 1 of more
say more
say rows
keep weekdays as a list of "Mon" and "Tue"
say weekdays followed by (a list of "Wed")
say weekdays
note: the kinds can come from how a function is called
to both with first part and second part:
    give back first part followed by second part
say both with (a list of 1) and (a list of 2 and 3)
say length of (both with (a list of 4) and (an empty list of numbers))
note: anything other than two lists still joins as text
say morning slots followed by "!"
say 5 followed by evening slots
