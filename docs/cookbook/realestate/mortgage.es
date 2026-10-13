ask "House price? " and call it price.
ask "Deposit? " and call it deposit.
ask "Yearly interest rate, in percent? " and call it rate.
ask "How many years? " and call it years.
let loan be price minus deposit.
let monthly rate be rate divided by 100 divided by 12.
let months be years times 12.
let factor be 1 minus (1 plus monthly rate) to the power of (0 minus months).
let payment be loan times monthly rate divided by factor.
say "Loan:", loan as money.
say "Monthly payment:", payment as money.
say "Paid in all:", payment times months, as money.
