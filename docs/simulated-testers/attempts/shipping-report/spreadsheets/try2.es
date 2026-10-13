read file "shipments.csv" and call it shipments
let counts be count of shipments by country
say counts
let avg be average of weight kg in shipments
say avg
let heaviest be max of weight kg in shipments
say heaviest
for each row in shipments:
    let days be delivered - shipped
    if days > 5, say row
