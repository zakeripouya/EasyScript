note: A shipping calculator: tables, describing words, values, rules and defaults
note: instead of "if"s. (English pass part 3.)

a parcel has a weight (a number), a country (text), a fragile (yes or no) and a note (text or nothing).

a weight must be between 0 and 70.
a parcel's country must not be empty.

a parcel is heavy if its weight is over 20.
a parcel is abroad if its country isn't "Canada".

the base price of a parcel depends on its weight:
    5 when it's under 2.
    9 when it's under 10.
    15 when it's under 20.
    25 otherwise.

a parcel's price is its base price plus (the extra of it) plus (the care of it).
the extra of a parcel depends on its country:
    0 when it's "Canada".
    12 otherwise.
the care of a parcel depends on its fragile:
    4 when it's yes.
    0 otherwise.

the priciest parcel is the one with the highest price.

let parcels be a list of (a new parcel with weight 1.5, country "Canada", fragile no and note nothing), (a new parcel with weight 12, country "France", fragile yes and note "glass"), (a new parcel with weight 30, country "Canada", fragile no and note nothing).

for each parcel:
    say parcel's weight, "kg to", parcel's country, "costs", parcel's price, "(" followed by (parcel's note, or "no note"), ")".

say "Heavy parcels:", how many parcels are heavy.
say "Going abroad:", how many parcels are abroad.
say "Total:", the total price of parcels.
say "Priciest:", the priciest parcel's price.
let counts be an empty map from text to numbers.
for each parcel:
    add 1 to the value for (parcel's country) in counts, starting at 0.
say counts.
