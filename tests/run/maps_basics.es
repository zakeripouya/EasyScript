note: making maps, reading, setting, asking, changing, removing, and printing

let ages be an empty map from text to numbers.
say "empty: [" followed by ages followed by "]"
say ages is empty
set the value for "Paris" in ages to 19.
set the value for "Junie" in ages to 24.
say ages
say ages is not empty
say the value for "Paris" in ages
say the number of entries in ages

note: setting an existing key keeps its place
set the value for "Paris" in ages to 20.
say ages

note: defaults, only worked out when the key is missing
say the value for "Bob" in ages, or 0
say the value for "Junie" in ages, or 0
say (the value for "Bob" in ages, or 99) plus 1
to noisy default:
    say "(working out the default)"
    give back -1
say the value for "Junie" in ages, or noisy default
say the value for "Bob" in ages, or noisy default

note: asking
say ages has "Paris"
say ages has "Bob"
say ages does not have "Bob"
say ages doesn't have "Paris"
if ages has "Junie", say "Junie is " followed by the value for "Junie" in ages
say the keys of ages
say the values of ages
say the number of items in the keys of ages

note: changing values in place
add 5 to the value for "Paris" in ages.
subtract 1 from the value for "Paris" in ages.
increase the value for "Junie" in ages by 10.
decrease the value for "Junie" in ages by 4.
multiply the value for "Paris" in ages by 2.
divide the value for "Junie" in ages by 3.
say ages

note: removing, then adding again puts the key at the end
remove "Paris" from ages.
say ages
set the value for "Paris" in ages to 1.
set the value for "Ada" in ages to 36.
say ages
remove "Junie" from ages.
remove "Ada" from ages.
remove "Paris" from ages.
say ages is empty

note: written out
let prices be a map of "coffee" to 3, "tea" to 2 and "milk" to 1.
say prices
let sizes be a map of 1 to "small", 2 to "medium", 3 to "large".
say sizes
say the value for 2 in sizes
let flags be a map of "on" to yes and "off" to no.
say flags
say the value for "on" in flags

note: number keys that print the same are the same key
let found be a map of 0.1 plus 0.2 to "three tenths".
say the value for 0.3 in found
set the value for 0.3 in found to "replaced"
say found
say the number of entries in found
let negative zero be a map of -0 to "zero".
say the value for 0 in negative zero

note: going through a map, in order
for each item name and price in prices:
    say item name followed by " costs " followed by price
let total be 0
for each name and price in prices:
    add price to total
say total
for each n and size in sizes:
    if n is 2, skip this one
    say n followed by ": " followed by size

note: changing a map while going through it doesn't change what the loop visits
for each name and price in prices:
    remove name from prices
    set the value for name followed by "!" in prices to price
say prices

note: as text, joined with other text
say "Prices: " and prices
say "Sizes: " followed by sizes
let shown be sizes as text
say the number of characters in shown

note: sharing and copying
let same prices be prices
set the value for "water" in same prices to 0
say prices has "water"
let copied be a copy of prices
set the value for "juice" in copied to 4
say prices has "juice"
say copied has "juice"
say copied is prices
remove "juice" from copied
say copied is prices
let reordered be a map of "water" to 0, "coffee!" to 3, "milk!" to 1, "tea!" to 2.
say reordered is prices
say reordered is not prices
note: number keys within rounding of each other (12 significant digits) are one key, like "is"
let near be a map of 1 to "one"
set the value for 1.0000000000001 in near to "still one"
set the value for 1.00000000001 in near to "another"
say the number of entries in near
say the value for 1 in near
