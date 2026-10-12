note: "a dog can bark:" defines a method, like "to bark, for a dog:".
a dog has a name (text) and an age (a number).
a cat has a name (text).
something that can speak is anything with:
    a way to speak, giving back text.
a dog can bark:
    say the dog's name, "says woof".
a dog can fetch with a toy:
    say the dog's name, "fetches the", toy.
a dog can speak, giving back text:
    give back "Woof from [the dog's name]".
a cat can speak, giving back text:
    give back "Meow from [the cat's name]".
a dog can have a birthday:
    add 1 to the dog's age.
let rex be a new dog with name "Rex" and age 3.
bark rex.
fetch rex with "ball".
have a birthday for rex.
say rex's age.
let pets be an empty list of things that can speak.
add rex to the end of pets.
add (a new cat with name "Tom") to the end of pets.
for each pet in pets:
    say speak pet.
a shape is one of:
    a circle with a radius (a number).
    a square with a side (a number).
a shape can tell its area, giving back a number:
    check which kind the shape is:
        when it's a circle:
            give back 3 times its radius times its radius.
        when it's a square:
            give back its side times its side.
say tell its area (a new square with side 4).
