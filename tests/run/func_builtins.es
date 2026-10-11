note: built-ins that take a function, as values and as sentences
to double with x:
    give back x times 2.
to sum of x and y:
    give back x plus y.
to even with n:
    give back n mod 2 is 0.
let scores be a list of 40, 75, 12, 90 and 51.
say double applied to every number in scores.
say the numbers in scores where it is more than 50.
say the numbers in scores that pass even.
say scores combined using sum, starting from 0.
say the numbers in scores combined using sum.
let names be a list of "Cyd", "Al", "Barbara" and "Di".
to letters in name:
    give back the number of characters in name.
say names sorted by letters.
say names sorted by letters biggest first.
let bonus be 5.
say (a function taking s that gives back s plus bonus) applied to every score in scores.
apply double to every number in scores.
say scores.
keep only the numbers in scores where it is at least 50.
say scores.
let shared be scores.
keep only the numbers in scores that pass even.
say shared.
sort names by letters, biggest first.
say names.
let words be a list of "a", "bb" and "ccc".
say words combined using (a function taking acc and w that gives back acc followed by w), starting from ">".
say (the function letters) applied to every word in words.

note: stable: equal keys keep their order
let pairs be a list of "b2", "a1", "b1", "a2" and "c".
to first letter of word:
    give back character 1 of word.
say pairs sorted by first letter.
say (a list of 3, 1 and 2) sorted by (a function taking n that gives back 0 minus n).

note: a closure that changes the list it goes through sees it as it was
let growing be a list of 1, 2 and 3.
let grow be a function taking n:
    add n to the end of growing.
    give back yes.
say the numbers in growing that pass grow.
say growing.

note: the list of a record, and lists of text
a team is a kind of thing with:
    players, which is a list of text.
let reds be a new team with players (a list of "ann", "bo" and "cy").
to shout of word:
    give back word in uppercase.
apply shout to every player in reds's players.
keep only the players in reds's players where it is not "BO".
say reds.
let limit be 2.
say the words in (a list of "a", "bbb", "cc") where the number of characters in it is at most limit.
say (the numbers in scores where it is more than 100) combined using sum, starting from 1000.
