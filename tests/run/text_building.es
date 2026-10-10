note: Adding to text in place: only text nobody else holds grows in place,
note: so every other holder still sees the text it had.
let story be "Once"
let copy be story
set story to story followed by " upon" followed by " a time"
say story
say copy
set story to story and ", the end"
say story
note: The added part reads the text itself: worked out before anything changes.
let echo be "ab"
set echo to echo followed by echo
say echo
set echo to echo followed by "-" followed by echo
say echo
note: Numbers and lists are turned into text, as with any join.
let counted be "n="
set counted to counted followed by 3 followed by " " followed by (a list of 1 and 2)
say counted
note: Building text in a loop, then sharing it.
let row be ""
let rows be an empty list of text
repeat 3 times:
    set row to row followed by it
    add row to the end of rows
say rows
note: Inside a function, an input is held by the caller too.
to stretch with base:
    set base to base followed by "!"
    set base to base followed by "?"
    give back base
let start be "hey"
say stretch with start
say start
note: Characters are counted correctly after growing.
let accents be "é"
repeat 3 times:
    set accents to accents followed by "è"
say length of accents
say character 3 of accents
note: Kinds worked out from text tools.
to shout with something:
    give back something in uppercase
to has seven with things:
    give back things contains 7
say shout with "quiet"
say has seven with a list of 1, 7 and 9
