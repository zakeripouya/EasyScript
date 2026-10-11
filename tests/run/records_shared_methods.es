# Methods with the same name for different kinds of record: the call picks one by the record.
a dog is a kind of thing with:
    name, which is text.
a cat is a kind of thing with:
    name, which is text.
    lives, which is a number, starting at 9.
to speak, for a dog:
    say the dog's name followed by " says woof".
to speak, for a cat:
    say the cat's name followed by " says meow".
to describe, for a dog, giving back text:
    give back "a dog called " followed by the dog's name.
to describe, for a cat, giving back text:
    give back "a cat with " followed by the cat's lives followed by " lives".
to introduce with pet:
    speak pet.
let rex be a new dog with name "Rex".
let tom be a new cat with name "Tom".
speak rex.
speak tom.
say describe rex.
say describe for tom.
introduce with rex.
for each d in a list of rex and (a new dog with name "Fido"):
    speak d.
note: a record or nothing, checked first, picks by its kind too
an owner is a kind of thing with:
    a pet, which is a cat or nothing.
let ann be a new owner.
set ann's pet to tom.
if ann's pet is not nothing, speak ann's pet.
say describe for (a copy of tom)
