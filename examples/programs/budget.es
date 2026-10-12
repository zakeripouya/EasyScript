note: The program a budget session in the notebook would save (from the
note: English pass plan). The first run writes an example spending.csv.
if file "spending.csv" doesn't exist:
    put the text below in file "spending.csv":
        date,store,category,amount
        2026-09-01,Landlord,rent,1200
        2026-09-05,Whole Foods,groceries,84.20
        2026-09-12,Nobu,eating out,96.40
        2026-09-20,Whole Foods,groceries,312.75
        2026-10-01,Landlord,rent,1200
        2026-10-03,Sushi Bar,eating out,71.25
        2026-10-08,Apple,electronics,249

read the table in "spending.csv" and call it purchases.
a purchase is big if its amount is over $200.
say "Purchases: [how many purchases there are]".
say "Total: [the total amount of purchases]".
show the total amount of purchases per category, sorted biggest first, as a table.
say "Top store:", the store with the highest total amount.
show the big purchases whose category isn't "rent", sorted by amount biggest first, as a table.
