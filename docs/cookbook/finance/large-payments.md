# Find the large payments

List the card payments over $100, smallest first, and say how many there are.

## The program

`large-payments.es`:

```
read the table in "card.csv", with amount in dollars, and call it purchases.
let large be the purchases whose amount is over $100.
say how many purchases there are, "payments,", the number of items in large, "of them over $100:".
for each purchase in large sorted by amount:
    say "  [purchase's date]  [purchase's description]: [purchase's amount]".
```

## The data

`card.csv`, in the same folder:

```
date,description,amount
2026-09-02,Flights,$412.80
2026-09-05,Coffee,$4.50
2026-09-09,Hotel,$268.00
2026-09-12,Books,$36.20
2026-09-20,Laptop repair,$145.00
```

## Running it

`easyscript run large-payments.es` shows:

```
5 payments, 3 of them over $100:
  2026-09-20  Laptop repair: $145.00
  2026-09-09  Hotel: $268.00
  2026-09-02  Flights: $412.80
```
