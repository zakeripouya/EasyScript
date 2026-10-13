note: The file's name isn't written out, so the program can't look at it before
note: it runs: "amount" is a number, and reading "$1200" stops the program.
put the text below in file "bank.csv":
    date,store,category,amount
    2026-08-01,Landlord,rent,$1200
    2026-08-04,Kroger,groceries,$96.40
    2026-09-20,Whole Foods,groceries,$312.75
    2026-10-08,Apple,electronics,$249

let name be "bank.csv".
read the table in name and call it purchases.
say the total amount of purchases.
