# Total a few months of shop sales

Your sales are in a csv file with a quantity and a price for each sale. Show the money taken, the best-selling product, the money per month, and save a bar chart.

## The program

`monthly-sales.es`:

```
read the table in "sales.csv" and call it sales.
a sale's money is its price times its quantity.
say "Money taken:", the total money of sales.
say "Best seller:", the product with the highest total quantity.
say "Money per month:".
for each month and its money in the total money of sales per month:
    say "  [month]: [money]".
save a bar chart of the total money of sales per month as "months.svg".
```

## The data

`sales.csv`, in the same folder:

```
date,product,quantity,price
2026-08-03,Coffee beans,4,$12.50
2026-08-03,Mug,2,$8.00
2026-08-17,Filter papers,10,$3.20
2026-09-02,Coffee beans,6,$12.50
2026-09-14,Mug,1,$8.00
2026-09-21,Grinder,1,$45.00
2026-10-01,Coffee beans,3,$12.50
2026-10-05,Filter papers,12,$3.20
```

## Running it

`easyscript run monthly-sales.es` shows:

```
Money taken: $301.90
Best seller: Filter papers
Money per month:
  August 2026: $98.00
  September 2026: $128.00
  October 2026: $75.90
```

It also makes `months.svg`.

## Under the hood

The prices are written with `$`, so EasyScript reads them as exact amounts of money: `$12.50 times 4` is exactly `$50.00`, with no rounding errors, and totals print as money.
