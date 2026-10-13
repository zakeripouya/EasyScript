# See which products need restocking

Each product has a stock count and a lowest number to keep. List the ones running low and how many to order to get back to twice their lowest.

## The program

`restock.es`:

```
a product has a name, a stock and a lowest.
let products be a list of a new product with name "Coffee beans", stock 4 and lowest 10,
    a new product with name "Mug", stock 25 and lowest 5,
    a new product with name "Filter papers", stock 30 and lowest 40,
    a new product with name "Grinder", stock 1 and lowest 2.
a product is low if its stock is below its lowest.
say "Running low:", how many products are low.
for each product in the low products:
    say "  Order [twice its lowest minus its stock] of [its name]."
```

## Running it

`easyscript run restock.es` shows:

```
Running low: 3
  Order 16 of Coffee beans.
  Order 50 of Filter papers.
  Order 3 of Grinder.
```
