ask "How much do you start with? " and call it balance.
ask "How much will you add each year? " and call it yearly.
ask "What's the yearly interest rate, in percent? " and call it rate.
count from 1 to 5 as year:
    add yearly to balance.
    add rate percent of balance to balance.
    say "After year [year]: [balance as money]".
