ask "House price: " and call the answer price
ask "Deposit: " and call the answer deposit
ask "Yearly interest rate (%): " and call the answer rate
ask "Years: " and call the answer years

let loan be price - deposit
let r be rate / 100 / 12
let n be years * 12

let factor be 1
for each i from 1 to n:
    set factor to factor * (1 + r)

let payment be loan * r / (1 - 1 / factor)
set payment to round payment to 2
let total be payment * n

print("Loan amount: " + loan)
print("Monthly payment: " + payment)
print("Total paid: " + total)
