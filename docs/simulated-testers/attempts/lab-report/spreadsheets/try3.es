read file "readings.csv" and call it Data
for each Group in Data:
  let Avg be average of temperature
  let BiggestDrop be max of (previous temperature - temperature)
  say Group, Avg, BiggestDrop
for each Row in Data:
  let Fahrenheit be temperature * 9 / 5 + 32
  say group, minute, temperature, Fahrenheit
let GroupA be all rows with group is "A"
make a line chart of GroupA called "cooling.svg"
