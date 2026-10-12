note: A field across a list (the names of the drinks, each drink's name), and counting a list field (they have more than 1 orders).
a drink has a name and a size.
an order has a customer and some drinks (a list of drinks).
a customer has a name and some orders (a list of orders).
a customer is loyal if they have more than 1 orders.
let latte be a new drink with name "latte" and size "medium".
let tea be a new drink with name "tea" and size "small".
let drinks be a list of latte and tea.
say the names of the drinks.
say each drink's size.
say the names of drinks whose size is "small".
let o1 be a new order with customer "Ann" and drinks (a list of latte).
let o2 be a new order with customer "Ann" and drinks (a list of tea and latte).
let ann be a new customer with name "Ann" and orders (a list of o1 and o2).
let bo be a new customer with name "Bo" and orders (a list of o1).
let customers be a list of ann and bo.
say ann is loyal, bo is loyal.
if ann has at least 2 orders, say "two or more".
if bo has exactly 1 orders, say "one".
say the names of the loyal customers.
say how many customers have fewer than 2 orders.
for each customer in customers:
    say customer's name, customer has at most 1 orders.
let ages be a map of "Ann" to 30.
say ages has "Ann", the sizes of the drinks.
say every customer has at least 1 orders, any customer has more than 5 orders.
