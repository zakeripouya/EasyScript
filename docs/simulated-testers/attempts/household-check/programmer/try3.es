ask "Income for the month: " and call the answer income
ask "Rent: " and call the answer rent
ask "Utilities: " and call the answer utilities
ask "Insurance: " and call the answer insurance
ask "Phone: " and call the answer phone

let total_bills be rent + utilities + insurance + phone
let left be income - total_bills
print("Left after bills: $" + left)

if left < 500:
    print("WARNING: less than $500 left after bills!")

ask "How much to save per month: " and call the answer savings
let goal be 5000
let months be goal / savings rounded up
print("Months to reach $5,000 emergency fund: " + months)
