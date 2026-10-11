note: Orders kept with their dates: totals per month, this month's orders,
note: and the orders from the earliest to the latest.
an order is a kind of thing with:
    item, which is text.
    day, which is a date.
    price, which is a number.

let orders be an empty list of orders
add (a new order with item "tea", day today, price 4) to the end of orders
add (a new order with item "cake", day 3 days ago, price 6.5) to the end of orders
add (a new order with item "jam", day 35 days ago, price 3) to the end of orders
add (a new order with item "honey", day 40 days ago, price 8) to the end of orders
add (a new order with item "bread", day 70 days ago, price 2.25) to the end of orders

say "This month:"
for each order in orders:
    if order's day is in this month:
        say "  " followed by order's item followed by " on " followed by order's day

let months be a list of "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November" and "December"
let per month be an empty map from numbers to numbers
for each order in orders:
    let month be the month of order's day
    set the value for month in per month to (the value for month in per month, or 0) plus order's price
say "Per month:"
for each month and total in per month:
    say "  " followed by item month of months followed by ": " followed by total

let days be (a function taking order that gives back order's day) applied to every order in orders
say "The first order was on " followed by the smallest of days followed by "."
say "The last order was on " followed by the biggest of days followed by "."

say "From the earliest:"
sort orders by (a function taking order that gives back order's day)
for each order in orders:
    say "  " followed by order's item
say the first item of orders
