a parcel has a weight and a country.
the cost of a parcel depends on its weight:
    $5 when it's at most 1.
    $9 when it's at most 5.
    $15 when it's at most 20.
    $30 otherwise.
let parcels be a list of a new parcel with weight 0.8 and country "France",
    a new parcel with weight 3.2 and country "Germany",
    a new parcel with weight 22 and country "Spain".
for each parcel in parcels:
    say parcel's weight, "kg to", parcel's country, "costs", parcel's cost.
say "All together:", the total cost of parcels.
