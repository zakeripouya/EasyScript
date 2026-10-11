# Records in lists, and lists in records.
a player is a kind of thing with:
    a name, which is text.
    the scores, which is a list of numbers, starting at an empty list of numbers.

a team is a kind of thing with:
    a name, which is text.
    the players, which is a list of players, starting at an empty list of players.
    a captain, which is a player or nothing.
    the rounds, which is a list of lists of numbers, starting at an empty list of lists of numbers.

let ann be a new player with name "Ann".
let bo be a new player with name "Bo" and scores (list of 3, 4).
say ann.
say bo.

note: lists in records change in place
add 7 to the end of ann's scores.
add 9 to the end of ann's scores.
insert 1 at position 1 of ann's scores.
set item 2 of ann's scores to 8.
remove the last item from ann's scores.
sort ann's scores, biggest first.
say ann's scores.
say ann's scores added together.
say the biggest of ann's scores.
say the number of items in ann's scores.
let kept be ann's scores.
add 100 to the end of kept.
say ann's scores.

note: every record starts with its own list
let cy be a new player with name "Cy".
add 5 to the end of cy's scores.
let dee be a new player with name "Dee".
say dee's scores is empty.

note: records in lists are shared, not copied
let reds be a new team with name "Reds" and captain ann.
add ann to the end of reds's players.
add bo to reds's players.
add list of 1, 2 to the end of reds's rounds.
say reds.
for each p in reds's players:
    say p's name followed by ": " followed by (p's scores added together).
set (item 2 of reds's players)'s name to "Bob".
say bo's name.
say the number of items in reds's players.
if reds's players contains bo, say "Bob plays".
if reds's players does not contain cy, say "Cy doesn't play".
say the position of bo in reds's players.
say the position of cy in reds's players.

note: copies of lists share the records in them
let squad be a copy of reds's players.
add cy to the end of squad.
say the number of items in reds's players.
set (first item of squad)'s name to "Annie".
say ann's name.

note: list forms with records
let everyone be list of ann, bo, cy, dee.
say (last item of everyone)'s name.
for each p in only everyone that are cy:
    say p's name.
for each p in items 2 to 3 of everyone:
    say p's name.
for each p in everyone reversed:
    say p's name.
for each p in everyone without the first item:
    say p's name.
remove item 1 from everyone.
say the number of items in everyone.
let groups be list of (list of ann, bo), (list of cy).
say the number of items in item 1 of groups.
say groups.
say empty list of players.
say reds's captain.
let crew be every p in everyone.
say the number of items in crew.
