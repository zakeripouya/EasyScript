# Total a week of café sales

Last week's orders are in a csv file. Show the money taken each day, the most popular item, the average order and the busiest day.

## The program

`week-of-sales.es`:

```
read the table in "orders.csv" and call it orders.
say "Money taken each day:".
for each day and its total in the total price of orders per day:
    say "  [day]: [total]".
say "Most popular item:", the item with the highest number of orders.
say "Average order:", the average price of orders.
say "Busiest day:", the day with the highest number of orders.
```

## The data

`orders.csv`, in the same folder:

```
day,item,size,price
Monday,Latte,large,$4.50
Monday,Tea,small,$2.00
Monday,Latte,small,$3.50
Tuesday,Espresso,small,$2.50
Tuesday,Latte,large,$4.50
Wednesday,Cappuccino,large,$4.80
Wednesday,Tea,large,$2.80
Wednesday,Latte,small,$3.50
Wednesday,Espresso,small,$2.50
Thursday,Tea,small,$2.00
Friday,Latte,large,$4.50
Friday,Cappuccino,small,$3.80
```

## Running it

`easyscript run week-of-sales.es` shows:

```
Money taken each day:
  Monday: $10.00
  Tuesday: $7.00
  Wednesday: $13.60
  Thursday: $2.00
  Friday: $8.30
Most popular item: Latte
Average order: $3.41
Busiest day: Wednesday
```
