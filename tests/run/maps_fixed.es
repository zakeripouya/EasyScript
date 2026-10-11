note: fixed maps, made with "keep"
keep prices as a map of "coffee" to 3, "tea" to 2.
keep sizes as a map of 1 to "small", 2 to "medium" and 3 to "large".
keep switches as a map of "on" to yes, "off" to no.
keep closed menu as an empty map from text to numbers.
keep size count at the number of entries in sizes.
keep big as a map of 1 to 1, 2 to 4, 3 to 9, 4 to 16, 5 to 25, 6 to 36, 7 to 49, 8 to 64, 9 to 81, 10 to 100, 11 to 121, 12 to 144.

say prices
say the value for "tea" in prices
say the value for "milk" in prices, or 0
say prices has "coffee"
say sizes
say size count
say switches
say closed menu is empty
say the value for 11 in big
say big has 13
say the keys of prices
say the values of sizes
for each name and price in prices:
    say name followed by ": " followed by price

note: a name that keeps a fixed map gets its own copy, which can change
let mine be prices
set the value for "milk" in mine to 1
say mine
say prices
let squares be big
remove 1 from squares
say the number of entries in squares
say the number of entries in big

note: in a function, and as a field's starting value
to cost of item:
    give back the value for item in prices, or -1
say cost of "tea"
say cost of "cake"
an order is a kind of thing with:
    a menu, which is a map from text to numbers, starting at prices.
let first order be a new order
let second order be a new order
set the value for "cake" in first order's menu to 5
say first order's menu
say second order's menu
say prices is first order's menu
say prices is second order's menu
