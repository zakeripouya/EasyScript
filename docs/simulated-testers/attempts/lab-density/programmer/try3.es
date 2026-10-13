let again be "yes"
while again is not "no":
    ask "Object name: " and call the answer name
    ask "Mass (g): " and call the answer mass
    ask "Volume (cm^3): " and call the answer volume
    let density be (mass / volume) rounded to 2 decimal places
    if density < 1:
        let result be "floats"
    else:
        let result be "sinks"
    say name + ": density = " + density + " g/cm^3, it " + result
    ask "Another object? " and call the answer again
