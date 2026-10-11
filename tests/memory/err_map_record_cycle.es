# Records that keep each other alive through a map are a circle too: the
# memory check at the end follows maps (and lists in them) to find it.
a person is a kind of thing with:
    a name, which is text.
    a friends, which is a map from text to lists of persons, starting at an empty map from text to lists of persons.

let ann be a new person with name "Ann".
let bo be a new person with name "Bo".
set the value for "close" in ann's friends to a list of bo.
set the value for "close" in bo's friends to a list of ann.
let cy be a new person with name "Cy".
set the value for "close" in cy's friends to a list of ann.
set cy to a new person with name "Dee".
say "made a circle of two".
