note: 200,000 rounds of functions kept as values: made with names they share,
note: kept in lists, maps and records, called, replaced and dropped.
a button is a kind of thing with:
    label, which is text.
    action, which is a function taking text and giving back text.

to make counter with start (text), giving back a function giving back text:
    let count be 0.
    let label be start.
    give back a function giving back text:
        add 1 to count.
        set label to start followed by count.
        give back label.

let total be 0.
let last be "".
let kept be an empty list of functions taking a number and giving back a number.
let named be an empty map from text to functions giving back text.
let b be a new button with label "go" and action (a function taking s that gives back s).
repeat 200000 times:
    let round be it.
    let tag be "round " followed by round.
    let next be call make counter with tag.
    set last to call next.
    set last to call next.
    let scale be a function taking n that gives back n times round.
    add scale to the end of kept.
    if the number of items in kept is more than 3, remove the first item from kept.
    set the value for tag in named to next.
    if the number of entries in named is more than 2, remove (item 1 of the keys of named) from named.
    set b's action to a function taking s that gives back s followed by tag.
    set last to call b's action with last.
    add call (last item of kept) with 1 to total.
say total.
say last.
