# Spending per category from a bank export

Your bank export is a csv file, with one amount not filled in yet. Show the spending per category and per month, the biggest expense, and a pie chart.

## The program

`spending-by-category.es`:

```
read the table in "bank.csv", with amount in dollars, and call it purchases.
say "Spent per category:".
for each category and its total in the total amount of purchases per category, sorted biggest first:
    say "  [category]: [total]".
say "Spent per month:".
for each month and its total in the total amount of purchases per month:
    say "  [month]: [total]".
say "Biggest expense:", the description of the purchase with the highest amount.
save a pie chart of the total amount of purchases per category as "categories.svg".
```

## The data

`bank.csv`, in the same folder:

```
date,description,category,amount
2026-08-01,Rent,Housing,$1200
2026-08-04,Groceries,Food,$96.40
2026-08-11,Electric bill,Utilities,$71.25
2026-08-19,Groceries,Food,$110.15
2026-09-01,Rent,Housing,$1200
2026-09-06,Restaurant,Food,$58.00
2026-09-15,Internet,Utilities,$45.00
2026-09-22,Pharmacy,Health,
2026-09-28,Groceries,Food,$87.30
```

## Running it

`easyscript run spending-by-category.es` shows:

```
Spent per category:
  Housing: $2,400.00
  Food: $351.85
  Utilities: $116.25
Spent per month:
  August 2026: $1,477.80
  September 2026: $1,390.30
Biggest expense: Rent
```

It also makes `categories.svg`.

## Under the hood

`with amount in dollars` says what the column holds, so the program doesn't depend on what the file looks like. The blank amount (the pharmacy) is nothing: totals, averages and charts leave it out, as a spreadsheet does.
