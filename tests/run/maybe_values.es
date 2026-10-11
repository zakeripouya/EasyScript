note: Numbers, text and yes/no values that can be nothing: where they come
note: from, checking them, giving and storing them, and printing them.

let word be "unbreakable".
let at be the position of "break" in word.
let missing be the position of "x" in word.
say at.
say missing.
say at is missing.
say at is the position of "break" in "unbreakable".

note: Checked, it's a number.
if at is not nothing:
    say at plus 1.
if missing is nothing, say "no x".
if at is not nothing and at is more than 2, say "after the second character".
to first letter of w:
    let where be the position of "a" in w.
    if where is nothing, give back "none".
    give back character where of w.
say first letter of "banana".
say first letter of "kiwi".

note: check which kind, with "when it's nothing".
for each found in a list of at and missing:
    check which kind found is:
        when it's nothing:
            say "not found".
        otherwise:
            say "found at " followed by it.

note: Functions take and give back values that can be nothing.
to half of n (a number or nothing), giving back a number:
    if n is nothing, give back 0.
    give back n divided by 2.
say half of 10.
say half of nothing.
say half of at.
to sign of n, giving back text or nothing:
    if n is 0, give back nothing.
    if n is more than 0, give back "plus".
    give back "minus".
say sign of 5.
say sign of 0.
to decided with answer (yes or no or nothing):
    check which kind answer is:
        when it's nothing:
            say "undecided".
        otherwise:
            if it, say "yes please".
            if not it, say "no thanks".
decided with yes.
decided with nothing.

note: Fields, variables, lists and maps.
a person is a kind of thing with:
    a name, which is text.
    a nickname, which is text or nothing.
    an age, which is a number or nothing.
let ann be a new person with name "Ann" and age 30.
say ann.
set ann's nickname to "Annie".
if ann's nickname is not nothing:
    say "Hi " followed by ann's nickname followed by "!".
if ann's age is not nothing:
    say ann's age plus 1.
set ann's age to nothing.
say ann.
let maybe be the position of "b" in "abc".
set maybe to 7.
say maybe.
set maybe to nothing.
say maybe.
say a list of at and missing.
say a map of "at" to at and "missing" to missing.
