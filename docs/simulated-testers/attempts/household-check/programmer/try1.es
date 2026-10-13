income = number(input("Income for the month: "))

bills = {}
for name in ["rent", "utilities", "insurance", "phone"]:
    bills[name] = number(input(name + ": "))

total_bills = sum(bills.values())
left = income - total_bills
print("Left after bills: $" + left)

if left < 500:
    print("WARNING: less than $500 left after bills!")

savings = number(input("How much to save per month: "))
goal = 5000
months = ceil(goal / savings)
print("Months to reach $5,000 emergency fund: " + months)
