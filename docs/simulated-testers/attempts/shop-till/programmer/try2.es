let prices be a list of "coffee beans" is 12.50, "mug" is 8, "filter papers" is 3.20
let total be 0

repeat:
    ask "Item: " and call the answer item
    if item is "done", stop
    if item is not in prices:
        say "Sorry, " + item + " isn't on the list."
        continue
    ask "How many: " and call the answer qty
    set total to total + price of item in prices * qty

say "Total: $" + total
if total is above 50:
    let discounted be total * 0.9
    say "Discounted total: $" + discounted
