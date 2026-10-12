note: "depends on" tables: a name made from a table, and a value of a kind.
let total be 70.
the shipping cost depends on the total:
    0 when it's over 100.
    5 when it is over 50.
    10 otherwise.
say the shipping cost.
set total to 150.
say shipping cost.
a drink has a name (text) and a size (text).
the price of a drink depends on its size:
    3 when it's "small".
    4 when it's "medium".
    5 otherwise.
let latte be a new drink with name "latte" and size "medium".
let drinks be a list of latte, (a new drink with name "tea" and size "small") and (a new drink with name "mocha" and size "large").
say the price of latte, latte's price.
say the total price of drinks.
for each drink in drinks:
    say drink's name, "costs", drink's price.
let word be "".
the mood depends on word:
    "quiet" when it's empty.
    "loud" when it's at least "m".
    "calm" otherwise.
say mood.
for each n in a list of 1, 15 and 30:
    the size label depends on n:
        "small" when it's under 10.
        "medium" when it's at most 20.
        "large" otherwise.
    say n, size label.
