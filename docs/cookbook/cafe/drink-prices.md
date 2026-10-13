# Price drinks by their size

A drink's price depends on its size. Show each drink in an order with its price, and the total.

## The program

`drink-prices.es`:

```
a drink has a name and a size.
the price of a drink depends on its size:
    $3 when it's "small".
    $4 when it's "medium".
    $5 otherwise.
let order be a list of a new drink with name "Latte" and size "large",
    a new drink with name "Tea" and size "small",
    a new drink with name "Mocha" and size "medium".
for each drink in order:
    say drink's name, "([drink's size]):", drink's price.
say "Total:", the total price of order.
```

## Running it

`easyscript run drink-prices.es` shows:

```
Latte (large): $5.00
Tea (small): $3.00
Mocha (medium): $4.00
Total: $12.00
```
