balance = number(input("Starting amount: "))
deposit = number(input("Yearly deposit: "))
rate = number(input("Yearly interest rate (%): "))

for year in range(1, 6):
    balance = balance + deposit
    balance = balance * (1 + rate / 100)
    print("Year " + year + ": " + round(balance, 2))
