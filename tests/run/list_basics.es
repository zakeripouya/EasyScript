note: writing lists, looking at their items, and asking about them
let scores be a list of 72, 95 and 88.
let names be a list of "Ann" and "Bob"
let flags be a list of yes, no and yes
let one be a list of 42
let nothing_yet be an empty list of numbers
let no_names be an empty list of text
let no_flags be an empty list of yes/no values
say scores
say names
say flags
say one
say "[" followed by nothing_yet followed by "]"
note: items are worked out like any value
let base be 10
say a list of base, base plus 1, base times 2 and (base minus 1) times 2
say a list of 1, 2, 3, 4
say a list of 1 and 2 and 3

note: items by position, from 1
say item 1 of scores
say item 3 in scores
let n be 2
say item n of scores
say item (n plus 1) of scores
say the first item in names
say the last item of names
say first item of flags
count from 1 to 3:
    say item it of scores

note: how many
say the length of scores
say the number of items in names
say length of nothing_yet
say length of one plus 1

note: questions
say scores contains 95
say scores contains 96
say scores does not contain 96
say scores doesn't contain 95
say names contains "Bob"
say names contains "bob"
say flags contains no
say scores is empty
say nothing_yet is empty
say scores is not empty
say no_names isn't empty
say "" is empty
say "x" is empty
if no_flags is empty, say "no flags yet"
if scores contains 72 and not names is empty, say "both"

note: comparing whole lists, item by item
say scores is a list of 72, 95 and 88
say scores is a list of 72, 88 and 95
say scores is not a list of 72
say nothing_yet is an empty list of numbers
say a list of 0.1 plus 0.2 is a list of 0.3

note: lists as text
say "Scores: " followed by scores
let line be scores as text
say line
say length of line
say "Names: " and names
write names to file "names.txt"
say contents of file "names.txt"
