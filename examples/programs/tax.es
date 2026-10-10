note: A constant keeps a value that never changes, and functions can read it.
keep tax rate as 0.2
keep shop at "Corner Shop"
to add tax to amount:
    give back amount plus amount times tax rate
say "Prices at " followed by shop followed by ":"
count from 10 to 30 by 10 as price:
    say price followed by " becomes " followed by (add tax to price)
