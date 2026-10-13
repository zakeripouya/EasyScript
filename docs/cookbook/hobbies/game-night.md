# Who wins game night

From the results of your board game nights, show who has won the most, the average winning score of each game, and the top 3 winning scores.

## The program

`game-night.es`:

```
read the table in "games.csv" and call it games.
say "Most wins:", the winner with the highest number of games.
say "Average winning score per game:".
for each game and its average in the average score of games per game:
    say "  [game]: [average rounded to 1 decimal]".
say "Top 3 winning scores:".
for each game in the top 3 games by score:
    say "  [game's score] by [game's winner] at [game's game]".
```

## The data

`games.csv`, in the same folder:

```
date,game,winner,score
2026-09-04,Catan,Maya,10
2026-09-04,Ticket to Ride,Jon,124
2026-09-11,Catan,Jon,11
2026-09-11,Azul,Maya,78
2026-09-18,Ticket to Ride,Maya,141
2026-09-18,Azul,Priya,85
2026-09-25,Catan,Maya,10
2026-09-25,Ticket to Ride,Priya,118
```

## Running it

`easyscript run game-night.es` shows:

```
Most wins: Maya
Average winning score per game:
  Catan: 10.3
  Ticket to Ride: 127.7
  Azul: 81.5
Top 3 winning scores:
  141 by Maya at Ticket to Ride
  124 by Jon at Ticket to Ride
  118 by Priya at Ticket to Ride
```
