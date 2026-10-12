note: every way to change a list; the list itself changes
let scores be a list of 72, 95 and 88
add 60 to the end of scores
say scores
insert 7 at position 1 of scores
insert 8 at position 3 in scores
insert 9 at position 7 of scores
say scores
remove item 2 from scores
say scores
remove the first item from scores
remove the last item from scores
say scores
set item 1 of scores to 100
set the first item in scores to item 1 of scores plus 1
set the last item of scores to 0
change item 2 of scores to 50
say scores

note: "add X to Y": a list of text can only mean adding to its end
let names be a list of "Cy"
add "Ann" to names
add "Bob" to the end of names
say names
let flags be an empty list of yes/no values
add yes to flags
add 1 is 2 to flags
say flags

note: an empty list grows
let squares be an empty list of numbers
count from 1 to 5:
    add it times it to the end of squares
say squares
remove item 1 from squares
remove item 4 from squares
say squares

note: sorting changes the list; equal items keep their order
let numbers be a list of 5, 3, 9, 1, 3 and -2
sort numbers
say numbers
sort numbers, biggest first
say numbers
sort numbers smallest first
say numbers
let words be a list of "pear", "Fig", "apple", "fig" and "banana"
sort words
say words
sort words, biggest first
say words
let fruit be a list of "kiwi", "fig", "plum", "banana", "pear" and "date"
sort fruit by their length
say fruit
sort fruit by their length, longest first
say fruit
sort fruit by their length shortest first
say fruit
let empty_list be an empty list of numbers
sort empty_list
say "sorted nothing: " followed by empty_list
let many be an empty list of numbers
count down from 100 to 1:
    add (it times 37) mod 101 to the end of many
sort many
say item 1 of many followed by " " followed by item 50 of many followed by " " followed by item 100 of many

note: reversed makes a new list; the old one doesn't change
let order be a list of 1, 2 and 3
let backwards be order reversed
say backwards
say order
say (a list of "a", "b" and "c") reversed
