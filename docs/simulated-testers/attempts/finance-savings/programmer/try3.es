ask "Starting amount: " and call the answer start_text
ask "Yearly deposit: " and call the answer deposit_text
ask "Yearly interest rate (%): " and call the answer rate_text

let balance be number(start_text)
let deposit be number(deposit_text)
let rate be number(rate_text)

for each year from 1 to 5:
    set balance to balance + deposit
    set balance to balance * (1 + rate / 100)
    say "Year " followed by year followed by ": " followed by (balance rounded to 2 decimal places)
