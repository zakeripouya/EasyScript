a dog is a kind of thing with:
    a name, which is text.
    an age, which is a number, starting at 0.
    a friend, which is a dog or nothing.
    the toys, which is a list of text, starting at an empty list of text.
    a start time, which is a number. a good boy, which is yes or no.

to bark, for a dog:
    say the dog's name.
to have a birthday for a dog:
    add 1 to the dog's age.
to rename, for a dog, with new name (text), giving back a dog:
    set the dog's name to new name.
    give back the dog
to best of dogs (a list of dogs), giving back a dog or nothing:
    give back nothing

let rex be a new dog with name "Rex" and age 3, friend nothing.
let pup be a new dog.
let pair be a new dog with name "A", friend (a new dog with name "B").
say rex's friend's name.
say (item 1 of list of rex)'s start time.
set rex's age to 4.
set it's name to "x".
set (rex's friend)'s age to 1.
add 1 to rex's age.
subtract 1 from rex's age.
increase rex's start time by 2.
bark rex.
bark for rex.
have a birthday for rex.
rename rex with "Max".
say rename for rex with "Max".
say best of list of rex.
if rex's friend is not nothing, say "friend".
say rock'n'roll.
