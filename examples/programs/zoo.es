note: An interface: anything that can speak and describe itself can live in
note: the zoo. Dogs, cats and birds never say they're speakers; each just has
note: the methods.

something that can speak is anything with:
    a way to speak, giving back text.
    a way to describe, giving back text.

a dog is a kind of thing with:
    name, which is text.
    tricks, which is a list of text, starting at an empty list of text.
a cat is a kind of thing with:
    name, which is text.
    lives, which is a number, starting at 9.
a bird is one of:
    a parrot with name (text) and words (a list of text).
    a crow.

to speak, for a dog, giving back text:
    give back "Woof!".
to describe, for a dog, giving back text:
    if the dog's tricks is empty, give back the dog's name followed by " the dog".
    give back the dog's name followed by " the dog, who can " followed by (the dog's tricks joined with " and ").

to speak, for a cat, giving back text:
    give back "Meow.".
to describe, for a cat, giving back text:
    give back the cat's name followed by " the cat, with " followed by the cat's lives followed by " lives".

to speak, for a bird, giving back text:
    check which kind the bird is:
        when it's a parrot:
            give back its words joined with ", " followed by "!".
        when it's a crow:
            give back "Caw.".
to describe, for a bird, giving back text:
    check which kind the bird is:
        when it's a parrot:
            give back its name followed by " the parrot".
        when it's a crow:
            give back "a crow".

note: any of them can be given to this
to introduce with animal (something that can speak):
    say (describe animal) followed by " says " followed by (speak animal).

let rex be a new dog with name "Rex".
add "sit" to the end of rex's tricks.
add "roll over" to the end of rex's tricks.

let zoo be an empty list of things that can speak.
add rex to the end of zoo.
add (a new cat with name "Tom") to the end of zoo.
add (a new parrot with name "Polly" and words (a list of "hello" and "pretty bird")) to the end of zoo.
add (a new crow) to the end of zoo.

for each animal in zoo:
    introduce with animal.

note: built-ins work on them too
let loud be the animals in zoo where speak it ends with "!".
say "Loud ones: " followed by the number of items in loud.
let sounds be (a function taking pet that gives back speak pet) applied to every animal in zoo.
say sounds.
