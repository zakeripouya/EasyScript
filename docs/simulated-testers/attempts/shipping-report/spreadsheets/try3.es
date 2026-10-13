read file "shipments.csv" and call it shipments
for each country in shipments:
    say country
    say count of shipments where country is country
let avg be average of "weight kg" in shipments
say avg
let heaviest be max of "weight kg" in shipments
say heaviest
for each row in shipments:
    let days be delivered - shipped
    if days > 5, say row
