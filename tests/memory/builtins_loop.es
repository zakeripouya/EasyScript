note: 200,000 rounds of the built-ins that take a function, with text and
note: lists of lists, as values and as sentences, through "to" functions and closures.
to label of n:
    give back "item " followed by n.
to joined of so far and word:
    give back so far followed by word.
to long enough with word:
    give back the number of characters in word is at least 6.

let total be 0.
let last be "".
repeat 200000 times:
    let round be it.
    let numbers be a list of round, round plus 1 and round plus 2.
    let labels be label applied to every number in numbers.
    let tagged be (a function taking n that gives back label of (n plus round)) applied to every number in numbers.
    keep only the words in labels that pass long enough.
    let groups be a list of (a list of "b", "a"), (a list of "c").
    sort groups by (a function taking g that gives back the number of items in g).
    apply (a function taking g that gives back g followed by (a list of "z")) to every group in groups.
    set last to (tagged sorted by (a function taking t that gives back t) biggest first) combined using joined, starting from "".
    add the number of items in labels plus the number of items in (item 1 of groups) to total.
say total.
say last.
