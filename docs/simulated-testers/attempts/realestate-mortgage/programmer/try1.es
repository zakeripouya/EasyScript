price = number(input("House price: "))
deposit = number(input("Deposit: "))
rate = number(input("Yearly interest rate (%): "))
years = number(input("Years: "))

loan = price - deposit
r = rate / 100 / 12
n = years * 12

function monthlyPayment(loan, r, n) {
    return loan * r / (1 - (1 + r) ** (-n))
}

payment = round(monthlyPayment(loan, r, n), 2)
total = round(payment * n, 2)

print("Loan amount: " + loan)
print("Monthly payment: " + payment)
print("Total paid: " + total)
