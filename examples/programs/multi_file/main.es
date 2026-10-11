note: A program in two files: this one, which runs, and shapes.es, which it uses.
use shapes.

let garden be a list of (a new rectangle with width 4 and height 3), (a new circle with radius 1) and (a new rectangle with width 2 and height 2)
let total be 0
for each bed in garden:
    say shapes's summary of bed
    add area of bed to total
say "All together: " followed by total
