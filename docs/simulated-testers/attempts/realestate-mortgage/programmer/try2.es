ask "House price: " and call the answer price
ask "Deposit: " and call the answer deposit
ask "Yearly interest rate (%): " and call the answer rate
ask "Years: " and call the answer years

let loan be price - deposit
let r be rate / 100 / 12
let n be years * 12

let payment be round(loan * r / (1 - pow(1 + r, -n)), 2)
let total be round(payment * n, 2)

print("Loan amount: " + loan)
print("Monthly payment: " + payment)
print("Total paid: " + total)
