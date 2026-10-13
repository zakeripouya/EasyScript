# Roll a die 20 times

Roll a die 20 times, then show the rolls, their total, how many sixes, and the most common roll.

## The program

`dice.es`:

```
let rolls be an empty list of numbers.
repeat 20 times:
    add roll a die to the end of rolls.
say "Rolls:", rolls.
say "Total:", the total of rolls.
say "Sixes:", how many rolls are 6.
say "Most common:", the most common roll of rolls.
```

## Running it

`easyscript run dice.es` shows:

```
Rolls: 1, 3, 1, 5, 3, 6, 5, 5, 5, 2, 6, 5, 4, 6, 1, 6, 2, 1, 2 and 2
Total: 71
Sixes: 4
Most common: 5
```
