# A mortgage payment calculator

Ask for the price, the deposit, the interest rate and the number of years. Show the loan, the monthly payment and what's paid in all.

## The program

`mortgage.es`:

```
ask "House price? " and call it price.
ask "Deposit? " and call it deposit.
ask "Yearly interest rate, in percent? " and call it rate.
ask "How many years? " and call it years.
let loan be price minus deposit.
let monthly rate be rate divided by 100 divided by 12.
let months be years times 12.
let factor be 1 minus (1 plus monthly rate) to the power of (0 minus months).
let payment be loan times monthly rate divided by factor.
say "Loan:", loan as money.
say "Monthly payment:", payment as money.
say "Paid in all:", payment times months, as money.
```

## Running it

When it's run with `easyscript run mortgage.es` and these answers are typed (`350000`, `70000`, `6`, `25`), it shows (the questions and answers run together here, because the typed answers aren't shown):

```
House price? Deposit? Yearly interest rate, in percent? How many years? Loan: $280,000.00
Monthly payment: $1,804.04
Paid in all: $541,213.18
```

## Under the hood

The payment uses the usual formula, written in steps with names, so each line is easy to check. `to the power of` works with any number, including the minus number of months here.
