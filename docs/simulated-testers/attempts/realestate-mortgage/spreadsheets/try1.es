Ask for Price
Ask for Deposit
Ask for Rate
Ask for Years
Loan = Price - Deposit
MonthlyRate = Rate / 100 / 12
Months = Years * 12
Payment = ROUND(Loan * MonthlyRate / (1 - (1 + MonthlyRate)^(-Months)), 2)
Total = Payment * Months
Show "Loan amount: " & Loan
Show "Monthly payment: " & Payment
Show "Total paid: " & Total
