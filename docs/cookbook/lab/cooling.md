# Cooling readings in Celsius and Fahrenheit

Your class measured how fast water cools. Show each group's average temperature, every reading in Fahrenheit, and a line chart of group A.

## The program

`cooling.es`:

```
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
```

## The data

`readings.csv`, in the same folder:

```
group,minute,temperature
A,0,80
A,5,71
A,10,64
A,15,59
B,0,82
B,5,70
B,10,62
B,15,57
```

## Running it

`easyscript run cooling.es` shows:

```
Average temperature per group:
  A: 68.5 °C
  B: 67.75 °C
Every reading in Fahrenheit:
  A at minute 0: 176 °F
  A at minute 5: 159.8 °F
  A at minute 10: 147.2 °F
  A at minute 15: 138.2 °F
  B at minute 0: 179.6 °F
  B at minute 5: 158 °F
  B at minute 10: 143.6 °F
  B at minute 15: 134.6 °F
```

It also makes `cooling.svg`.
