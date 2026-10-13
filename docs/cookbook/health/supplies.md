# Check the supplies every week

Ask how many of each supply are on the shelf, and list what to order to get back to twice the lowest number. Say so when nothing needs ordering.

## The program

`supplies.es`:

```
a supply has a name, a unit and a lowest.
let supplies be a list of a new supply with name "gloves", unit "boxes" and lowest 20,
    a new supply with name "masks", unit "boxes" and lowest 15,
    a new supply with name "bandages", unit "packs" and lowest 30,
    a new supply with name "hand gel", unit "bottles" and lowest 10.
let orders be 0.
for each supply in supplies:
    ask "How many [supply's unit] of [supply's name] do we have? " and call it count.
    if count is below supply's lowest:
        say "  Order [twice supply's lowest minus count] [supply's unit] of [supply's name]."
        add 1 to orders.
if orders is 0, say "Nothing to order this week."
```

## Running it

When it's run with `easyscript run supplies.es` and these answers are typed (`25`, `8`, `30`, `3`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
How many boxes of gloves do we have? How many boxes of masks do we have?   Order 22 boxes of masks.
How many packs of bandages do we have? How many bottles of hand gel do we have?   Order 17 bottles of hand gel.
```
