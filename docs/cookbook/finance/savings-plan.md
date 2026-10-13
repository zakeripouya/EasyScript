# Watch savings grow for five years

Ask how much you start with, how much you add each year, and the interest rate, then show the balance after each of five years.

## The program

`savings-plan.es`:

```
ask "How much do you start with? " and call it balance.
ask "How much will you add each year? " and call it yearly.
ask "What's the yearly interest rate, in percent? " and call it rate.
count from 1 to 5 as year:
    add yearly to balance.
    add rate percent of balance to balance.
    say "After year [year]: [balance as money]".
```

## Running it

When it's run with `easyscript run savings-plan.es` and these answers are typed (`1000`, `500`, `4`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
How much do you start with? How much will you add each year? What's the yearly interest rate, in percent? After year 1: $1,560.00
After year 2: $2,142.40
After year 3: $2,748.10
After year 4: $3,378.02
After year 5: $4,033.14
```

## Under the hood

The answers to the first two questions are used with an amount of money, so they're asked for as amounts (`$1000` or `1000` both work). `4 percent of balance` is worked out exactly and rounded to the cent.
