read file "readings.csv" and call it data

for each group in ["A", "B"]:
    let rows be all rows of data with group is group
    let temps be temperature of rows
    let avg be sum(temps) / count(temps)
    let biggest_drop be 0
    for each i from 2 to count(temps):
        let drop be item (i - 1) of temps - item i of temps
        if drop > biggest_drop:
            set biggest_drop to drop
    say "Group " + group + ": average = " + avg + ", biggest drop = " + biggest_drop

for each row in data:
    let f be temperature of row * 9 / 5 + 32
    say group of row + " minute " + minute of row + ": " + f + " F"

let group_a be all rows of data with group is "A"
make a line chart of group_a with minute and temperature and save it as "cooling.svg"
