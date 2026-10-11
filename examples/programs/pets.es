note: Two kinds of record with methods of the same name. Each call picks
note: the method for the kind of record it's given.

a dog is a kind of thing with:
    a name, which is text.
    a trick, which is text, starting at "sit".

a cat is a kind of thing with:
    a name, which is text.
    a mood, which is text, starting at "sleepy".

to speak, for a dog:
    say the dog's name followed by " says woof".

to speak, for a cat:
    say the cat's name followed by " says meow".

to describe, for a dog, giving back text:
    give back the dog's name followed by " can " followed by the dog's trick

to describe, for a cat, giving back text:
    give back the cat's name followed by " is " followed by the cat's mood

let dogs be a list of (a new dog with name "Rex"), (a new dog with name "Fido" and trick "roll over")
let cat be a new cat with name "Tom"

for each dog in dogs:
    speak dog.
    say describe dog.
speak cat.
say describe cat.
