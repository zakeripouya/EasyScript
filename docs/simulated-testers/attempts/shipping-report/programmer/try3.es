read file "shipments.csv" and call it data

let total be 0
let count be 0
let heaviest be 0
let heaviest_id be ""

for each row in data:
    let country be row's country
    let weight be row's "weight kg"
    add weight to total
    add 1 to count
    if weight is more than heaviest:
        set heaviest to weight
        set heaviest_id to row's id

say "Average weight: " + total / count
say "Heaviest: " + heaviest_id + " " + heaviest

say "Took more than 5 days:"
for each row in data:
    let days be row's delivered - row's shipped
    if days is more than 5:
        say row's id + " " + days

say "Shipments per country:"
for each country in data's country:
    say country + ": " + how many rows in data have country
