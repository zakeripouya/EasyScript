let Another be "yes"
repeat while Another is "yes":
  ask "Object name?" and call the answer Name
  ask "Mass (g)?" and call the answer Mass
  ask "Volume (cm3)?" and call the answer Volume
  let Density be Mass / Volume rounded to 2 decimal places
  if Density < 1:
    let Result be "floats"
  else:
    let Result be "sinks"
  say Name, Density, Result
  ask "Another object? (yes/no)" and call the answer Another
