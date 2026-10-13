read file "readings.csv" and call it readings

for each group in readings:
    say the average temperature of the group
    let biggest be the largest difference between one temperature and the next one
    say biggest

for each reading in readings:
    let fahrenheit be temperature times 9 divided by 5 plus 32
    say fahrenheit

let group A temperatures be all rows with group is "A"
make a line chart of group A temperatures called "cooling.svg"
