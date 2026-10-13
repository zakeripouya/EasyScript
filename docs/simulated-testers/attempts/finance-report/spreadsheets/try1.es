Load bank.csv as a table.
Remove rows where amount is blank.
Convert the amount column to numbers (remove the $ sign).
Create a pivot table: rows = category, values = SUM of amount.
Create a pivot table: rows = month of date, values = SUM of amount.
Show MAX of amount, with the matching description, category and date.
Make a pie chart from the category pivot table: labels = category, values = SUM of amount.
Save the chart as categories.svg.
