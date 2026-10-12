an order has an amount (a number), a size (text) and a note (text or nothing).
an order is big if its amount is over 1000.
a small order is one whose amount is under 10.
an order's tax is its amount times 0.2.
the best order is the one with the highest amount.
the big orders are the ones that are big.
the price of an order depends on its size:
    3 when it's "small".
    5 otherwise.
the fee depends on total:
    0 when it's over 100.
    5 otherwise.
an amount must be between 0 and 10000.
an order's size must not be empty.
say o is big, o isn't small, how many orders are big, the big orders, the best order, o's tax.
say o's note, or "none" if there isn't one.
add 1 to the value for w in counts, starting at 0.
remove the orders that are big.
set every order's size to "small".
