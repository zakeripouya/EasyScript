note: Defaults: ", starting at" for a map's values, and ", or X if there isn't one".
let counts be an empty map from text to numbers.
for each word in the words of "the cat and the hat and the bat":
    add 1 to the value for word in counts, starting at 0.
say counts.
increase the value for "cat" in counts by 10, starting at 100.
increase the value for "dog" in counts by 10, starting at 100.
multiply the value for "fox" in counts by 3, starting at 2.
say counts.
let groups be an empty map from text to lists of text.
for each word in the words of "apple avocado banana blueberry cherry":
    add word to the value for (the first character of word) in groups, starting at an empty list of text.
for each letter and words in groups:
    say letter, ":", words.
to fresh start:
    say "(working out a start)".
    give back 0.
let tally be a map of "a" to 5.
add 1 to the value for "a" in tally, starting at fresh start.
add 1 to the value for "b" in tally, starting at fresh start.
say tally.
a person has a name (text) and a nickname (text or nothing).
let ann be a new person with name "Ann" and nickname nothing.
let bo be a new person with name "Bo" and nickname "Bobby".
say ann's nickname, or "friend" if there isn't one.
say bo's nickname, or "friend".
say "Hi", (ann's nickname, or ann's name), "!".
let ages be a map of "Ann" to 30.
say the value for "Bob" in ages, or 0 if there isn't one.
let at be the position of "z" in "abc".
say at, or 0.
let found be the position of "b" in "abc".
say found, or 0.
a node has a value (a number) and a next (a node or nothing).
let last be a new node with value 2 and next nothing.
let first be a new node with value 1 and next last.
say (first's next, or first)'s value, (last's next, or first)'s value.
