note: A column of amounts written with "$" is money because the file says so,
note: in a read with nothing after it and in a second read of the file in steps.
put the text below in file "bank.csv":
    date,store,category,amount
    2026-08-01,Landlord,rent,$1200
    2026-08-04,Kroger,groceries,$96.40
    2026-09-20,Whole Foods,groceries,$312.75
    2026-10-08,Apple,electronics,$249

read the table in "bank.csv" and call it purchases.
say the total amount of purchases.
read the table in "bank.csv", then total the amount per month,
then make a bar chart of it called "spending.svg".
show what's in file "spending.svg".
