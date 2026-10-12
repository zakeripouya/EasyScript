note: Yes/no fields read as describing words, and choices written as values.
a parcel has a weight and a country, and can be fragile.
a parcel can be urgent.
a parcel is abroad if its country isn't "Canada".
a parcel's care fee is 4 if it's fragile, otherwise 0.
a parcel's price is 5, plus 12 if it's abroad, plus 4 if it's fragile.
let box be a new fragile parcel with weight 12 and country "France".
let crate be a new parcel with weight 3 and country "Canada".
let parcels be a list of box and crate.
say box's fragile, crate's fragile, box's urgent.
if box is fragile, say "Handle with care.".
say how many parcels are fragile.
say the care fee of box, the care fee of crate.
say box's price, crate's price.
mark box as not fragile.
mark crate as urgent.
say box is fragile, crate is urgent.
for each parcel in the fragile parcels:
    say "still fragile".
let label be "glass" if box is fragile, otherwise "other".
say label.
say 1 if crate is urgent, otherwise 2.
if every parcel is urgent, say "all urgent".
set box's fragile to yes.
say box isn't fragile.
to rate with p, giving back a number:
    give back 10 if p is fragile, otherwise 20.
say rate with box.
let crates be a list of crate.
mark crate as not urgent.
say how many crates are urgent, the 1 if crate is urgent, otherwise 0.
let shipping be 3, minus 1 if crate is fragile.
say shipping.
say "Fee:", 2 if box is fragile, otherwise 1, "dollars".
