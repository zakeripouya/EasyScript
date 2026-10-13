Repeat:
  Name = ask "Object name?"
  Mass = ask "Mass (g)?"
  Volume = ask "Volume (cm3)?"
  Density = ROUND(Mass / Volume, 2)
  Result = IF(Density < 1, "floats", "sinks")
  Show Name, Density, Result
  Another = ask "Another object? (yes/no)"
Until Another = "no"
