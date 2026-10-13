ask "House price?" and call the answer Price
ask "Deposit?" and call the answer Deposit
ask "Yearly interest rate (%)?" and call the answer Rate
ask "Number of years?" and call the answer Years
let Loan be Price - Deposit
let MonthlyRate be Rate / 100 / 12
let Months be Years * 12
let Payment be ROUND(Loan * MonthlyRate / (1 - POWER(1 + MonthlyRate, -Months)), 2)
let Total be Payment * Months
say "Loan amount: " + Loan
say "Monthly payment: " + Payment
say "Total paid: " + Total
