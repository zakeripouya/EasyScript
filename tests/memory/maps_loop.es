# 200,000 rounds making maps that hold text, lists, records and other maps:
# setting, changing, removing, copying, going through and dropping them,
# with live memory kept small.
a pet is a kind of thing with:
    a name, which is text.
    a tags, which is a map from text to numbers, starting at an empty map from text to numbers.

let recent be an empty map from text to numbers.
let groups be an empty map from numbers to lists of text.
let total be 0.
count from 1 to 200000 as n:
    let key be "key " followed by (n mod 50).
    set the value for key in recent to (the value for key in recent, or 0) plus 1.
    if the number of entries in recent is more than 30, remove key from recent.
    set the value for n mod 7 in groups to a list of key and "x".
    add key to the end of the value for n mod 7 in groups.
    let pet be a new pet with name key.
    set the value for "seen" in pet's tags to n.
    let owners be a map of key to pet.
    let nested be a map of "inner" to (a copy of recent).
    let twin be a copy of nested.
    set the value for "extra" in the value for "inner" in twin to 1.
    for each name and count in the value for "inner" in twin:
        add count to total.
    add the number of entries in owners to total.
    add the number of items in the keys of groups to total.
say total.
say the number of entries in recent.
