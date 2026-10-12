note: "read the table in F": rows whose fields are the columns the program uses.
put "date,region,Amount,paid,note" in file "sales.csv".
add "2026-09-03,East,120,yes,first" to the end of file "sales.csv".
add "2026-10-01,West,80.5,no,\"late, again\"" to the end of file "sales.csv".
add "2026-10-15 14:30,East,40,yes," to the end of file "sales.csv".
add "2026-09-20,North,300,true,big one" to the end of file "sales.csv".
read the table in "sales.csv" and call it sales.
say the total amount of sales.
say the total amount of sales per region, sorted biggest first.
say the number of sales per month.
for each sale in sales:
    say sale's region, sale's amount, sale's paid, sale's date.
say how many sales whose amount is over 100.
say item 3 of sales.
set (item 1 of sales)'s amount to 7.
save the sales whose region is "East" as "east.csv".
say the contents of file "east.csv".
read the table in "sales.csv", then total the amount per month,
    then sort it biggest first, then save it as "best months.csv".
say the contents of file "best months.csv".
read the table in "east.csv", called east.
say east.
