# Lists made, changed, and dropped 200,000 times: each round's lists are
# freed when nothing holds them, so memory stays flat.
let kept be an empty list of numbers
let total be 0
repeat 200000 times:
    let row be a list of it, it plus 1 and it plus 2
    let words be a list of "w" followed by it as text and "x"
    add it to the end of row
    insert 0 at position 1 of row
    remove the last item from row
    set item 2 of row to item 3 of row
    set total to total plus (row added together)
    let doubled be every n in row times 2
    let big be only the doubled that are at least 4
    let both be a list of row and doubled
    if length of words is 2, set total to total plus the length of item 1 of both
    let label be words as text followed by (both reversed) as text
    sort row, biggest first
    for each w in words:
        if w is "x", skip this one
        if length of w is 3, stop the loop
    if it mod 1000 is 0:
        add it to the end of kept
        set kept to a copy of kept
say total
say the length of kept
say the last item in kept
