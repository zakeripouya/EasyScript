# Count the guests and their meals

From the RSVPs, show how many said yes, the headcount with the extra guests they bring, the meals needed, and a pie chart of the meals.

## The program

`guest-count.es`:

```
read the table in "guests.csv" and call it guests.
let coming be the guests whose rsvp is "yes".
a guest's people is 1 plus its plus ones.
say "Said yes:", the number of items in coming.
say "Headcount:", the total people of coming.
let meals be the total people of coming per meal.
for each meal and its count in meals:
    say "  [meal]: [count]".
save a pie chart of meals as "meals.svg", titled "Meals".
```

## The data

`guests.csv`, in the same folder:

```
name,rsvp,meal,plus ones
Ana,yes,fish,1
Bo,no,veggie,0
Cy,yes,veggie,0
Dee,maybe,fish,1
Eli,yes,chicken,2
Fay,yes,veggie,1
Gus,no,chicken,0
```

## Running it

`easyscript run guest-count.es` shows:

```
Said yes: 4
Headcount: 8
  fish: 2
  veggie: 3
  chicken: 3
```

It also makes `meals.svg`.
