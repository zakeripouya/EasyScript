Load sales.csv as sales
Add column revenue = quantity * price
Show SUM of revenue as "Total takings"
Group sales by product, SUM of quantity, sort descending, show top 1 as "Best seller"
Add column month = MONTH(date)
Pivot sales by month with SUM of revenue
Show the pivot
Make bar chart of the pivot, month on x axis, revenue on y axis
Save chart as months.svg
