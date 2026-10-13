read file "shipments.csv" and call it data

let counts be a new list
for each row in data:
    add 1 to counts of row country
say "Shipments per country:"
say counts

let total be 0
for each row in data:
    add row weight kg to total
let average be total / number of rows in data
say "Average weight: " + average

let heaviest be the first row in data
for each row in data:
    if row weight kg is more than heaviest weight kg:
        set heaviest to row
say "Heaviest: " + heaviest id + " " + heaviest weight kg

say "Took more than 5 days:"
for each row in data:
    let days be row delivered - row shipped
    if days is more than 5:
        say row id + " " + days
