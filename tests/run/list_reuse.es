note: values worked out and used once are reused instead of copied; nothing changes for the program
let row be a list of 1 and 2
let grid be a list of row, (a list of 3) and (a list of 4 and 5)
note: a slice of a new list: the lists in it still don't belong to anything else
let top be items 1 to 2 of (grid reversed)
add 9 to the end of item 2 of top
say top
say grid
say row
let cut be (a list of row and row) without the last item
add 7 to the end of item 1 of cut
say cut
say row
let copied be a copy of (a list of row)
add 8 to the end of item 1 of copied
say copied
say row
note: slices of slices, copies of copies, every other item of a new list
say every other item of (items 2 to 6 of (a list of 1, 2, 3, 4, 5, 6 and 7))
say a copy of (a copy of (a list of "a" and "b"))
say the last 2 items in (the first 4 items in (a list of 1, 2, 3, 4 and 5))
let words be a list of "x", "y" and "z"
say words without the first item
say words

note: sorting numbers: in place, and "not a number" (infinity minus infinity) goes last
let huge be 1000000000000
repeat 30 times:
    multiply huge by 1000000000000
let odd be huge minus huge
let mixed be a list of 3, odd, -1, huge, 2, odd and 0
sort mixed
say mixed
sort mixed, biggest first
say mixed
let many be an empty list of numbers
count from 1 to 10000:
    add (it times 7919) mod 10007 to the end of many
sort many
say item 1 of many and " " and item 5000 of many and " " and item 10000 of many
sort many, biggest first
say item 1 of many and " " and item 10000 of many
let same be a list of 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5 and 1
sort same
say the first 2 items in same
