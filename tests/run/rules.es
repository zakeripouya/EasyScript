note: "must" rules, checked whenever a field gets a value: in "new", "set", and the change sentences.
a person has a name (text), an age (a number), an email (text) and some pets (a list of text).
a dog has a name (text) and an age (a number).
an age must be between 0 and 150.
a person's email must contain "@".
a person's name must not be empty.
a person's pets must not contain "dragon".
keep oldest as 30.
a dog's age must be at most oldest.
let ann be a new person with name "Ann", age 30, email "ann@x.com" and pets (a list of "cat").
add 5 to ann's age.
subtract 35 from ann's age.
set ann's email to "ann@y.com".
add "dog" to the end of ann's pets.
let rex be a new dog with name "Rex" and age 12.
increase rex's age by 18.
say ann, rex.
