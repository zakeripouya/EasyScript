ask "Name of object?" and call the answer name
ask "Mass in gram?" and call the answer mass
ask "Volume in cubic centimeter?" and call the answer volume
let density be mass / volume
round density to 2 decimal
say density
if density is less than 1, say "it float"
else, say "it sink"
ask "Have another object?" and call the answer answer
if answer is yes, go to begin
if answer is no, stop
