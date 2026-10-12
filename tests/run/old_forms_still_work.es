note: The forms the "reads like text" round replaced in the examples still work.
a parcel has a weight, a country (text) and a fragile (yes or no).
the extra of a parcel depends on its country:
    0 when it's "Canada".
    12 otherwise.
the care of a parcel depends on its fragile:
    4 when it's yes.
    0 otherwise.
a parcel's price is 5 plus (the extra of it) plus (the care of it).
let box be a new parcel with weight 12, country "France" and fragile no.
set box's fragile to yes.
say box's price, box is fragile.
make a new file called "notes.txt", then write "milk" in file it,
then add "eggs" and "bread" to the end of file it, one per line.
say the contents of file "notes.txt".
if not (file "missing.txt" exists), say "no file".
unless file "missing.txt" exists, say "still no file".
let parcels be a list of box.
let totals per country be the total weight of parcels per country, sorted biggest first.
say the first item of the keys of totals per country.
say "Total: " followed by (box's price) as money.
let count be 4.
say "([count] items)".
