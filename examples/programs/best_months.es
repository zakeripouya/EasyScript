note: The "best months" request from the English pass plan, as a program: steps
note: in a row with "then", where "it" is what the step before gave. The first run
note: writes an example sales.csv to read.
if file "sales.csv" doesn't exist:
    put the text below in file "sales.csv":
        date,region,amount
        2026-08-14,East,120
        2026-09-03,West,310.50
        2026-09-21,East,95
        2026-10-02,North,180
        2026-10-09,East,42.25

read the table in "sales.csv", then total the amount per month,
then sort it biggest first, then save it as "best months.csv",
and then make a bar chart of it called "best months.svg".

show what's in file "best months.csv".
