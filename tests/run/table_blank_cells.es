# A blank cell in a column of numbers or amounts is nothing: totals,
# averages, medians, the top N, the highest and groups skip it; "how many"
# counts rows; showing and saving leave it blank; ", or" and "is nothing"
# work on it.
put the text below in file "bank.csv":
    date,description,category,amount,count
    2026-08-01,Rent,Housing,$1200,1
    2026-08-04,Groceries,Food,$96.40,
    2026-09-22,Pharmacy,Health,,2
    2026-09-28,Groceries,Food,$87.30,4

read the table in "bank.csv" and call it purchases.
say "Total:", the total amount of purchases.
say "Average:", the average amount of purchases.
say "Median:", the median amount of purchases.
say "Count total:", the total count of purchases.
say "Average count:", the average count of purchases.
say "How many:", how many purchases there are.
say the total amount of purchases per category.
say the average count of purchases per category.
say the description of the purchase with the highest amount.
for each purchase in the top 2 purchases by amount:
    say "Top:", purchase's description.
for each purchase in purchases sorted by amount:
    say "Sorted:", purchase's description.
for each purchase in purchases:
    say purchase's description, purchase's amount, or $0.
    if purchase's count is nothing, say "  no count".
    if purchase's amount is not nothing, say "  has an amount".
show purchases as a table.
say item 2 of purchases.
save purchases as "out.csv".
show what's in file "out.csv".
save a bar chart of the total amount of purchases per category as "spending.svg".
say "Chart saved:", file "spending.svg" exists.
