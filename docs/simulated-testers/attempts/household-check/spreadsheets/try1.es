Ask "Income for the month?" and put it in income.
Ask "Rent?" and put it in rent.
Ask "Utilities?" and put it in utilities.
Ask "Insurance?" and put it in insurance.
Ask "Phone?" and put it in phone.
bills = SUM(rent, utilities, insurance, phone)
left = income - bills
Show "Left after bills: " & left
IF left < 500 then show "Warning: less than $500 left!"
Ask "How much do you want to save each month?" and put it in savings.
months = ROUNDUP(5000 / savings, 0)
Show "Months to reach $5,000 emergency fund: " & months
