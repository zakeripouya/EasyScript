note: 200,000 rounds of numbers and text that can be nothing: found or not,
note: checked, kept in records, lists and maps, given back, and dropped.
a person is a kind of thing with:
    a nickname, which is text or nothing.
    an age, which is a number or nothing.

to nickname for n, giving back text or nothing:
    if n mod 3 is 0, give back nothing.
    give back "pal " followed by n.

let total be 0.
let p be a new person.
repeat 200000 times:
    let at be the position of "b" in ("ab" followed by it).
    if at is not nothing, add at to total.
    set p's nickname to nickname for it.
    set p's age to the position of "9" in (it as text).
    if p's nickname is not nothing and p's age is not nothing:
        add the number of characters in p's nickname to total.
    check which kind p's age is:
        when it's nothing:
            skip this one.
        otherwise:
            add it to total.
    let pair be a list of at and (p's age).
    if pair is empty, stop the loop.
say total.
