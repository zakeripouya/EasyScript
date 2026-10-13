note: A report from a table. Replace the sales below with your own csv file,
note: or delete the "put" sentence and put your file in this folder.
put the text below in file "sales.csv":
    date,region,amount
    2026-09-02,North,$120.00
    2026-09-14,South,$80.50
    2026-10-01,North,$95.25
    2026-10-08,South,$143.10

read the table in "sales.csv", with amount in dollars, and call it sales.
say "Total:", the total amount of sales.
say "Average sale:", the average amount of sales.
for each region and its total in the total amount of sales per region:
    say "  [region]: [total]".
save a bar chart of the total amount of sales per month as "sales.svg", titled "Sales per month".
say "Saved a chart in sales.svg."
