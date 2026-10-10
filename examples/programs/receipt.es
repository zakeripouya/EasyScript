note: A shop receipt: constants for the values that never change, a function, and a loop.
keep shop as "Corner Shop"
keep tax rate as 0.2
keep rule as "------------------------"
to add tax to amount:
    give back amount plus amount times tax rate
let total be 0
say shop
say rule
count from 1 to 3 as item:
    let price be item times 2.5
    say "Item " followed by item followed by ": " followed by price
    add price to total
say rule
say "Subtotal: " followed by total
say "With tax: " followed by add tax to total
