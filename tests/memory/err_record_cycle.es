# Records that point to each other in a circle are never freed: the memory
# check at the end finds the circle and explains it.
a person is a kind of thing with:
    a name, which is text.
    a friend, which is a person or nothing.

let ann be a new person with name "Ann".
let bo be a new person with name "Bo" and friend ann.
let cy be a new person with name "Cy" and friend bo.
let dee be a new person with name "Dee" and friend cy.
set ann's friend to dee.
let eve be a new person with name "Eve" and friend ann.
say "made a circle of four".
