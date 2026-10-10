note: a list is shared: every name for it sees the same list
let scores be a list of 1, 2 and 3
let same be scores
add 4 to the end of same
say scores
set item 1 of scores to 10
say same
say same is scores

note: "a copy of" makes a separate list
let separate be a copy of scores
add 5 to the end of separate
remove item 1 from separate
say scores
say separate

note: functions get the list itself, so they can change it
to add_bonus with marks (a list of numbers):
    add 100 to the end of marks
add_bonus with scores
say scores

to double_all with marks, giving back a list of numbers:
    give back every mark in marks times 2
let doubled be double_all with scores
say doubled
say scores

to fresh_list, giving back a list of text:
    let made be a list of "new"
    give back made
let first_made be fresh_list
let second_made be fresh_list
add "changed" to the end of first_made
say first_made
say second_made

note: nested lists share their inner lists; a copy copies them too
let row be a list of 1 and 2
let grid be a list of row and row
add 3 to the end of row
say grid
let grid_copy be a copy of grid
add 4 to the end of item 1 of grid_copy
say grid
say grid_copy

note: a variable keeps its list until it's given another
set same to a list of 7
say same
say scores
