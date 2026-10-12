note: The coffee shop's rules from the English pass plan, without the web parts:
note: drinks priced by a table, describing words, values, rules, questions,
note: a group discount, and an evening report on a schedule.

an email is text that must contain "@".
a drink has a name, a size and an extra shot.
an order has a customer, some drinks and a discount.
a customer has a name and an email.

a drink's size must not be empty.
an order's discount must be between 0 and 50.

the price of a drink depends on its size:
    $3 when it's "small".
    $4 when it's "medium".
    $5 otherwise.

a drink's cost is its price plus its shot.
the shot of a drink depends on its extra shot:
    $1 when it's yes.
    $0 otherwise.

an order's bill is the total cost of its drinks, minus its discount percent.
an order is big if its bill is over $12.
the biggest order is the one with the highest bill.

let latte be a new drink with name "latte", size "medium" and extra shot yes.
let tea be a new drink with name "tea", size "small" and extra shot no.
let mocha be a new drink with name "mocha", size "large" and extra shot yes.

let customers be a list of a new customer with name "Ann" and email "ann@example.com",
    a new customer with name "Bo" and email "bo@example.com".
let orders be a list of a new order with customer "Ann", drinks (a list of latte and tea) and discount 0,
    a new order with customer "Bo", drinks (a list of mocha, mocha and latte) and discount 0,
    a new order with customer "Ann", drinks (a list of tea) and discount 0.

say "A large drink costs", the price of mocha.
set every big order's discount to 10.

say "Orders:", how many orders there are.
say "Big orders:", how many orders are big.
say "Money made:", the total bill of orders.
say "Biggest order:", the biggest order's customer, "(", the biggest order's bill, ")".
let sold drinks be a list of latte, tea, mocha, mocha, latte and tea.
for each size and its count in the number of sold drinks per size, sorted biggest first:
    say "  [size]:", count, "sold".

let names be a list of "latte", "tea", "mocha", "mocha", "latte" and "mocha".
every day at 9pm:
    say "Evening report at [the time now]: the most common drink is", the most common name.
    stop the program.
