note: Definitions as sentences: adjectives, values worked out from a record, and named values.
an order has a customer (text), an amount (a number) and a cost (a number).
a customer has a name (text) and some orders (a list of orders).
an order is big if its amount is over 1000.
a cheap order is one whose amount is under 100.
a tiny order is one where its amount is below 10.
an order's profit is its amount minus its cost.
a customer's profit is the total profit of their orders.
a customer is loyal if the number of items in their orders is at least 2.
the best customer is the one with the highest profit.
the big orders are the ones that are big.
the poor customers are the ones whose profit is below 600.
let o1 be a new order with customer "Ann", amount 1200 and cost 700.
let o2 be a new order with customer "Ann", amount 50 and cost 20.
let o3 be a new order with customer "Bo", amount 900 and cost 100.
let o4 be a new order with customer "Cy", amount 5 and cost 1.
let orders be a list of o1, o2, o3 and o4.
let customers be a list of (a new customer with name "Ann" and orders (a list of o1 and o2)), (a new customer with name "Bo" and orders (a list of o3)).
say how many orders are big, how many orders are cheap, how many orders are tiny.
say the big orders.
say the number of items in the cheap orders, the number of items in the big orders.
if o1 is big, say "o1 is big".
if o2 is not big and o2 isn't tiny, say "o2 is neither".
say o1's profit, the profit of o3, the total profit of orders.
say the best customer's name.
say the loyal customers.
if any customer is loyal, say "Someone is loyal.".
if every loyal customer's profit is above 100, say "Loyal customers made money.".
say the customer with the lowest profit's name.
say the poor customers.
for each customer:
    say customer's name, customer's profit, customer is loyal.
say the orders where it's profit is above (the profit of o3 minus 1).
