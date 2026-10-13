# Does it float? A density calculator

Ask for an object's mass and volume, work out its density to 2 decimal places, and say whether it floats in water. Keep going until the answer to "Another object?" is no.

## The program

`density.es`:

```
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
```

## Running it

When it's run with `easyscript run density.es` and these answers are typed (`cork`, `4.8`, `20`, `yes`, `steel bolt`, `31.4`, `4`, `no`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
Object? Mass in grams? Volume in cubic centimeters? cork has a density of 0.24 and floats.
Another object? Object? Mass in grams? Volume in cubic centimeters? steel bolt has a density of 7.85 and sinks.
Another object? 
```
