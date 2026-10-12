note: A shipping calculator: tables, describing words, values, rules and defaults
note: instead of "if"s. (English pass part 3.)

a parcel has a weight (a number), a country (text) and a note (text or nothing), and can be fragile.

a weight must be between 0 and 70.
a parcel's country must not be empty.

a parcel is heavy if its weight is over 20.
a parcel is abroad if its country isn't "Canada".

the base price of a parcel depends on its weight:
    5 when it's under 2.
    9 when it's under 10.
    15 when it's under 20.
    25 otherwise.

a parcel's price is its base price, plus 12 if it's abroad, plus 4 if it's fragile.

the priciest parcel is the one with the highest price.

let parcels be a list of a new parcel with weight 1.5, country "Canada" and note nothing,
    a new fragile parcel with weight 12, country "France" and note "glass",
    a new parcel with weight 30, country "Canada" and note nothing.

for each parcel:
    let remark be parcel's note, or "no note".
    say parcel's weight, "kg to", parcel's country, "costs", parcel's price, "([remark])".

say "Heavy parcels:", how many parcels are heavy.
say "Going abroad:", how many parcels are abroad.
say "Total:", the total price of parcels.
say "Priciest:", the priciest parcel's price.
say the number of parcels per country.
