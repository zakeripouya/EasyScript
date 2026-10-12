note: Grouping by a date: per month, per year, per day, per weekday, per month of FIELD.
a sale has an amount (a number), a day (a date) and a paid day (a date).
a visit has a when (a date).
let sales be a list of (a new sale with amount 10, day today and paid day today), (a new sale with amount 5, day 40 days ago and paid day today), (a new sale with amount 7, day 1 day ago and paid day 1 day ago).
say the total amount of sales per month of day.
say the total amount of sales per year of paid day.
say the number of sales per weekday of day.
say the total amount of sales per day of paid day.
let visits be a list of (a new visit with when today) and (a new visit with when 400 days ago).
say the number of visits per year.
say the number of visits per month.
for each month and count in the number of visits per month:
    say month, ":", count.
