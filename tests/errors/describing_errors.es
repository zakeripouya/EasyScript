a parcel has a weight and a country, and can be fragile.
let box be a new fragile parcel with weight 12 and country "France".
let label be "glass" if box is fragile, otherwise 0.
let size be 1 if box's weight, otherwise 2.
mark box as weight.
let total be box's weight, plus 4 if box's country.
