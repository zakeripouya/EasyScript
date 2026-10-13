ask "What is the name of the object?" and call the answer name
ask "What is its mass in grams?" and call the answer mass
ask "What is its volume in cubic centimeters?" and call the answer volume
let density be mass divided by volume, rounded to 2 decimal places
say "The density is " and density
if density is less than 1, say "It floats in water."
otherwise, say "It sinks in water."
ask "Is there another object?" and call the answer another
if another is yes, then do it all again
if another is no, then stop
