note: slices: parts of a list, as new lists
let scores be a list of 10, 20, 30, 40, 50 and 60
say items 2 to 4 of scores
say items 3 to 3 in scores
say items 5 to the end of scores
say items 1 to the end in scores
let n be 2
say items n to (n plus 1) of scores
say the first 3 items in scores
say the last 2 items of scores
say the first n items in scores
say the first 0 items in scores
say "[" followed by the last 0 items in scores followed by "]"
say the first 10 items in scores
say the last 99 items in scores
say scores without the first item
say scores without the last item
say every second item of scores
say every third item of scores
say every 4th item in scores
say every seventh item of scores
say every 1st item of scores

note: slices of slices, and slices in other values
say (items 2 to 5 of scores) without the first item
say every second item of (the first 5 items in scores)
say (the last 3 items in scores) added together
say the length of every second item of scores
for each score in items 2 to 3 of scores:
    say "score " followed by score
say every s in (the first 2 items in scores) times 10

note: a slice is a new list: changing it doesn't change the original
let part be items 1 to 2 of scores
add 99 to the end of part
set item 1 of part to 0
say part
say scores
let rows be a list of (a list of 1 and 2), (a list of 3) and (a list of 4 and 5)
let top be the first 2 items in rows
add 9 to the end of item 1 of top
say top
say rows
let names be a list of "Ann", "Bob" and "Cy"
say names without the last item
say every second item of names

note: empty lists and short lists
let empty_list be an empty list of numbers
say length of (the first 3 items in empty_list)
say length of (the last 3 items in empty_list)
say length of (every second item of empty_list)
let one be a list of 7
say one without the first item is empty
say every second item of one is empty
say items 1 to 1 of one

note: "every other item" starts at the first item; "every second item" at the second
say every other item of scores
say every other item in names
say length of (every other item of empty_list)
say every other item of one
