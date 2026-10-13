read file "readings.csv" and call it readings
for each group in readings:
    let average be average of temperature
    let drop be biggest drop between two reading next each other
    say average
    say drop
for each reading in readings:
    let F be temperature * 9 / 5 + 32
    say F
make line chart for temperature of group A and save as "cooling.svg"
