note: Definitions with their inputs in the middle, the way you'd say them.
a customer has a name and a price, and an orders count.
a customer is loyal if its orders count is over 5.

to give a customer a discount of some percent:
    reduce the customer's price by the percent percent.

to introduce a customer to another customer:
    say "[the customer's name], meet [the other customer's name].".

let ann be a new customer with name "Ann", price 100 and orders count 7.
let bo be a new customer with name "Bo", price 50 and orders count 2.
let customers be a list of ann and bo.

give every loyal customer a discount of 10 percent.
give bo a discount of 5 percent.
say "Ann pays", ann's price, "and Bo pays", bo's price.
introduce ann to bo.
