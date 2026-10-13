read file "readings.csv" and call it readings
for each group in readings:
    let average be average of temperature
    let biggest be maximum of temperature before minus temperature after
    say average
    say biggest
for each reading in readings:
    let F be temperature * 9 / 5 + 32
    say F
let group A be all rows with group is "A"
make a line chart of group A called "cooling.svg"
