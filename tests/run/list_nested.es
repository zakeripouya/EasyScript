note: lists of lists, totals, and functions that work out list kinds
let grid be an empty list of lists of numbers
add a list of 1, 2 and 3 to the end of grid
add (a list of 4) to grid
add an empty list of numbers to the end of grid
say grid
say item 2 of item 1 of grid
add 5 to the end of item 2 of grid
set item 1 of item 1 of grid to 10
insert 0 at position 1 of the last item in grid
say grid
say the length of grid
say every row in grid added together
say grid contains a list of 4 and 5
say grid contains a list of 5 and 4
say only the grid that are not empty
sort grid by their length, longest first
say grid
remove the last item from grid
say grid is a list of (a list of 10, 2 and 3) and (a list of 4 and 5)
let cube be a list of (a list of (a list of 1 and 2) and (a list of 3)) and (a list of (a list of 4))
say cube
say item 1 of item 1 of item 1 of cube
let words be an empty list of lists of text
add a list of "a" and "b" to words
say words

note: totals
let prices be a list of 2.5, 10 and 0.1
say prices added together
say the biggest of prices
say the smallest of prices
say the biggest of a list of "pear", "apple" and "fig"
say the smallest of a list of "pear", "apple" and "fig"
say the biggest of (a list of -3, -1 and -2)
say (every p in prices times 2) added together
say (an empty list of numbers) added together
let ties be a list of 3, 1, 3 and 1
say the biggest of ties followed by " " followed by the smallest of ties

note: kinds of lists worked out from how functions use them and what they're given
to total of marks:
    let sum be 0
    for each mark in marks:
        set sum to sum plus mark
    give back sum
say total of prices
to first_of with things:
    give back the first item in things
say first_of with a list of "x" and "y"
to pair with left and right:
    give back a list of left and right
say pair with "a" and "b"
to longest with texts:
    sort texts by their length, longest first
    give back item 1 of texts
let names be a list of "Al", "Beatrice" and "Cy"
say longest with names
say names
to counts with rows:
    give back every row in rows reversed
say counts with grid
