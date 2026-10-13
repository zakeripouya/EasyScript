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
