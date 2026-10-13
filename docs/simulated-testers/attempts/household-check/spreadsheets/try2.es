Ask "Income for the month?" and call the answer income.
Ask "Rent?" and call the answer rent.
Ask "Utilities?" and call the answer utilities.
Ask "Insurance?" and call the answer insurance.
Ask "Phone?" and call the answer phone.
Let bills be SUM(rent, utilities, insurance, phone).
Let left be income - bills.
Show "Left after bills: " plus left
IF left < 500 then show "Warning: less than $500 left!"
Ask "How much do you want to save each month?" and call the answer savings.
Let months be ROUNDUP(5000 / savings, 0).
Show "Months to reach $5,000 emergency fund: " plus months
