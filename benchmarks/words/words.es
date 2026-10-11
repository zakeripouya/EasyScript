note: Counting words: two million words picked from a vocabulary of 20,000
note: ("w0" to "w19999"; the same picks in every language: x times 16807 mod
note: 2147483647 each time, word number x mod 20000), counted with a map from
note: each word to how many times it appears.
let vocabulary be an empty list of text
count from 0 to 19999 as n:
    add "w" followed by n to the end of vocabulary
let counts be an empty map from text to numbers
let x be 42
repeat 2000000 times:
    set x to x times 16807 mod 2147483647
    let word be item (x mod 20000 plus 1) of vocabulary
    set the value for word in counts to (the value for word in counts, or 0) plus 1
say the number of entries in counts and " " and the value for "w0" in counts and " " and the value for "w7" in counts and " " and the value for "w19999" in counts
