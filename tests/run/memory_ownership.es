# Every way a text value can be shared, handed over, or replaced. Run with
# the memory check on, so anything left alive at the end fails the test.
keep greeting as "Hello"

to shout with words:
    set words to words followed by "!"
    give back words

to same with thing:
    give back thing

to describe with a_text and b_text:
    if a_text is b_text, give back "same"
    give back a_text followed by " and " followed by b_text

let t be greeting followed by ", " followed by "world"
let u be t
set t to "replaced"
say u
say t
say shout with u
say u
say same with same with shout with t
say describe of t and u
say describe of t and "replaced"
say greeting and " again"
if u is at least "Hello", say "in order"
let n be length of (u followed by u)
say n
let copy be same with greeting
say copy
write u to file "out.txt"
append t followed by "?" to file "out" followed by ".txt"
read file "out.txt" and call it back
say back
