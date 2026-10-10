# Text taken apart, searched, changed and built 100,000 times: everything
# each round makes is freed when nothing holds it, so memory stays flat.
let found be 0
let built be ""
repeat 100000 times:
    let line be "  Round " followed by it followed by ": café, crème and naïve  "
    let words be the words of line
    let parts be (line without extra spaces) split by ", "
    let shouted be line in uppercase
    let quiet be shouted in lowercase
    let swapped be line with "é" replaced by "e"
    let letters be the characters of (item 2 of words)
    let first part be the first 5 characters of line
    let tail be characters 10 to the end of line
    let joined be parts joined with "|"
    if joined contains "crème", add 1 to found
    if the position of "naïve" in line is 0, add 1 to found
    if character 3 of line is a letter, add 1 to found
    if the code of (character 3 of line) is 82, add 1 to found
    set built to built followed by (the character with code (65 plus it mod 26))
    if length of built is 999, set built to ""
say found
say length of built
