note: The program a budget session in the notebook would save (from the
note: English pass plan). The first run writes an example spending.csv.
unless file "spending.csv" exists:
    put "date,store,category,amount" in file "spending.csv"
    add "2026-09-01,Landlord,rent,1200" to the end of file "spending.csv"
    add "2026-09-05,Whole Foods,groceries,84.20" to the end of file "spending.csv"
    add "2026-09-12,Nobu,eating out,96.40" to the end of file "spending.csv"
    add "2026-09-20,Whole Foods,groceries,312.75" to the end of file "spending.csv"
    add "2026-10-01,Landlord,rent,1200" to the end of file "spending.csv"
    add "2026-10-03,Sushi Bar,eating out,71.25" to the end of file "spending.csv"
    add "2026-10-08,Apple,electronics,249" to the end of file "spending.csv"

read the table in "spending.csv" and call it purchases.
a purchase is big if its amount is over 200.
say "Purchases: [how many purchases there are]".
say "Total: [the total amount of purchases as money]".
show the total amount of purchases per category, sorted biggest first, as a table.
let totals per store be the total amount of purchases per store, sorted biggest first.
say "Top store:", the first item of the keys of totals per store.
show the big purchases whose category isn't "rent", sorted by amount biggest first, as a table.
