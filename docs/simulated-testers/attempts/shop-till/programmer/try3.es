let prices be a map of "coffee beans" to 12.50, "mug" to 8 and "filter papers" to 3.20
let total be 0
let product be ""

repeat until product is "done":
    ask "Item: " and call the answer product
    if product is "done", stop
    if prices doesn't contain product:
        say "Sorry, " + product + " isn't on the list."
        continue
    ask "How many: " and call the answer qty
    set total to total + (prices of product) * qty

say "Total: $" + total
if total is above 50:
    let discounted be total * 0.9
    say "Discounted total: $" + discounted
