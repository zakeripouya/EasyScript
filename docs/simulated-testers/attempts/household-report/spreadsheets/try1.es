Load spending.csv as a table with columns date, category, amount.
Make a column month = the year and month part of date.
Budget table:
  groceries = 400
  eating out = 150
  transport = 120
  fun = 100
Pivot table: rows = month, columns = category, values = SUM of amount.
Show the pivot table.
For each month and category, over = SUM of amount - budget for that category (lookup from budget table).
Show each month and category where over > 0, with the over amount.
Make a bar chart of total amount per month (SUM of amount grouped by month).
Save chart as months.svg.
