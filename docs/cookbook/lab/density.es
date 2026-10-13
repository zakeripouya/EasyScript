let another be yes.
while another:
    ask "Object? " and call it name.
    ask "Mass in grams? " and call it mass.
    ask "Volume in cubic centimeters? " and call it volume.
    let density be mass divided by volume, rounded to 2 decimals.
    if density is below 1:
        say name, "has a density of", density, "and floats."
    otherwise:
        say name, "has a density of", density, "and sinks."
    ask yes or no "Another object? " and call it another.
