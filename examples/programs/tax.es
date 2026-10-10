note: A constant keeps a value that never changes, and functions can read it.
keep tax_rate as 0.2
keep shop at "Corner Shop"
to price_with_tax of amount:
    give back amount plus amount times tax_rate
say "Prices at " followed by shop followed by ":"
count from 10 to 30 by 10 as price:
    say price followed by " becomes " followed by (price_with_tax of price)
