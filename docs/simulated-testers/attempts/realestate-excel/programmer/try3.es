read file "listings.xlsx" and call it listings

let totals be {}
let counts be {}
let cheapest be {}

for each row in listings:
    let hood be row["neighborhood"]
    let price be row["price"]
    if hood is not in totals:
        set totals[hood] to 0
        set counts[hood] to 0
        set cheapest[hood] to row
    set totals[hood] to totals[hood] + price
    set counts[hood] to counts[hood] + 1
    if price < cheapest[hood]["price"]:
        set cheapest[hood] to row

for each hood in totals:
    let avg be totals[hood] / counts[hood]
    say hood + ": avg price = " + avg + ", cheapest = " + cheapest[hood]["address"]
