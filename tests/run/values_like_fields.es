note: Values defined with "depends on" or "KIND's NAME is" read like fields everywhere, and ", plus N if C".
a parcel has a weight and a country, and can be fragile.
a parcel is abroad if its country isn't "Canada".
the base price of a parcel depends on its weight:
    5 when it's under 2.
    9 otherwise.
the extra of a parcel depends on its country:
    0 when it's "Canada".
    12 otherwise.
a parcel's care is 4 if it's fragile, otherwise 0.
a parcel's price is its base price plus its extra plus its care.
a parcel's total is its price, minus 1 if it's abroad.
let box be a new fragile parcel with weight 12 and country "France".
let parcels be a list of box.
say box's price, the price of box, box's extra, the base price of box, box's total.
say the total price of parcels, the parcels sorted by price.
for each parcel in parcels:
    say parcel's care.
let crate be a new parcel with weight 1 and country "Canada".
add crate to the end of parcels.
say the parcels whose price is over 10.
say the parcel with the highest price's country, how many parcels whose extra is 0.
say the parcels sorted by total biggest first.
say the price of crate, crate's total.
say 10, minus 2 if crate is abroad, plus 3 if crate is fragile.
