ask "name?" and call the answer name
ask "mass?" and call the answer mass
ask "volume?" and call the answer volume
let density be mass / volume rounded to 2 decimal places
if density < 1:
  say "floats"
else:
  say "sinks"
ask "another?" and call the answer another
