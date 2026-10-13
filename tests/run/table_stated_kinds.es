# "read the table in F, with COLUMN in dollars | as a number": the stated
# kind wins over what the file looks like. Plain numbers in a column of
# amounts are that currency, when the program is checked and when the file
# is read (here, its name isn't written out, so only reading can tell).
put the text below in file "bank.csv":
    date,description,amount,quantity
    2026-08-01,Rent,1200,1
    2026-08-04,Groceries,96.40,3
    2026-08-11,Electric,71.25,1

read the table in "bank.csv", with amount in dollars and quantity as a number, and call it purchases.
say the total amount of purchases.
say the total quantity of purchases.
read the table in "bank.csv" and call it again, with amount in dollars.
say the average amount of again.
let name be "mixed.csv".
put the text below in file name:
    date,amount
    2026-08-01,$1200
    2026-08-02,96.40
    2026-08-03,"1,250.50 dollars"

read the table in name and call it mixed, with amount in dollars.
say the total amount of mixed.
put the text below in file "fares.csv":
    city,fare
    Paris,12
    Rome,7.50

read the table in "fares.csv", with fare in euros, then total the fare per city, then say it.
