note: The "best months" request from the English pass plan, as a program: steps
note: in a row with "then", where "it" is what the step before gave. The first run
note: writes an example sales.csv to read.
if not (file "sales.csv" exists):
    put "date,region,amount" in file "sales.csv"
    add "2026-08-14,East,120" to the end of file "sales.csv"
    add "2026-09-03,West,310.50" to the end of file "sales.csv"
    add "2026-09-21,East,95" to the end of file "sales.csv"
    add "2026-10-02,North,180" to the end of file "sales.csv"
    add "2026-10-09,East,42.25" to the end of file "sales.csv"

read the table in "sales.csv", then total the amount per month,
then sort it biggest first, then save it as "best months.csv",
and then make a bar chart of it called "best months.svg".

say the contents of file "best months.csv".
