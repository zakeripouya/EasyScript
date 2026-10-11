note: maps of lists, lists of maps, maps of maps, records and maps, maps through functions

note: a map from text to lists: the lists belong to the map
let graph be an empty map from text to lists of text.
set the value for "A" in graph to a list of "B" and "C".
set the value for "B" in graph to an empty list of text.
add "D" to the end of the value for "B" in graph.
add "E" to the value for "B" in graph.
say graph
say item 2 of the value for "A" in graph
let neighbours be the value for "A" in graph
add "Z" to the end of neighbours
say graph
say the number of items in the value for "A" in graph

note: a copy of a map copies its lists too
let backup be a copy of graph
add "Q" to the end of the value for "B" in graph
say backup
say graph
say the values of graph

note: maps of maps
let grades be an empty map from text to maps from text to numbers.
set the value for "Ada" in grades to a map of "math" to 95.
set the value for "history" in the value for "Ada" in grades to 88.
add 1 to the value for "math" in the value for "Ada" in grades.
say grades
say the value for "math" in the value for "Ada" in grades
let nested copy be a copy of grades
set the value for "math" in the value for "Ada" in nested copy to 0
say grades
say nested copy

note: lists of maps
let rows be a list of (a map of "a" to 1) and (a map of "b" to 2).
say rows
say rows contains (a map of "b" to 2)
say rows contains (a map of "b" to 3)
say the position of (a map of "b" to 2) in rows
add (a map of "c" to 3) to the end of rows
set the value for "a" in item 1 of rows to 10
say rows
let first two be the first 2 items in rows
set the value for "a" in item 1 of first two to 99
say item 1 of rows
say first two

note: records holding maps, and maps holding records
a student is a kind of thing with:
    a name, which is text.
    a scores, which is a map from text to numbers, starting at an empty map from text to numbers.
let ada be a new student with name "Ada"
set the value for "math" in ada's scores to 95.
let grace be a new student with name "Grace" and scores (a map of "logic" to 99)
say ada
say grace
let class be an empty map from text to students.
set the value for "ada" in class to ada.
set the value for "grace" in class to grace.
set the value for "physics" in (the value for "ada" in class)'s scores to 90.
say ada's scores
let class copy be a copy of class
set (the value for "ada" in class copy)'s name to "Ada L."
say ada's name
for each key and person in class:
    say key followed by ": " followed by person's name

note: maps through functions, with kinds worked out from use
to tally with words, giving back a map from text to numbers:
    let counts be an empty map from text to numbers
    for each word in words:
        set the value for word in counts to (the value for word in counts, or 0) plus 1
    give back counts
let counts be tally with the words of "the cat and the hat and the bat"
say counts
to total of m:
    let sum be 0
    for each k and v in m:
        add v to sum
    give back sum
say total of counts
to bump with m and key:
    if m has key:
        add 1 to the value for key in m
    otherwise:
        set the value for key in m to 1
bump with counts and "cat"
bump with counts and "dog"
say counts
to described with places (a map from numbers to text):
    give back the number of entries in places
say described with an empty map from numbers to text

note: every and only with maps
say every name in the keys of counts in uppercase
say only (the values of counts) that are greater than 1
let limit be 2
say every v in the values of counts plus limit

note: a field holding a list of maps, starting empty (each record gets its own list)
a ledger is a kind of thing with:
    an entries, which is a list of maps from text to numbers, starting at an empty list of maps from text to numbers.
let first ledger be a new ledger
let second ledger be a new ledger
add (a map of "paid" to 10) to the end of first ledger's entries
say first ledger
say second ledger
let shelf be an empty list of maps from numbers to text
add (a map of 1 to "one") to the end of shelf
say shelf
