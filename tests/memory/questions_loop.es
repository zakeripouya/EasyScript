note: 200,000 rounds of questions about lists of text, records and lists: how many,
note: any, every, totals, the top N, the most common, the one with the highest, per.
a pet has a name (text), a kind (text), an age (a number) and some toys (a list of text).
let checks be 0.
repeat 200000 times:
    let round be it.
    let names be a list of "rex " followed by round, "bo" and "rex " followed by round.
    let pets be a list of (a new pet with name item 1 of names, kind "dog", age round mod 7 and toys (a list of "ball")), (a new pet with name "bo", kind "cat", age 3 and toys (a list of "yarn", "box")).
    if any name in names starts with "rex", add 1 to checks.
    if every pet's age is at least 0, add 1 to checks.
    add how many names start with "b" to checks.
    let common be the 1 most common names.
    let oldest be the pet with the highest age.
    let busiest be the pet with the most toys.
    let top be the top 1 pets by age.
    let per kind be the number of pets per kind.
    let ages be the average age of pets per kind.
    let middle be the median age of pets per name.
    let counted be how many pets whose name contains "rex" there are.
    sort per kind biggest first.
    let sorted ages be ages sorted.
    if round is 199999:
        say common, oldest's name, busiest's name, the number of items in top, per kind, counted.
say checks.
