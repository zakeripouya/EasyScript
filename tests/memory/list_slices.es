# Slices made and dropped 300,000 times (ranges, first/last, every second
# item, without, of nested lists): each is freed when nothing holds it.
let total be 0
repeat 300000 times:
    let row be a list of it, it plus 1, it plus 2 and it plus 3
    let grid be a list of row and (row reversed)
    let part be items 2 to 3 of row
    let ends be (the first 1 items in row) followed by (the last 2 items in grid) as text
    set total to total plus (part added together) plus length of (every second item of row) plus length of (grid without the first item) plus length of ends
say total
