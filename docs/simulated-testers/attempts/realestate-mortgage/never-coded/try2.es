ask "What is the price of the house?" and call the answer house price
ask "How much is the deposit?" and call the answer deposit
ask "What is the yearly interest rate in percent?" and call the answer interest rate
ask "How many years will the loan last?" and call the answer years
let loan amount be house price minus deposit
say loan amount
let monthly rate be interest rate divided by 100 divided by 12
let number of months be years times 12
let monthly payment be loan amount times monthly rate divided by (1 minus (1 plus monthly rate) to the power of minus number of months)
say monthly payment rounded to the cent
let total paid be monthly payment times number of months
say total paid
