note: 200,000 rounds of part 3's forms with text: adjectives, values, named values,
note: tables, rules, defaults, "remove the ... that ..." and "every ...'s".
a pet has a name (text), a kind (text or nothing) and some toys (a list of text).
a pet is spoiled if the number of items in its toys is at least 2.
a pet's label is its name followed by "!".
the favourite pet is the one with the most toys.
a pet's name must not be empty.
the sound of a pet depends on its name:
    "woof" when it starts with "r".
    "meow" otherwise.
let total be 0.
repeat 200000 times:
    let round be it.
    let pets be a list of (a new pet with name "rex " followed by round, kind "dog" and toys (a list of "ball", "bone")), (a new pet with name "tom", kind nothing and toys (a list of "yarn")).
    let counts be an empty map from text to numbers.
    for each pet in pets:
        add 1 to the value for (pet's kind, or "unknown") in counts, starting at 0.
    if any pet is spoiled, add 1 to total.
    let best be the favourite pet's label.
    let sounds be how many pets whose sound is "woof".
    set every pet's toys to a list of "stick".
    remove the pets that are spoiled.
    remove the pets whose name starts with "t".
    if round is 199999:
        say counts, best, sounds, the number of items in pets.
say total.
