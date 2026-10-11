note: a map with many entries: the index grows, removed keys leave room, and the order stays
let squares be an empty map from numbers to numbers.
count from 1 to 20000 as n:
    set the value for n in squares to n times n
say the number of entries in squares
say the value for 12345 in squares
count from 1 to 20000 as n:
    if n mod 3 is not 0, remove n from squares
say the number of entries in squares
say the first 5 items in the keys of squares
say the last 3 items in the keys of squares
count from 1 to 10 as n:
    set the value for n in squares to 0
say the last 3 items in the keys of squares
let total be 0
for each n and square in squares:
    add square to total
say total

note: text keys, made one at a time
let words be an empty map from text to numbers.
count from 1 to 5000 as n:
    set the value for "w" followed by n in words to n
say the number of entries in words
say the value for "w4321" in words
count from 1 to 5000 as n:
    if n is more than 3, remove "w" followed by n from words
say words
