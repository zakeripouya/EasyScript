Ask "Start balance?" and call the answer start.
Ask "Yearly deposit?" and call the answer deposit.
Ask "Interest rate (%)?" and call the answer rate.
Let balance be VALUE(start).
Let yearly be VALUE(deposit).
Let percent be VALUE(rate).
For each year from 1 to 5:
    Set balance to (balance + yearly) * (1 + percent / 100).
    Say year followed by " " followed by balance rounded to 2 decimal places.
