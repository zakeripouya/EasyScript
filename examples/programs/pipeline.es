note: A sales report built from functions given to built-ins: apply, keep only,
note: combined using, and sorted by.

a sale is a kind of thing with:
    item, which is text.
    region, which is text.
    amount, which is a number.

let sales be an empty list of sales.
add (a new sale with item "tea", region "north" and amount 12) to the end of sales.
add (a new sale with item "coffee", region "south" and amount 30) to the end of sales.
add (a new sale with item "cake", region "north" and amount 7) to the end of sales.
add (a new sale with item "coffee", region "north" and amount 25) to the end of sales.
add (a new sale with item "tea", region "south" and amount 4) to the end of sales.
add (a new sale with item "cake", region "east" and amount 18) to the end of sales.

to price of s (a sale):
    give back s's amount.
to sum of so far (a number) and amount (a number):
    give back so far plus amount.

let amounts be price applied to every sale in sales.
say "All amounts: " followed by amounts.
say "Total: " followed by (amounts combined using sum, starting from 0).

note: a function made here can use names from around it
let threshold be 15.
let big be the sales in sales where its amount is at least threshold.
say "Big sales: " followed by the number of items in big.
let north be the sales in sales that pass (a function taking s that gives back s's region is "north").
say "North total: " followed by ((price applied to every sale in north) combined using sum, starting from 0).

note: the largest sale, by combining
let largest be sales combined using a function taking best and s:
    if s's amount is more than best's amount, give back s.
    give back best.
say "Largest: " followed by largest's item followed by " in the " followed by largest's region.

note: sentences change the list itself
let with tax be a copy of amounts.
apply (a function taking n that gives back n times 1.2) to every amount in with tax.
keep only the amounts in with tax where it is at least 10.
say "With tax, 10 or more: " followed by with tax.
sort sales by price, biggest first.
say "Biggest first: " followed by (the function item name applied to every sale in sales).
to item name of s (a sale):
    give back s's item.
