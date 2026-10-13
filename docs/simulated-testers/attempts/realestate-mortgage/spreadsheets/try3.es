ask "House price?" and call the answer Price
ask "Deposit?" and call the answer Deposit
ask "Yearly interest rate (%)?" and call the answer Rate
ask "Number of years?" and call the answer Years
let Loan be Price - Deposit
let MonthlyRate be Rate / 100 / 12
let Months be Years * 12
let Factor be (1 + MonthlyRate) POWER -Months
let Payment be Loan * MonthlyRate / (1 - Factor)
let Payment be ROUND Payment, 2
let Total be Payment * Months
say "Loan amount: " + Loan
say "Monthly payment: " + Payment
say "Total paid: " + Total
