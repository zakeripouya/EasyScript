ask "Starting amount: " and call the answer balance
ask "Yearly deposit: " and call the answer deposit
ask "Yearly interest rate (%): " and call the answer rate

for each year from 1 to 5:
    set balance to balance + deposit
    set balance to balance * (1 + rate / 100)
    say "Year " + year + ": " + (balance rounded to 2 decimal places)
