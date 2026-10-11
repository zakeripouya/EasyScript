# Methods: functions for a kind of record.
a dog is a kind of thing with:
    a name, which is text.
    an age, which is a number, starting at 1.

note: the record is called like its kind inside ("the dog")
to bark, for a dog:
    say the dog's name followed by " says woof".

to have a birthday for a dog:
    add 1 to the dog's age.

to rename, for a dog, with new name:
    set the dog's name to new name.

to describe, for a dog, giving back text:
    give back the dog's name followed by " (" followed by the dog's age followed by ")"

to swap names, for a dog, with other (a dog) and note:
    let old be the dog's name.
    set the dog's name to other's name.
    set other's name to old.
    say note.

let rex be a new dog with name "Rex" and age 3.
let fido be a new dog with name "Fido".

note: as sentences
bark rex.
bark for fido.
have a birthday for rex.
have a birthday for rex.
rename fido with "Fifi".
call bark with fido.
swap names rex with fido and "swapped".
say describe rex.
say describe for fido.

note: in values
let text be describe of rex.
say text.
say (describe rex) followed by "!".
if describe for fido is "Rex (1)", say "fido has the name Rex now".

note: on records in a list
for each d in list of rex, fido:
    have a birthday for d.
    bark it.
say (describe rex) followed by " and " followed by (describe fido).
note: a method's inputs can be introduced with "to", "from" or "in", and called with the same word
to retitle, for a dog, to new name:
    set the dog's name to new name.
to fetch, for a dog, from place:
    say the dog's name followed by " fetches from " followed by place.
retitle rex to "Rexy".
fetch rex from "the park".
