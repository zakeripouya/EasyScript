# A simple till for the counter

Keep asking for an item and how many, until someone types done. Show the total, with 10% off when it's over $50.

## The program

`till.es`:

```
let prices be a map of "coffee beans" to $12.50, "mug" to $8 and "filter papers" to $3.20.
let total be $0.
forever:
    ask "Item (or done): " and call it item.
    if item is "done", stop the loop.
    unless prices has item:
        say "Sorry, we don't sell [item]."
        skip this one.
    ask "How many? " and call it quantity.
    add the value for item in prices times quantity to total.
say "Total:", total.
if total is over $50:
    say "With 10% off:", total, minus 10 percent.
```

## Running it

When it's run with `easyscript run till.es` and these answers are typed (`mug`, `3`, `coffee beans`, `2`, `teapot`, `filter papers`, `5`, `done`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
Item (or done): How many? Item (or done): How many? Item (or done): Sorry, we don't sell teapot.
Item (or done): How many? Item (or done): Total: $65.00
With 10% off: $58.50
```

## Under the hood

The answer to "How many?" is used as a number (`times quantity`), so EasyScript asks again until a number is typed. The prices are kept in a map from text to amounts.
