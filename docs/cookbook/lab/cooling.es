read the table in "readings.csv" and call it readings.
say "Average temperature per group:".
for each group and its average in the average temperature of readings per group:
    say "  [group]: [average] °C".
say "Every reading in Fahrenheit:".
for each reading in readings:
    let fahrenheit be reading's temperature times 9 divided by 5 plus 32.
    say "  [reading's group] at minute [reading's minute]: [fahrenheit] °F".
let group a be the readings whose group is "A".
save a line chart of the average temperature of group a per minute as "cooling.svg", titled "Group A cooling".
