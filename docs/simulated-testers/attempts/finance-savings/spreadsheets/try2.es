Ask "Start balance?" and call the answer start.
Ask "Yearly deposit?" and call the answer deposit.
Ask "Interest rate (%)?" and call the answer rate.
Let balance be start.
For each year from 1 to 5:
    Set balance to (balance + deposit) * (1 + rate / 100).
    Say year and balance rounded to 2 decimal places.
