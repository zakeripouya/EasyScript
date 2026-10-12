a customer has a name (text), a total (a number) and a nickname (text or nothing).
a product has a name (text) and a price (a number).
a customer is best if its total is over 100.
a customer's discount is its total times 0.1.
let ann be a new customer with name "Ann", total 200 and nickname nothing.
let pen be a new product with name "Pen" and price 2.
if ann is loyal, say "yes".
if pen is best, say "yes".
say pen's discount.
say ann's total, or 0.
say ann's nickname, or 5.
let counts be an empty map from text to numbers.
add 1 to the value for "a" in counts, starting at "zero".
let x be 3.
the label depends on x:
    "small" when it's under 10.
    5 otherwise.
the verdict depends on x:
    "big" when it's "x".
    "other" otherwise.
an age must be between 0 and 150.
a product's colour must not be empty.
a product's price must be "cheap".
a customer's name must contain x.
if x is 3:
    a product's name must not be empty.
keep fixed as pen's price, or 1.
